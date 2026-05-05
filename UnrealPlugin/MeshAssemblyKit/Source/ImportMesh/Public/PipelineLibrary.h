#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PipelineLibrary.generated.h"

UCLASS()
class IMPORTMESH_API UPipelineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static void AssembleScene(FString RawData);

	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static FString GetClipboardText();

	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static FSceneData ParseAssetEntryFromJson(const FString& JsonString);

	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static FString SerializeAssetEntryToJson(const FSceneData& SceneData);

	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static UStaticMesh* FindMeshInProject(const FString& MeshName);

	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static void ConvertSelectionToHISM();

	UFUNCTION(BlueprintCallable, Category = "Mesh Assembly Kit")
	static void ConvertHISMToStaticMeshActors();
};