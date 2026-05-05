#include "MaterialMapper.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/StaticMesh.h"

TArray<FString> MaterialMapper::GetUniqueMaterialNamesFromContentBrowser()
{
	TSet<FString> UniqueNames;

	// 1. Get the Content Browser selection
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		// 2. Only process Static Meshes
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(AssetData.GetAsset()))
		{
			for (const FStaticMaterial& Mat : Mesh->GetStaticMaterials())
			{
				UniqueNames.Add(Mat.ImportedMaterialSlotName.ToString());
			}
		}
	}
	return UniqueNames.Array();
}

void MaterialMapper::ApplyMaterialsToAssets(const TArray<TSharedPtr<FMaterialMappingRow>>& Mapping)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData>	   SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	int32 TotalMeshes = SelectedAssets.Num();
	if (TotalMeshes == 0)
		return;

	// 1. Initialize the Progress Bar
	FScopedSlowTask Progress(TotalMeshes, FText::FromString("Assigning Materials..."));
	Progress.MakeDialog();

	for (const FAssetData& AssetData : SelectedAssets)
	{
		// 2. Update the progress frame
		Progress.EnterProgressFrame(1.f, FText::Format(FText::FromString("Processing: {0}"), FText::FromName(AssetData.AssetName)));

		if (UStaticMesh* Mesh = Cast<UStaticMesh>(AssetData.GetAsset()))
		{
			bool bModified = false;
			for (auto& Row : Mapping)
			{
				if (!Row->SelectedMaterial)
					continue;

				for (int32 i = 0; i < Mesh->GetStaticMaterials().Num(); i++)
				{
					// Check if the slot name matches the Blender name
					if (Mesh->GetStaticMaterials()[i].ImportedMaterialSlotName.ToString() == Row->BlenderMaterialName)
					{
						// Only update if the material is actually different to save time
						if (Mesh->GetMaterial(i) != Row->SelectedMaterial)
						{
							Mesh->SetMaterial(i, Row->SelectedMaterial);
							bModified = true;
						}
					}
				}
			}

			if (bModified)
			{
				Mesh->MarkPackageDirty();
			}
		}
	}
}

void MaterialMapper::FastReimportSelectedAssets()
{
	// 1. Get Selected Assets
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData>	   SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.Num() == 0)
		return;

	// 2. Setup Progress Bar
	FScopedSlowTask Progress(SelectedAssets.Num(), FText::FromString("Fast Reimporting Assets..."));
	Progress.MakeDialog();

	for (const FAssetData& AssetData : SelectedAssets)
	{
		Progress.EnterProgressFrame(1.f, FText::Format(FText::FromString("Reimporting: {0}"), FText::FromName(AssetData.AssetName)));

		UObject* Asset = AssetData.GetAsset();
		if (Asset)
		{
			// 3. Trigger the reimport without opening the "Import Options" dialog for every file
			// Parameters: (Object, bAskForNewFile, bShowMessages, SourceFilename, ReimportHandler, SlotIndex)
			FReimportManager::Instance()->Reimport(Asset, false, false);
		}
	}

	// 4. Force a single UI refresh at the very end
	FAssetRegistryModule::AssetCreated(nullptr);
}
