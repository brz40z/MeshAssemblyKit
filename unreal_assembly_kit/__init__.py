bl_info = {
    "name": "Unreal Assembly Kit",
    "author": "Bozhyk Yuriy",
    "version": (1, 2, 1),
    "blender": (5, 0, 0),
    "location": "View3D > Sidebar > Unreal Assembly Kit",
    "description": "Batch export to FBX, transfer instances to Unreal Engine via JSON",
    "category": "Import-Export",
}

import bpy
from . import operators
from . import ui

modules = [operators, ui]

def register():
    # Register Properties
    bpy.types.Scene.ue_ui_tab = bpy.props.EnumProperty(
        items=[('MASTERS', "Masters", "Export Source Meshes"), 
               ('INSTANCES', "Instances", "Copy Instance Data")],
        default='MASTERS'
    )
    bpy.types.Scene.ue_export_path = bpy.props.StringProperty(
        name="Path", subtype='DIR_PATH', default="//Export/"
    )
    bpy.types.Object.ue_is_original = bpy.props.BoolProperty(
        name="Is Original", default=False
    )

    # Register Classes
    operators.register()
    ui.register()

def unregister():
    ui.unregister()
    operators.unregister()

    del bpy.types.Scene.ue_ui_tab
    del bpy.types.Scene.ue_export_path
    del bpy.types.Object.ue_is_original

if __name__ == "__main__":
    register()