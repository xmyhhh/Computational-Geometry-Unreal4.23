#pragma once

#include "CoreMinimal.h"
#include "TetGenWrapper.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Structs.generated.h"


USTRUCT(BlueprintType)
struct FHexRenderData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FColor pointColor;

	UPROPERTY(BlueprintReadWrite)
	FColor edgeColor;

	TetGenParam tetGenParam;

	UPROPERTY(BlueprintReadWrite)
	UMaterialInterface* Material;
};
