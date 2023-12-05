#pragma once
#include "CoreMinimal.h"
#include "UnrealString.h"
#include "TetGenWrapper.generated.h"

USTRUCT(BlueprintType)
struct FTetGenParam
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	FString max_size = "1e8";
	UPROPERTY(BlueprintReadWrite)
	FString max_radius_edge_ration_bound = "2";
	UPROPERTY(BlueprintReadWrite)
	FString min_dihedral_angle_bound = "0";
};

struct Tetrahedra
{
	int* pointList;
};

struct Polygon
{
	int numberOfPoints = 3; //3表示三角形
	int* pointList;
};

struct TetGenInputPLC
{
	int numberOfPoints = 0;
	double* pointList;

	int numberOfFace = 0;
	Polygon* faceList;
};

struct TetGenResult
{
	int numberOfPoints = 0;
	double* pointList;

	int numberOfTetrahedra = 0;
	Tetrahedra* tetrahedraList;
};


USTRUCT(BlueprintType)
struct FTetGenResult_BP
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	int numberOfPoints = 0;
	UPROPERTY(BlueprintReadWrite)
	int numberOfTetrahedra = 0;
};

UENUM(BlueprintType)
enum class ETetGenStage : uint8
{
	Stage_Begin UMETA(DisplayName="Begin"),
	Stage_LoadGeometry UMETA(DisplayName="Load"),
	Stage_Calculate_0 UMETA(DisplayName="Calculate 0"),
	Stage_Calculate_1 UMETA(DisplayName="Calculate 1"),
	Stage_Calculate_2 UMETA(DisplayName="Calculate 2"),
	Stage_Calculate_3 UMETA(DisplayName="Calculate 3"),
	Stage_Calculate_4 UMETA(DisplayName="Calculate 4"),
	Stage_Calculate_5 UMETA(DisplayName="Calculate 5"),
	Stage_WriteGeometry UMETA(DisplayName="Write"),
	Stage_End UMETA(DisplayName="End"),
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTetGenStageChangedSignature, ETetGenStage);


class TETGEN_API TetGenWrapper
{
public:
	static ETetGenStage GetTetGenStage();
	static const TetGenResult* GetTetGenLastResult();
	static bool TetrahedralMeshGeneration(FString file_path, FTetGenParam p, TetGenResult*& res);
	static bool TetrahedralMeshGeneration(TetGenInputPLC input, FTetGenParam p, TetGenResult*& res);
	static FOnTetGenStageChangedSignature OnTetGenStageChanged;

	static void SetAndBrodacastStage(ETetGenStage value);

private:
	static FCriticalSection stageGuard;
	static char* Readline(char* string, FILE* infile, int* linenumber);
	static bool Read_vtk(FString file_path, TetGenResult& res);
	static ETetGenStage statge;
	static TetGenResult* lastResult;
};
