using UnrealBuildTool;

public class ImportMesh : ModuleRules

{

        public ImportMesh(ReadOnlyTargetRules Target) : base(Target)

        {

                PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

                PublicDependencyModuleNames.AddRange(new string[]
                {       "Core",
                        "CoreUObject",
                        "Engine",
                        "InputCore",
                        "Slate",
                        "SlateCore",
                        "EditorStyle",
                        "UnrealEd",
                        "LevelEditor",
                        "ApplicationCore",
                        "Json",
                        "JsonUtilities",
                        "AssetRegistry",
                        "ContentBrowser",
                        "PropertyEditor",
                        "EditorWidgets",
                        "EditorFramework",
                        "Projects" });
        }
}