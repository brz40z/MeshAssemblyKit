# MeshAssemblyKit

**Version**: 0.0.1
**Blender Version**: 4.5.0 and above
**Unreal Engine Version**: 5.4
**Author**: Bozhyk Yuriy
**Email**: bozheka@gmail.com

Mesh Assembly Kit is a workflow utility designed to bridge between external DCCs (like Blender) and Unreal Engine.
> NOTE: The Unreal plugin and Blender addon are designed to work together.
## Core Features
1. **Batch Export:** Export multiple meshes from Blender to FBX format.
2. **Instance Data Transfer:** Copy mesh instance data (location, rotation, scale) to the clipboard for seamless transfer.
3. **Automatic Assembly:** Rebuild layouts in Unreal Engine by automatically matching copied instance data.
4. **Quick Reimport:** Reimport selected meshes.
5. **HISM Conversion:** Instantly convert selected static mesh instances into a single Actor utilizing Hierarchical Instanced Static Mesh (HISM) components for optimized performance.
6. **HISM Reversion:** Easily revert HISM components back into individual static mesh actors.
7. **Material Assignment:** Batch assign materials to selected meshes based on naming conventions.

# Unreal Engine Setup
You can clone this repository with git and build it in the Unreal project from source or you can download a pre-built binary release.
## Cloning the repository
Source files will need to be placed in `{Project}/Plugins/MeshAssemblyKit/`
## Download Binary releases
Download the latest binary release as a zip file and extract it in either your `{Project}/Plugins/` or `{Engine}/Plugins/` directory
## Enable the Plugin
Open your Unreal Engine project and click `Edit → Plugins`.

Search for Mesh Assembly Kit and check the Enabled checkbox.

Restart the editor

Go to Window → Mesh Assembly Kit to open plugin window

# Blender Setup
Download `unreal_assembly_kit.zip` under Releases section

Open Blender. Edit → Preferences → Addons → Install from disk...

Choose file `unreal_assembly_kit.zip`

In the sidebar appears a new tab `Unreal Assembly Kit`

# Quick Start
Prepare your Blender scene by ensuring you have your source meshes and their corresponding linked instances.

https://github.com/user-attachments/assets/f047e570-a702-4936-bcff-7ab366dbacc6

Select your source meshes, check the Is Original box, and press Set Selection as Master. This marks them as the parent assets for your instances. You must also mark unique meshes that have no instances to ensure they are exported correctly.

https://github.com/user-attachments/assets/768a0e17-a117-4b59-b128-6f8e30a57d00

Next, select the meshes you want to assemble in Unreal, navigate to the Instances tab and click `Copy Instances JSON` to copy the instance data to your clipboard.

https://github.com/user-attachments/assets/63ad1bfc-c707-4b2f-98e3-fafe8cdd40e2

Import the exported FBX files into your Unreal Engine project. Then, press Assemble Scene; the tool will automatically spawn Static Mesh Actors in your level, applying their corresponding world transforms. It will try to find matched existing Static Meshes from your Content Browser by name.

https://github.com/user-attachments/assets/83deaa0f-8fc9-4075-9f03-142695c5e9a3

Use Convert Selection to HISM to merge selected actors into a Hierarchical Instanced Static Mesh for improved performance and reduced draw calls. You can easily undo this process and return to individual actors using Revert HISM to Static Meshes.

https://github.com/user-attachments/assets/326425d9-ce67-46b2-99be-afa14d3899ff

The Material Tool allows for batch material assignment by selecting your project's materials for the mesh's material slot names.

https://github.com/user-attachments/assets/c46bfddd-d4e0-4e57-b1b3-b7c181cceed2