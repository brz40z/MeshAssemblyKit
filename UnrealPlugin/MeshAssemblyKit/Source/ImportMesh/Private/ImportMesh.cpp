#include "ImportMesh.h"
#include "PipelineLibrary.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"

static const FName ImportMeshTabName("ImportMesh");

void FImportMeshModule::StartupModule()
{
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(ImportMeshTabName, FOnSpawnTab::CreateRaw(this, &FImportMeshModule::OnSpawnPluginTab))
		.SetDisplayName(FText::FromString("Mesh Assembly Kit"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

TSharedRef<SDockTab> FImportMeshModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
			[SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(10)
					[SNew(SVerticalBox)

						// --- SECTION 1: IMPORT TOOLS ---
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
							[SNew(SBorder)
									.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
									.Padding(FMargin(10, 5))
										[SNew(STextBlock)
												.Text(FText::FromString("IMPORT TOOLS"))
												.Font(FAppStyle::GetFontStyle("BoldFont"))]]
						+ SVerticalBox::Slot().AutoHeight().Padding(5, 0, 5, 10)
							[SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
									[SNew(SButton).ContentPadding(FMargin(10, 5)).OnClicked_Lambda([]() {
										UPipelineLibrary::AssembleScene(UPipelineLibrary::GetClipboardText());
										return FReply::Handled();
									})[SNew(STextBlock).Text(FText::FromString("Assemble Scene")).Justification(ETextJustify::Center)]]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
									[SNew(SButton).ContentPadding(FMargin(10, 5)).OnClicked_Lambda([]() {
										MaterialMapper::FastReimportSelectedAssets();
										return FReply::Handled();
									})[SNew(STextBlock).Text(FText::FromString("Reimport Selected Assets")).Justification(ETextJustify::Center)]]
								// --- NEW BUTTON: HISM CONVERSION ---
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
									[SNew(SButton).ContentPadding(FMargin(10, 5)).OnClicked_Lambda([]() {
										UPipelineLibrary::ConvertSelectionToHISM();
										return FReply::Handled();
									})[SNew(STextBlock).Text(FText::FromString("Convert Selection to HISM")).Justification(ETextJustify::Center)]]
								// --- NEW BUTTON: HISM REVERSION ---
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
									[SNew(SButton).ContentPadding(FMargin(10, 5)).OnClicked_Lambda([]() {
										UPipelineLibrary::ConvertHISMToStaticMeshActors();
										return FReply::Handled();
									})[SNew(STextBlock).Text(FText::FromString("Revert HISM to Static Meshes")).Justification(ETextJustify::Center)]]]

						+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)[SNew(SSeparator)]

						// --- SECTION 2: MATERIAL MAPPER ---
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
							[SNew(SBorder)
									.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
									.Padding(FMargin(10, 5))
										[SNew(STextBlock)
												.Text(FText::FromString("MATERIAL MAPPER"))
												.Font(FAppStyle::GetFontStyle("BoldFont"))]]
						+ SVerticalBox::Slot().AutoHeight().Padding(5, 0, 5, 5)
							[SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 5, 0)
									[SNew(SButton).ContentPadding(5).Text(FText::FromString("Get Materials")).OnClicked_Lambda([this]() { RefreshMaterialList(); return FReply::Handled(); })]
								+ SHorizontalBox::Slot().FillWidth(1.0f)
									[SNew(SButton).ContentPadding(5).Text(FText::FromString("Assign Materials")).OnClicked_Lambda([this]() {
										MaterialMapper::ApplyMaterialsToAssets(MaterialRows);
										return FReply::Handled();
									})]]
						+ SVerticalBox::Slot().AutoHeight().Padding(5, 5, 5, 10)
							[SNew(SBorder)
									.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
										[SAssignNew(MaterialListWidget, SVerticalBox)]]]];
}

void FImportMeshModule::RefreshMaterialList()
{
	if (!MaterialListWidget.IsValid())
		return;

	MaterialRows.Empty();
	MaterialListWidget->ClearChildren();

	TArray<FString> Names = MaterialMapper::GetUniqueMaterialNamesFromContentBrowser();

	if (Names.Num() > 0)
	{
		// Add Header
		MaterialListWidget->AddSlot().Padding(5)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.4f)[SNew(STextBlock).Text(FText::FromString("Slot Name")).Font(FAppStyle::GetFontStyle("BoldFont"))]
				+ SHorizontalBox::Slot().FillWidth(0.6f)[SNew(STextBlock).Text(FText::FromString("Material")).Font(FAppStyle::GetFontStyle("BoldFont"))]];

		// Add Rows
		for (const FString& MatName : Names)
		{
			TSharedPtr<FMaterialMappingRow> NewRow = MakeShared<FMaterialMappingRow>(MatName);
			MaterialRows.Add(NewRow);

			MaterialListWidget->AddSlot().AutoHeight().Padding(2, 5)
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(MatName))]

					+ SHorizontalBox::Slot().FillWidth(0.6f)
						[SNew(SObjectPropertyEntryBox)
								.AllowedClass(UMaterialInterface::StaticClass())
								.ObjectPath_Lambda([NewRow]() {
									return NewRow->SelectedMaterial ? NewRow->SelectedMaterial->GetPathName() : TEXT("");
								})
								.OnObjectChanged_Lambda([NewRow](const FAssetData& Data) {
									NewRow->SelectedMaterial = Cast<UMaterialInterface>(Data.GetAsset());
								})]];
		}
	}
}

void FImportMeshModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ImportMeshTabName);
}

IMPLEMENT_MODULE(FImportMeshModule, ImportMesh)