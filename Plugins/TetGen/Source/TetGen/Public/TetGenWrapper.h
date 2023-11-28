#pragma once
#include "CoreMinimal.h"
#include "UnrealString.h"


struct TetGenParam
{
	FString max_size = "5e2";
	FString max_radius_edge_ration_bound = "2";
	FString min_dihedral_angle_bound = "0";
	FString file_path = "";
};

struct Tetrahedra
{
	int* pointList;
};

struct TetGenResult
{
	int numberOfPoints = 0;
	double* pointList;

	int numberOfTetrahedra = 0;
	Tetrahedra* TetrahedraList;
};


class TETGEN_API TetGenWrapper
{
public:
	static bool TetrahedralMeshGeneration(TetGenParam p, TetGenResult& res);

private:
	static char* Readline(char* string, FILE* infile, int* linenumber);
	static bool Read_vtk(FString file_path, TetGenResult& res);
};
