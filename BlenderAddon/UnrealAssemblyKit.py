import bpy
import json
import os

# --- ADDON METADATA ---
bl_info = {
    "name": "Unreal Assembly Kit",
    "author": "Bozhyk Yuriy",
    "version": (1, 2, 0),
    "blender": (5, 0, 0),
    "location": "View3D > Sidebar > Unreal Assembly Kit",
    "description": "Batch export to FBX, transfer instances to Unreal Engine via JSON",
    "warning": "",
    "category": "Import-Export",
}

# --- 1. POPUP DIALOG FOR MISSING MASTERS ---
class OBJECT_OT_MissingMasterDialog(bpy.types.Operator):
    bl_idname = "object.missing_master_dialog"
    bl_label = "Unlinked Meshes Found!"
    bl_options = {'INTERNAL'}
    
    missing_names: bpy.props.StringProperty()

    def execute(self, context):
        return {'FINISHED'}

    def invoke(self, context, event):
        return context.window_manager.invoke_props_dialog(self, width=400)

    def draw(self, context):
        layout = self.layout
        layout.label(text="These meshes are not Masters AND have no Master reference:", icon='ERROR')
        box = layout.box()
        for name in self.missing_names.split("|"):
            box.label(text=f"• {name}")
        layout.label(text="Check Mesh Data or tag a source as 'Original'.")

# --- 2. OPERATOR: SYNC PROPERTY ---
class OBJECT_OT_SyncOriginalProperty(bpy.types.Operator):
    bl_idname = "object.sync_original_property"
    bl_label = "Set Selection as Master"
    bl_description = "Applies the 'Original' tag to all selected meshes"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        active_obj = context.active_object
        state = active_obj.ue_is_original if active_obj else True
        mesh_selection = [obj for obj in context.selected_objects if obj.type == 'MESH']
        
        if not mesh_selection:
            self.report({'WARNING'}, "No meshes selected.")
            return {'CANCELLED'}
            
        for obj in mesh_selection:
            obj.ue_is_original = state
            if state:
                obj["Original"] = 1
            else:
                if "Original" in obj: del obj["Original"]
                
        self.report({'INFO'}, f"Synced {len(mesh_selection)} meshes.")
        return {'FINISHED'}

# --- 3. OPERATOR: EXPORT JSON ---
class OBJECT_OT_ExportUEData(bpy.types.Operator):
    bl_idname = "object.export_ue_instances"
    bl_label = "Copy JSON"
    bl_description = "Copy selected mesh transforms to clipboard for Unreal"
    bl_options = {'REGISTER'}

    def execute(self, context):
        selection = [obj for obj in context.selected_objects if obj.type == 'MESH']
        assets_list = []
        missing_masters = []
        unit_scale = 100.0 
        
        all_masters = [o for o in bpy.data.objects if o.type == 'MESH' and o.get("Original") in (1, True)]

        for obj in selection:
            is_master = obj.get("Original") in (1, True)
            source_name = obj.name if is_master else next((m.name for m in all_masters if m.data == obj.data), None)
            
            if source_name:
                loc, quat, scale = obj.matrix_world.decompose()
                assets_list.append({
                    "source_asset": source_name,
                    "instance_id": obj.name,
                    "parent_name": obj.parent.name if obj.parent else None,
                    "transform": {
                        "rotation": {"x": round(quat.x, 4), "y": round(quat.y * -1, 4), "z": round(quat.z, 4), "w": round(quat.w * -1, 4)},
                        "translation": {"x": round(loc.x * unit_scale, 4), "y": round(loc.y * unit_scale * -1, 4), "z": round(loc.z * unit_scale, 4)},
                        "scale3D": {"x": round(scale.x, 4), "y": round(scale.y, 4), "z": round(scale.z, 4)}
                    }
                })
            else:
                missing_masters.append(obj.name)

        if missing_masters:
            names_str = "|".join(missing_masters)
            bpy.ops.object.missing_master_dialog('INVOKE_DEFAULT', missing_names=names_str)
            return {'FINISHED'}

        if assets_list:
            context.window_manager.clipboard = json.dumps({"assets": assets_list}, indent=4)
            self.report({'INFO'}, f"Copied {len(assets_list)} items.")
        return {'FINISHED'}

# --- 4. OPERATOR: BATCH FBX ---
class OBJECT_OT_BatchExportFBX(bpy.types.Operator):
    bl_idname = "object.batch_export_fbx"
    bl_label = "Batch Export FBX"
    bl_options = {'REGISTER'}
    
    _timer = None
    _masters = []
    _index = 0
    _total = 0
    _export_path = ""

    def modal(self, context, event):
        if event.type == 'TIMER':
            if self._index >= self._total:
                return self.finish(context)

            obj = self._masters[self._index]
            self._index += 1
            
            # Update UI
            percent = int((self._index / self._total) * 100)
            context.workspace.status_text_set(f"Exporting: {percent}% | {obj.name}")
            
            # Export Process
            orig_m = obj.matrix_world.copy()
            obj.matrix_world.identity() 
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            context.view_layer.objects.active = obj
            
            full_path = os.path.join(self._export_path, f"{obj.name}.fbx")
            bpy.ops.export_scene.fbx(
                filepath=full_path, use_selection=True, global_scale=1.0, 
                apply_unit_scale=False, apply_scale_options='FBX_SCALE_NONE',
                bake_space_transform=True, axis_forward='X', axis_up='Z',
                mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False
            )
            obj.matrix_world = orig_m
            
        return {'RUNNING_MODAL'}

    def execute(self, context):
        masters = [obj for obj in context.selected_objects if obj.type == 'MESH' and obj.get("Original") in (1, True)]
        if not masters:
            self.report({'ERROR'}, "Select tagged 'Original' meshes first!")
            return {'CANCELLED'}
            
        self._masters = masters
        self._total = len(masters)
        self._index = 0
        self._export_path = bpy.path.abspath(context.scene.ue_export_path)
        
        if not os.path.exists(self._export_path):
            os.makedirs(self._export_path)

        context.window_manager.progress_begin(0, self._total)
        self._timer = context.window_manager.event_timer_add(0.001, window=context.window)
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def finish(self, context):
        context.window_manager.event_timer_remove(self._timer)
        context.window_manager.progress_end()
        context.workspace.status_text_set(None)
        self.report({'INFO'}, "Batch Export Complete.")
        return {'FINISHED'}

# --- 5. UI PANEL ---
class VIEW3D_PT_UEExporter(bpy.types.Panel):
    bl_label = "Unreal Assembly Kit"
    bl_idname = "VIEW3D_PT_ue_exporter"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Unreal Assembly Kit'

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        obj = context.active_object
        
        row = layout.row(align=True)
        row.prop(scene, "ue_ui_tab", expand=True)
        
        if scene.ue_ui_tab == 'MASTERS':
            box = layout.box()
            if obj and obj.type == 'MESH':
                box.prop(obj, "ue_is_original", text="Is Original")
            else:
                box.label(text="Active is not a Mesh", icon='INFO')
            
            box.operator("object.sync_original_property", icon='FILE_TICK')
            
            layout.separator()
            layout.prop(scene, "ue_export_path")
            layout.operator("object.batch_export_fbx", icon='FILE_3D')
        else:
            layout.operator("object.export_ue_instances", icon='COPY_ID', text="Copy Instances JSON")

# --- REGISTRATION ---

classes = (
    OBJECT_OT_MissingMasterDialog,
    OBJECT_OT_SyncOriginalProperty,
    OBJECT_OT_ExportUEData,
    OBJECT_OT_BatchExportFBX,
    VIEW3D_PT_UEExporter,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
        
    bpy.types.Scene.ue_ui_tab = bpy.props.EnumProperty(
        items=[('MASTERS', "Masters", "Export Source Meshes"), 
               ('INSTANCES', "Instances", "Copy Instance Data")],
        default='MASTERS'
    )
    bpy.types.Scene.ue_export_path = bpy.props.StringProperty(
        name="Path", 
        subtype='DIR_PATH', 
        default="//Export/"
    )
    bpy.types.Object.ue_is_original = bpy.props.BoolProperty(
        name="Is Original", 
        default=False
    )

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
        
    del bpy.types.Scene.ue_ui_tab
    del bpy.types.Scene.ue_export_path
    del bpy.types.Object.ue_is_original

if __name__ == "__main__":
    register()