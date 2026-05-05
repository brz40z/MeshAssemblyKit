#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "MaterialMapper.h"

class FImportMeshModule : public IModuleInterface {
public:
/** IModuleInterface implementation */
virtual void StartupModule() override;
virtual void ShutdownModule() override;

TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs &SpawnTabArgs);

private:
void RegisterMenus();

TSharedPtr<class FUICommandList> PluginCommands;

TArray<TSharedPtr<FMaterialMappingRow>> MaterialRows;
TSharedPtr<SVerticalBox> MaterialListWidget;
void RefreshMaterialList();
};