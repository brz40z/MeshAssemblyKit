import bpy
import json
import os

class OBJECT_OT_MissingMasterDialog(bpy.types.Operator):
    bl_idname = "object.missing_master_dialog"
    bl_label = "Unlinked Meshes Found!"
    bl_options = {'INTERNAL'}
    missing_names: bpy.props.StringProperty()

    def execute(self, context): return {'FINISHED'}
    def invoke(self, context, event): return context.window_manager.invoke_props_dialog(self, width=400)
    def draw(self, context):
        layout = self.layout
        layout.label(text="These meshes are not Masters AND have no Master reference:", icon='ERROR')
        box = layout.box()
        for name in self.missing_names.split("|"):
            box.label(text=f"• {name}")

class OBJECT_OT_SyncOriginalProperty(bpy.types.Operator):
    bl_idname = "object.sync_original_property"
    bl_label = "Set Selection as Master"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        active_obj = context.active_object
        state = active_obj.ue_is_original if active_obj else True
        mesh_selection = [obj for obj in context.selected_objects if obj.type == 'MESH']
        for obj in mesh_selection:
            obj.ue_is_original = state
            obj["Original"] = 1 if state else 0
        return {'FINISHED'}

class OBJECT_OT_ExportUEData(bpy.types.Operator):
    bl_idname = "object.export_ue_instances"
    bl_label = "Copy JSON"
    bl_options = {'REGISTER'}

    def execute(self, context):
        selection = [obj for obj in context.selected_objects if obj.type == 'MESH']
        assets_list = []
        missing_masters = []
        all_masters = [o for o in bpy.data.objects if o.type == 'MESH' and o.get("Original") == 1]

        for obj in selection:
            is_master = obj.get("Original") == 1
            source_name = obj.name if is_master else next((m.name for m in all_masters if m.data == obj.data), None)
            
            if source_name:
                loc, quat, scale = obj.matrix_world.decompose()
                assets_list.append({
                    "source_asset": source_name,
                    "instance_id": obj.name,
                    "parent_name": obj.parent.name if obj.parent else None,
                    "transform": {
                        "rotation": {"x": round(quat.x, 4), "y": round(quat.y * -1, 4), "z": round(quat.z, 4), "w": round(quat.w * -1, 4)},
                        "translation": {"x": round(loc.x * 100, 4), "y": round(loc.y * -100, 4), "z": round(loc.z * 100, 4)},
                        "scale3D": {"x": round(scale.x, 4), "y": round(scale.y, 4), "z": round(scale.z, 4)}
                    }
                })
            else:
                missing_masters.append(obj.name)

        if missing_masters:
            bpy.ops.object.missing_master_dialog('INVOKE_DEFAULT', missing_names="|".join(missing_masters))
            return {'FINISHED'}

        context.window_manager.clipboard = json.dumps({"assets": assets_list}, indent=4)
        return {'FINISHED'}

class OBJECT_OT_BatchExportFBX(bpy.types.Operator):
    bl_idname = "object.batch_export_fbx"
    bl_label = "Batch Export FBX"
    bl_options = {'REGISTER'}
    
    _timer = None
    _masters = []
    _index = 0

    def modal(self, context, event):
        if event.type == 'TIMER':
            if self._index >= len(self._masters): return self.finish(context)
            obj = self._masters[self._index]
            self._index += 1
            
            orig_m = obj.matrix_world.copy()
            obj.matrix_world.identity() 
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            
            bpy.ops.export_scene.fbx(
                filepath=os.path.join(self._export_path, f"{obj.name}.fbx"), 
                use_selection=True, bake_space_transform=True, axis_forward='X', axis_up='Z'
            )
            obj.matrix_world = orig_m
        return {'RUNNING_MODAL'}

    def execute(self, context):
        self._masters = [obj for obj in context.selected_objects if obj.get("Original") == 1]
        if not self._masters: return {'CANCELLED'}
        
        raw_path = context.scene.ue_export_path
        if raw_path.startswith("//") and not bpy.data.is_saved:
            self.report({'ERROR'}, "Save file first!")
            return {'CANCELLED'}
            
        self._export_path = bpy.path.abspath(raw_path)
        if not os.path.exists(self._export_path): os.makedirs(self._export_path)
        
        self._timer = context.window_manager.event_timer_add(0.001, window=context.window)
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def finish(self, context):
        context.window_manager.event_timer_remove(self._timer)
        return {'FINISHED'}

classes = (OBJECT_OT_MissingMasterDialog, OBJECT_OT_SyncOriginalProperty, OBJECT_OT_ExportUEData, OBJECT_OT_BatchExportFBX)

def register():
    for cls in classes: bpy.utils.register_class(cls)

def unregister():
    for cls in reversed(classes): bpy.utils.unregister_class(cls)