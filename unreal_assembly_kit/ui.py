import bpy

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
            box.operator("object.sync_original_property", icon='FILE_TICK')
            layout.separator()
            layout.prop(scene, "ue_export_path")
            layout.operator("object.batch_export_fbx", icon='FILE_3D')
        else:
            layout.operator("object.export_ue_instances", icon='COPY_ID', text="Copy Instances JSON")

def register():
    bpy.utils.register_class(VIEW3D_PT_UEExporter)

def unregister():
    bpy.utils.unregister_class(VIEW3D_PT_UEExporter)