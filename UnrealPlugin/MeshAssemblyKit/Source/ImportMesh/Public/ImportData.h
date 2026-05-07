// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"

#include "ImportData.Generated.h"

USTRUCT(BlueprintType)
struct FAssetEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FString source_asset;
    UPROPERTY()
    FString instance_id;
    UPROPERTY()
    FString parent_name;
    UPROPERTY()
    FTransform transform;
};

USTRUCT(BlueprintType)
struct FSceneData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FAssetEntry> assets;
};