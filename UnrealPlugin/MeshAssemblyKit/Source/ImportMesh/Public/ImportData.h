// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"

#include "ImportData.Generated.h"

USTRUCT(BlueprintType)
struct FAssetEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString source_asset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString instance_id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString parent_name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform transform;
};

USTRUCT(BlueprintType)
struct FSceneData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FAssetEntry> assets;
};