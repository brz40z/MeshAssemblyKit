#include "PipelineLibrary.h"
#include "Editor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "JsonObjectConverter.h"
#include "Engine/StaticMeshActor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Folder.h"
#include "ImportData.h"
#include "EditorActorFolders.h"
#include "EngineUtils.h"
#include "Modules/ModuleManager.h"

void UPipelineLibrary::AssembleScene(FString RawData)
{
	FMessageLog ImportLog("EditorErrors");
	FString		ClipboardText = RawData;
	FSceneData	SceneData = ParseAssetEntryFromJson(ClipboardText);

	int32 TotalAssets = SceneData.assets.Num();
	if (TotalAssets == 0)
	{
		ImportLog.Error(FText::FromString("JSON Parsing failed. No assets found."));
		ImportLog.Open(EMessageSeverity::Error);
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
		return;

	// 1. Initialize the Progress Bar
	// Parameters: Total amount of work (TotalAssets), Display Message, Show as Dialog
	FScopedSlowTask Progress(TotalAssets, FText::FromString("Assembling Blender Scene..."));
	Progress.MakeDialog(); // This makes the popup window appear

	TArray<AActor*> CreatedActors;
	int32			SuccessCount = 0;

	for (const FAssetEntry& Asset : SceneData.assets)
	{
		// 2. Update Progress
		// Move the bar forward by 1 unit and update the text
		Progress.EnterProgressFrame(1.f, FText::Format(FText::FromString("Spawning: {0}"), FText::FromString(Asset.instance_id)));

		UStaticMesh* StaticMesh = FindMeshInProject(Asset.source_asset);

		if (!StaticMesh)
		{
			ImportLog.Warning(FText::Format(FText::FromString("Asset '{0}' not found. Using fallback cube."), FText::FromString(Asset.source_asset)));
			StaticMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		}

		AStaticMeshActor* NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Asset.transform);
		if (NewActor)
		{
			NewActor->SetActorLabel(Asset.instance_id);
			NewActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			CreatedActors.Add(NewActor);

			if (!Asset.parent_name.IsEmpty())
			{
				NewActor->SetFolderPath(FName(*Asset.parent_name));
			}
			SuccessCount++;
		}
		else
		{
			ImportLog.Error(FText::Format(FText::FromString("Failed to spawn actor for: {0}"), FText::FromString(Asset.instance_id)));
		}
	}

	// 3. Final Logging
	ImportLog.Info(FText::Format(FText::FromString("Assembled {0} of {1} assets successfully."), SuccessCount, TotalAssets));
	ImportLog.Open(EMessageSeverity::Info);

	if (CreatedActors.Num() > 0)
	{
		// 1. Clear the current selection
		GEditor->SelectNone(true, true, false);

		// 2. Select the new actors
		for (AActor* Actor : CreatedActors)
		{
			GEditor->SelectActor(Actor, true, false);
		}

		// 3. Update the editor UI to show the selection
		GEditor->NoteSelectionChange();
	}
}

FString UPipelineLibrary::GetClipboardText()
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);
	return ClipboardText;
}

FSceneData UPipelineLibrary::ParseAssetEntryFromJson(const FString& JsonString)
{
	FSceneData SceneData;
	bool	   bSuccess = FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &SceneData, 0, 0);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully parsed JSON string to FSceneData struct. Number of assets: %d"), SceneData.assets.Num());
		return SceneData;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON string to FSceneData struct."));
		return FSceneData();
	}
}

FString UPipelineLibrary::SerializeAssetEntryToJson(const FSceneData& SceneData)
{
	FString JsonString;
	bool	bSuccess = FJsonObjectConverter::UStructToJsonObjectString(SceneData, JsonString);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully serialized FSceneData to JSON string: %s"), *JsonString);
		return JsonString;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to serialize FSceneData struct to JSON string."));
		return FString();
	}
}

UStaticMesh* UPipelineLibrary::FindMeshInProject(const FString& MeshName)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry&		  AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> FoundAssets;
	FString			   CleanMeshName = MeshName.TrimStartAndEnd();

	// 1. Gather ALL potential matches
	FARFilter Filter;
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AllMeshes;
	AssetRegistry.GetAssets(Filter, AllMeshes);

	for (const FAssetData& Data : AllMeshes)
	{
		bool bIsMatch = false;

		// Check Asset Name
		if (Data.AssetName.ToString().Equals(CleanMeshName, ESearchCase::IgnoreCase))
		{
			bIsMatch = true;
		}
		else
		{
			// Check Tags (Original Import Names, etc.)
			for (auto TagIt = Data.TagsAndValues.CreateConstIterator(); TagIt; ++TagIt)
			{
				if (TagIt.Value().AsString().Equals(CleanMeshName, ESearchCase::IgnoreCase))
				{
					bIsMatch = true;
					break;
				}
			}
		}

		if (bIsMatch)
		{
			FoundAssets.Add(Data);
		}
	}

	// 2. Handle results
	if (FoundAssets.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Mesh '%s' not found in project. Loading fallback engine cube."), *CleanMeshName);

		// Try to load the engine's default cube
		UStaticMesh* FallbackCube = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));

		// If for some reason the BasicShapes cube isn't there, try the very basic engine mesh
		if (!FallbackCube)
		{
			FallbackCube = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/EngineMeshes/Cube.Cube")));
		}

		return FallbackCube;
	}

	// --- NEW: DUPLICATE WARNING LOGIC ---
	if (FoundAssets.Num() > 1)
	{
		FString DuplicatePaths = "";
		for (const FAssetData& Asset : FoundAssets)
		{
			DuplicatePaths += FString::Printf(TEXT("\n   -> %s"), *Asset.PackagePath.ToString());
		}

		// Detailed log for the console
		UE_LOG(LogTemp, Warning, TEXT("Multiple meshes found for '%s'. Count: %d. Found at:%s"),
			*CleanMeshName, FoundAssets.Num(), *DuplicatePaths);

		// Visual Toast Notification
		FNotificationInfo Info(FText::Format(
			FText::FromString("WARNING: Found {0} versions of {1}! Using first match."),
			FText::AsNumber(FoundAssets.Num()),
			FText::FromString(CleanMeshName)));
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.WarningWithColor");

		FSlateNotificationManager::Get().AddNotification(Info);
	}

	// 3. Selection Bias: Prefer the mesh in the folder you are currently looking at
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString>		   SelectedFolders;
	ContentBrowserModule.Get().GetSelectedFolders(SelectedFolders);

	if (SelectedFolders.Num() > 0)
	{
		FString ActiveFolder = SelectedFolders[0];
		for (const FAssetData& Asset : FoundAssets)
		{
			if (Asset.PackagePath.ToString().StartsWith(ActiveFolder))
			{
				return Cast<UStaticMesh>(Asset.GetAsset());
			}
		}
	}

	// 4. Default: Return the first one found in the registry list
	return Cast<UStaticMesh>(FoundAssets[0].GetAsset());
}

void UPipelineLibrary::ConvertSelectionToHISM()
{
	FMessageLog ConversionLog("EditorErrors");
	TArray<AActor*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

	if (SelectedActors.Num() < 1) return;

	UStaticMesh* TargetMesh = nullptr;
	TArray<AStaticMeshActor*> SourceActors;
	FVector	CenterLocation = FVector::ZeroVector;

	// 1. Validate selection and calculate center
	for (AActor* Actor : SelectedActors)
	{
		if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Actor))
		{
			UStaticMesh* Mesh = SMA->GetStaticMeshComponent()->GetStaticMesh();
			if (!TargetMesh)
				TargetMesh = Mesh;

			if (Mesh == TargetMesh)
			{
				SourceActors.Add(SMA);
				CenterLocation += SMA->GetActorLocation();
			}
			else
			{
				ConversionLog.Error(FText::Format(FText::FromString("Mesh Mismatch: Actor '{0}' uses '{1}', but the target is '{2}'."),
					FText::FromString(SMA->GetActorLabel()),
					FText::FromString(Mesh->GetName()),
					FText::FromString(TargetMesh->GetName())));
				ConversionLog.Open(EMessageSeverity::Error);
				return;
			}
		}
	}

	if (SourceActors.Num() == 0)
		return;

	// Calculate final average center
	CenterLocation /= (float)SourceActors.Num();

	const FScopedTransaction Transaction(FText::FromString("Convert to HISM"));

	// Track folders to check for emptiness later
	TSet<FName> FoldersToProcess;
	for (AStaticMeshActor* SMA : SourceActors)
	{
		if (!SMA->GetFolderPath().IsNone())
		{
			FoldersToProcess.Add(SMA->GetFolderPath());
		}
	}

	UWorld* World = SourceActors[0]->GetWorld();

	// 2. Spawn HISM Actor at the calculated center
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Let the engine generate a unique internal name to avoid collisions that can crash name generation.
	AActor* NewActor = World->SpawnActor<AActor>(AActor::StaticClass(), CenterLocation, FRotator::ZeroRotator, SpawnParams);
	if (!NewActor)
		return;

	NewActor->SetActorLabel(FString::Printf(TEXT("HISM_%s"), *TargetMesh->GetName()));

	// Create a simple scene root at the desired world location so component transforms are relative to it
	USceneComponent* RootComp = NewObject<USceneComponent>(NewActor, TEXT("HISM_Root"), RF_Transactional);
	RootComp->SetWorldLocation(CenterLocation);
	NewActor->AddInstanceComponent(RootComp);
	RootComp->RegisterComponent();
	NewActor->SetRootComponent(RootComp);

	// Create the HISM component, attach to the root, then register it with the world
	UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(NewActor, NAME_None, RF_Transactional);
	HISM->SetStaticMesh(TargetMesh);
	NewActor->AddInstanceComponent(HISM);
	HISM->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
	HISM->RegisterComponentWithWorld(World);
	HISM->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);


	// 3. Add instances using WORLD transforms so instances keep their world positions
	for (AStaticMeshActor* OldActor : SourceActors)
	{
		if (!OldActor)
			continue;
		OldActor->Modify();
		FTransform WorldTransform = OldActor->GetActorTransform();
		HISM->AddInstance(WorldTransform, true); // true = transform is in world space
		OldActor->Destroy();
	}

	// 4. Clean up empty folders
	if (FoldersToProcess.Num() > 0)
	{
		// Remove folders that still have actors
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			FoldersToProcess.Remove(It->GetFolderPath());
		}

		// Delete any folders that are now empty via the EditorActorFolders module
		if (FoldersToProcess.Num() > 0)
		{
			for (const FName& FolderName : FoldersToProcess)
			{
				if (FolderName.IsNone())
					continue;
				// Build a FFolder for the world root and delete via FActorFolders
				FFolder WorldRootFolder = FFolder::GetWorldRootFolder(World);
				FFolder ToDelete(WorldRootFolder.GetRootObject(), FolderName);
				FActorFolders::Get().DeleteFolder(*World, ToDelete);
			}
		}
	}

	// 5. Finalize Selection
	if (NewActor)
	{
		GEditor->SelectNone(true, true, false);
		GEditor->SelectActor(NewActor, true, true);
		GEditor->NoteSelectionChange();
	}
}

void UPipelineLibrary::ConvertHISMToStaticMeshActors()
{
	FMessageLog		RevertLog("EditorErrors");
	TArray<AActor*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

	if (SelectedActors.Num() < 1)
		return;

	bool					 bShowLog = false;
	const FScopedTransaction Transaction(FText::FromString("Revert HISM to Static Mesh Actors"));

	TArray<AActor*> NewActors;

	for (AActor* Actor : SelectedActors)
	{
		if (!Actor)
			continue;

		TArray<UHierarchicalInstancedStaticMeshComponent*> HISMComponents;
		Actor->GetComponents<UHierarchicalInstancedStaticMeshComponent>(HISMComponents);

		if (HISMComponents.Num() == 0)
		{
			RevertLog.Warning(FText::Format(FText::FromString("Actor '{0}' does not contain HISM components and cannot be reverted."), FText::FromString(Actor->GetActorLabel())));
			bShowLog = true;
			continue;
		}

		const FName ParentFolderPath = Actor->GetFolderPath();
		FString		FolderName = Actor->GetActorLabel();

		if (FolderName.StartsWith(TEXT("HISM_")))
		{
			FolderName.RightChopInline(5);
		}

		FString NewFolderPathString = FolderName;
		if (!ParentFolderPath.IsNone())
		{
			NewFolderPathString = FString::Printf(TEXT("%s/%s"), *ParentFolderPath.ToString(), *FolderName);
		}
		const FName NewFolderPath = FName(*NewFolderPathString);

		UWorld* World = Actor->GetWorld();

		for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMComponents)
		{
			if (!HISM || !HISM->GetStaticMesh())
				continue;

			UStaticMesh* Mesh = HISM->GetStaticMesh();
			int32		 InstanceCount = HISM->GetInstanceCount();

			for (int32 i = 0; i < InstanceCount; ++i)
			{
				FTransform InstanceTransform;
				if (HISM->GetInstanceTransform(i, InstanceTransform, true))
				{
					AStaticMeshActor* NewSMA = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), InstanceTransform);
					if (NewSMA)
					{
						NewSMA->GetStaticMeshComponent()->SetStaticMesh(Mesh);
						NewSMA->SetActorLabel(FString::Printf(TEXT("%s_%d"), *Mesh->GetName(), i));
						NewSMA->SetFolderPath(NewFolderPath);
						NewActors.Add(NewSMA);
					}
				}
			}
		}

		Actor->Destroy();
	}

	if (bShowLog)
	{
		RevertLog.Open(EMessageSeverity::Warning);
	}

	if (NewActors.Num() > 0)
	{
		GEditor->SelectNone(true, true, false);
		for (AActor* NewActor : NewActors)
		{
			GEditor->SelectActor(NewActor, true, false);
		}
		GEditor->NoteSelectionChange();
	}
}