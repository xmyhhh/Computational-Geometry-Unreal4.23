// Fill out your copyright notice in the Description page of Project Settings.


#include "TetgenRenderingComponent.h"
#include "UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

FDateTime UTetgenRenderingComponent::GetGameStartTime(UObject* WorldContextObject)
{
	const float GameTime = UKismetSystemLibrary::GetGameTimeInSeconds(WorldContextObject);
	const FDateTime now = FDateTime::UtcNow();
	const FDateTime GameStart = now - FTimespan::FromSeconds(GameTime);
	return GameStart;
}

UTetgenRenderingComponent::UTetgenRenderingComponent(const FObjectInitializer& ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("Material'/Game/MeshMaterial.MeshMaterial'"));
	if (MaterialFinder.Succeeded())
	{
		RenderData.Material = MaterialFinder.Object;
	}
	bReplicates=true;
}

void UTetgenRenderingComponent::OnRegister()
{
	Super::OnRegister();
	
	SetIsReplicated(true);

	RenderData.edgeColor = FColor(0, 244, 10);
	RenderData.pointColor = FColor(186, 0, 0);
	RenderData.tetGenParam.max_size = "5e2";
	RenderData.tetGenParam.file_path = "C:/Users/xmyci/Desktop/tetgen1.6.0/build/4m_fix.off";
	if (GetWorld()->IsServer())
	{
	}
}

void UTetgenRenderingComponent::BeginPlay()
{
	Super::BeginPlay();
	HexagonProvider = NewObject<URuntimeMeshProviderHexagons>(this);
	HexagonProvider->SetRenderData(RenderData);

	GetOrCreateRuntimeMesh()->Initialize(HexagonProvider);
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionProfileName("BlockAll");
}

void UTetgenRenderingComponent::UpdateMesh()
{
	//TODO
	HexagonProvider->SetRenderData(RenderData);
}
