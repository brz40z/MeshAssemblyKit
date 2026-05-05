#pragma once
#include "CoreMinimal.h"
#include "EditorReimportHandler.h"
#include "Misc/ScopedSlowTask.h"
#include "PropertyCustomizationHelpers.h"

struct FMaterialMappingRow
{
    FString BlenderMaterialName;
    UMaterialInterface* SelectedMaterial;

    FMaterialMappingRow(FString InName) : BlenderMaterialName(InName), SelectedMaterial(nullptr) {}
};

class MaterialMapper
{
public:
    static TArray<FString> GetUniqueMaterialNamesFromContentBrowser();
    static void ApplyMaterialsToAssets(const TArray<TSharedPtr<FMaterialMappingRow>>& Mapping);
    static void FastReimportSelectedAssets();
};