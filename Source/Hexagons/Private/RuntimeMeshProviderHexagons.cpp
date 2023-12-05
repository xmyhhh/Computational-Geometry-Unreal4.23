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

	if (TempRenderData.file_path.IsEmpty())
		return false;

	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);
	TetGenResult* res;
	if (false/*use_file*/)

	{
		TetGenWrapper::TetrahedralMeshGeneration(TempRenderData.file_path, TempRenderData.tetGenParam, res);
	}
	else
	{
		//example code of load off file to TetGenInputPLC
		{
			FILE* fp;
			fp = fopen(TCHAR_TO_ANSI(*TempRenderData.file_path), "r");
			if (!(fp))
			{
				printf("  Unable to open file %s\n", TCHAR_TO_ANSI(*TempRenderData.file_path));
				return false;
			}

			auto readline = [](char* string, FILE* infile, int* linenumber)
			{
				char* result;

				// Search for a non-empty line.
				do
				{
					result = fgets(string, 2048 - 1, infile);
					if (linenumber) (*linenumber)++;
					if (result == (char*)NULL)
					{
						return (char*)NULL;
					}
					// Skip white spaces.
					while ((*result == ' ') || (*result == '\t')) result++;
					// If it's end of line, read another line and try again.
				}
				while ((*result == '\0') || (*result == '\r') || (*result == '\n'));
				return result;
			};
			auto findnextnumber = [](char* string)
			{
				char* result;

				result = string;
				// Skip the current field.  Stop upon reaching whitespace or a comma.
				while ((*result != '\0') && (*result != '#') && (*result != ' ') &&
					(*result != '\t') && (*result != ','))
				{
					result++;
				}
				// Now skip the whitespace and anything else that doesn't look like a
				//   number, a comment, or the end of a line. 
				while ((*result != '\0') && (*result != '#')
					&& (*result != '.') && (*result != '+') && (*result != '-')
					&& ((*result < '0') || (*result > '9')))
				{
					result++;
				}
				// Check for a comment (prefixed with `#').
				if (*result == '#')
				{
					*result = '\0';
				}
				return result;
			};
			
			char buffer[2048];
			char* bufferp;
			int line_count = 0;
			int nverts = 0, iverts = 0;
			int nfaces = 0, ifaces = 0;
			int nedges = 0;
			int smallestidx;
			{
				while ((bufferp = readline(buffer, fp, &line_count)) != NULL)
				{
					// Check section
					if (nverts == 0)
					{
						// Read header 
						bufferp = strstr(bufferp, "OFF");
						if (bufferp != NULL)
						{
							// Read mesh counts
							bufferp = findnextnumber(bufferp); // Skip field "OFF".
							if (*bufferp == '\0')
							{
								// Read a non-empty line.
								bufferp = readline(buffer, fp, &line_count);
							}
							if ((sscanf(bufferp, "%d%d%d", &nverts, &nfaces, &nedges) != 3)
								|| (nverts == 0))
							{

								fclose(fp);
								return false;
							}
							// Allocate memory for 'tetgenio'
							if (nverts > 0)
							{
								TempRenderData.input.numberOfPoints = nverts;
								TempRenderData.input.pointList = new double[nverts * 3];
								smallestidx = nverts + 1; // A bigger enough number.
							}
							if (nfaces > 0)
							{
								TempRenderData.input.numberOfFace = nfaces;
								TempRenderData.input.faceList = new Polygon[nfaces];
							}
						}
					}
					else if (iverts < nverts)
					{
						// Read vertex coordinates
						double*  coord = &TempRenderData.input.pointList[iverts * 3];
						for (int i = 0; i < 3; i++)
						{
							if (*bufferp == '\0')
							{
								fclose(fp);
								return false;
							}
							coord[i] = (double)strtod(bufferp, &bufferp);
							bufferp = findnextnumber(bufferp);
						}
						iverts++;
					}
					else if (ifaces < nfaces)
					{
						// Get next face
						auto f = &TempRenderData.input.faceList[ifaces];
						
						// In .off format, each facet has one polygon, no hole.
				
						f->pointList = new int[3];
						auto p = &f->pointList[0];
						// Read the number of vertices, it should be greater than 0.
						f->numberOfPoints = (int)strtol(bufferp, &bufferp, 0);
			
						if (f->numberOfPoints !=3)
						{
							fclose(fp);
							return false;
						}
						// Allocate memory for face vertices
					
						for (int i = 0; i < 3; i++)
						{
							bufferp = findnextnumber(bufferp);
							if (*bufferp == '\0')
							{
			
								fclose(fp);
								return false;
							}
							p[i] = (int)strtol(bufferp, &bufferp, 0);
							// Detect the smallest index.
							if (p[i] < smallestidx)
							{
								smallestidx = p[i];
							}
						}
						ifaces++;
					}
					else
					{
						// Should never get here

						break;
					}
				}

				fclose(fp);
			}
		}

		TetGenWrapper::TetrahedralMeshGeneration(TempRenderData.input, TempRenderData.tetGenParam, res);
	}

	MeshData.ReserveVertices(res->numberOfPoints);
	MeshData.Triangles.Reserve(res->numberOfTetrahedra * 4);
	for (size_t i = 0; i < res->numberOfPoints; i++)
	{
		AddVertex(MeshData, FVector(res->pointList[i * 3], res->pointList[i * 3 + 1], res->pointList[i * 3 + 2]),
		          TempRenderData.pointColor);
	}
	for (size_t i = 0; i < res->numberOfTetrahedra; i++)
	{
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[0]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[1]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[2]);

		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[0]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[2]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[3]);

		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[0]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[1]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[3]);

		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[1]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[2]);
		MeshData.Triangles.Add(res->tetrahedraList[i].pointList[3]);
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
