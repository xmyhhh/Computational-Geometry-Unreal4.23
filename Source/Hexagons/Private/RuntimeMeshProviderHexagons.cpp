// Copyright 2019-2021 Gabriel Zerbib (Moddingear). All Rights Reserved.


#include "RuntimeMeshProviderHexagons.h"
#include "TetGenWrapper.h"

void URuntimeMeshProviderHexagons::SetRenderData(FHexRenderData& InRenderData)
{
	FScopeLock Lock(&PropertySyncRoot);
	RenderData = InRenderData;
	MarkAllLODsDirty();
	MarkCollisionDirty();
}

FHexRenderData URuntimeMeshProviderHexagons::GetRenderData()
{
	FScopeLock Lock(&PropertySyncRoot);
	return RenderData;
}

int32 URuntimeMeshProviderHexagons::AddVertex(FRuntimeMeshRenderableMeshData& MeshData, FVector location, FColor color)
{
	int32 vertindex = MeshData.Positions.Add(location);
	MeshData.Tangents.Add(FVector(0, 0, 1), FVector(1, 0, 0));
	MeshData.TexCoords.Add(FVector2D(0, 0));
	MeshData.Colors.Add(color);
	return vertindex;
}

int32 URuntimeMeshProviderHexagons::AddVertexCollision(FRuntimeMeshCollisionData& CollisionData, FVector location)
{
	int32 vertindex = CollisionData.Vertices.Add(location);
	return vertindex;
}

void URuntimeMeshProviderHexagons::Initialize()
{
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;
	TArray<FRuntimeMeshLODProperties> LODs;
	LODs.Add(LODProperties);
	ConfigureLODs(LODs);

	SetupMaterialSlot(0, FName("Material"), RenderData.Material);

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Frequent;
	CreateSection(0, 0, Properties);
}


bool URuntimeMeshProviderHexagons::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId,
                                                        FRuntimeMeshRenderableMeshData& MeshData)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ProviderHexagon_GetSectionMesh);
	FDateTime RenderTime = FDateTime::UtcNow();
	FHexRenderData TempRenderData;
	{
		FScopeLock Lock(&PropertySyncRoot);
		TempRenderData = RenderData;
	}
	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);
	TetGenResult res;
	TetGenWrapper::TetrahedralMeshGeneration(RenderData.tetGenParam, res);
	int32 NumVertices = res.numberOfPoints;
	MeshData.ReserveVertices(NumVertices);

	for (uint8 i = 0; i < NumVertices; i++)
	{
		int32 index = AddVertex(
			MeshData, FVector(res.pointList[i * 3], res.pointList[i * 3 + 1], res.pointList[i * 3 + 2]),
			RenderData.pointColor);
	
	}

	//UE_LOG(LogTemp, Log, TEXT("Rendered %d sides out of %d total"), numRendered, ObstaclesToRender.Num())
	return true;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderHexagons::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = true;
	Settings.bUseComplexAsSimple = false;

	return Settings;
}

bool URuntimeMeshProviderHexagons::HasCollisionMesh()
{
	return false;
}

bool URuntimeMeshProviderHexagons::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	float RenderTime = UKismetSystemLibrary::GetGameTimeInSeconds(this);


	return true;
}

bool URuntimeMeshProviderHexagons::IsThreadSafe()
{
	return true;
}
