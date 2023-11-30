// Copyright 2019-2021 Gabriel Zerbib (Moddingear). All Rights Reserved.


#include "RuntimeMeshProviderHexagons.h"


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
	Properties.bWants32BitIndices = true;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
	CreateSection(0, 0, Properties);
}

FBoxSphereBounds URuntimeMeshProviderHexagons::GetBounds()
{
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 7000));
}


bool URuntimeMeshProviderHexagons::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId,
                                                        FRuntimeMeshRenderableMeshData& MeshData)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ProviderHexagon_GetSectionMesh);
	
	FHexRenderData TempRenderData;
	{
		FScopeLock Lock(&PropertySyncRoot);
		TempRenderData = RenderData;
	}

	if (TempRenderData.tetGenParam.file_path.IsEmpty())
		return false;
	
	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);
	TetGenResult *res;
	TetGenWrapper::TetrahedralMeshGeneration(TempRenderData.tetGenParam, res);

	MeshData.ReserveVertices(res->numberOfPoints);
	MeshData.Triangles.Reserve(res->numberOfTetrahedra * 4);
	for (size_t i = 0; i < res->numberOfPoints; i++)
	{
		AddVertex(MeshData, FVector(res->pointList[i * 3], res->pointList[i * 3 + 1], res->pointList[i * 3 + 2]),
		          TempRenderData.pointColor);
	}
	for (size_t i = 0; i <res->numberOfTetrahedra; i++)
	{
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[0]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[1]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[2]);

		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[0]);
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[2]);
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[3]);
		//
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[0]);
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[1]);
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[3]);
		//
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[1]);
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[2]);
		// MeshData.Triangles.Add(res->tetrahedraList[i].pointList[3]);
	}

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
	//float RenderTime = UKismetSystemLibrary::GetGameTimeInSeconds(this);
	return true;
}

bool URuntimeMeshProviderHexagons::IsThreadSafe()
{
	return true;
}
