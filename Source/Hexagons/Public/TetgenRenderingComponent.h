// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshComponent.h"
#include "Structs.h"
#include "RuntimeMeshProviderHexagons.h"
#include "TetgenRenderingComponent.generated.h"




/**
 * This class is blueprint facing and handles the data, that is replication and removing walls that are no longer needed because they are past the center
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UTetgenRenderingComponent : public URuntimeMeshComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Utilities|Time", meta=(WorldContext="WorldContextObject"))
	static FDateTime GetGameStartTime(UObject* WorldContextObject);

	UPROPERTY(SkipSerialization)
	URuntimeMeshProviderHexagons* HexagonProvider;

	UTetgenRenderingComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnRegister() override;

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, SkipSerialization)
	FHexRenderData RenderData; //Cache for data to be rendered, that is then passed to the provider

	UFUNCTION(BlueprintCallable)
	void UpdateMesh(FHexRenderData data); //Function called when the source data is new and the mesh needs to be updated

	UFUNCTION(BlueprintCallable)
	ETetGenStage GetTetGenStage();

	UFUNCTION(BlueprintCallable)
	FTetGenResult_BP GetTetGenStatistic();
};
