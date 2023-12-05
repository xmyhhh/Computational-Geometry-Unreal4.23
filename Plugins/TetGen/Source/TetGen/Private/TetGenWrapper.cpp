#include "TetGenWrapper.h"

#include "StringConv.h"
#include "tetgen_lib.h"

ETetGenStage TetGenWrapper::GetTetGenStage()
{
	FScopeLock Lock(&stageGuard);
	return statge;
}

const TetGenResult* TetGenWrapper::GetTetGenLastResult()
{
	return lastResult;
}

bool TetGenWrapper::TetrahedralMeshGeneration(FString file_path, FTetGenParam p, TetGenResult*& res)
{
	tetgenio in, out;
	tetgenbehavior b;

	SetAndBrodacastStage(ETetGenStage::Stage_Begin);

	FString op = "-pkBNEFOAa" + p.max_size + "q" + p.max_radius_edge_ration_bound + "/" + p.min_dihedral_angle_bound;

	char* argv[3] = {"null", TCHAR_TO_ANSI(*op), TCHAR_TO_ANSI(*file_path)};
	if (!b.parse_commandline(3, argv))
	{
		return false;
	}
	SetAndBrodacastStage(ETetGenStage::Stage_LoadGeometry);
	if (!in.load_plc(b.infilename, (int)b.object))
	{
		return false;
	}

	tetrahedralize(&b, &in, &out, NULL, NULL);

	if (lastResult != nullptr)
	{
		if (lastResult->pointList != nullptr)
			delete(lastResult->pointList);
		if (lastResult->tetrahedraList != nullptr)
			delete(lastResult->tetrahedraList);
		lastResult = nullptr;
	}

	//	Read VTK
	lastResult = new TetGenResult();
	res = lastResult;
	Read_vtk(file_path, *res);

	SetAndBrodacastStage(ETetGenStage::Stage_End);
	return true;
}

bool TetGenWrapper::TetrahedralMeshGeneration(TetGenInputPLC input, FTetGenParam p, TetGenResult*& res)
{
	tetgenio in, out;
	tetgenbehavior b;

	SetAndBrodacastStage(ETetGenStage::Stage_Begin);

	FString op = "-pAa" + p.max_size + "q" + p.max_radius_edge_ration_bound + "/" + p.min_dihedral_angle_bound;

	char* argv[3] = {"null", TCHAR_TO_ANSI(*op), ""};
	if (!b.parse_commandline(3, argv))
	{
		return false;
	}
	SetAndBrodacastStage(ETetGenStage::Stage_LoadGeometry);
	//convert input data to tetgen input io
	{
		in.numberofpoints = input.numberOfPoints;
		in.pointlist = new REAL[input.numberOfPoints * 3];
		memcpy(in.pointlist, input.pointList, sizeof(REAL) * input.numberOfPoints * 3);
		in.numberoffacets = input.numberOfFace;
		in.facetlist = new tetgenio::facet[input.numberOfFace];
		for (int i = 0; i < input.numberOfFace; i++)
		{
			auto f = &in.facetlist[i];
			f->numberofholes = 0;
			f->holelist = (REAL*)NULL;
			f->numberofpolygons = 1;
			f->polygonlist = new tetgenio::polygon[1];
			auto p = &f->polygonlist[0];
			p->vertexlist = (int*)NULL;
			p->numberofvertices = 3;
			p->vertexlist = new int[p->numberofvertices];
			p->vertexlist[0] = input.faceList[i].pointList[0];
			p->vertexlist[1] = input.faceList[i].pointList[1];
			p->vertexlist[2] = input.faceList[i].pointList[2];
		}
	}
	tetrahedralize(&b, &in, &out, NULL, NULL);

	if (lastResult != nullptr)
	{
		if (lastResult->pointList != nullptr)
			delete(lastResult->pointList);
		if (lastResult->tetrahedraList != nullptr)
			delete(lastResult->tetrahedraList);
		lastResult = nullptr;
	}

	//TODO:
	lastResult = new TetGenResult();
	lastResult->numberOfPoints = out.numberofpoints;
	lastResult->pointList = new double[out.numberofpoints * 3];
	memcpy(lastResult->pointList, out.pointlist, sizeof(double) * out.numberofpoints * 3);

	lastResult->numberOfTetrahedra = out.numberoftetrahedra;
	lastResult->tetrahedraList = new Tetrahedra[out.numberoftetrahedra];
	for (int i = 0; i < out.numberoftetrahedra; i++)
	{
		auto t = &lastResult->tetrahedraList[i];
		t->pointList = new int[4];
		t->pointList[0] = out.tetrahedronlist[i * 4];
		t->pointList[1] = out.tetrahedronlist[i * 4 + 1];
		t->pointList[2] = out.tetrahedronlist[i * 4 + 2];
		t->pointList[3] = out.tetrahedronlist[i * 4 + 3];
	}
	res = lastResult;

	SetAndBrodacastStage(ETetGenStage::Stage_End);
	return true;
}


void TetGenWrapper::SetAndBrodacastStage(ETetGenStage value)
{
	FScopeLock Lock(&stageGuard);
	statge = value;
	OnTetGenStageChanged.Broadcast(statge);
}

char* TetGenWrapper::Readline(char* string, FILE* infile, int* linenumber)
{
	char* result;

	// Search for a non-empty line.
	do
	{
		result = fgets(string, INPUTLINESIZE - 1, infile);
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
}

bool TetGenWrapper::Read_vtk(FString file_path, TetGenResult& res)
{
	char vtk_file_path[1024];
	{
		int index;
		file_path.FindLastChar('.', index);

		strncpy(vtk_file_path,TCHAR_TO_ANSI(*file_path), 1023);
		vtk_file_path[index++] = '.';
		vtk_file_path[index++] = '1';
		vtk_file_path[index++] = '.';
		vtk_file_path[index++] = 'v';
		vtk_file_path[index++] = 't';
		vtk_file_path[index++] = 'k';
		vtk_file_path[index++] = '\0';
	}

	char buffer[2048];
	char* bufferp;
	int line_count = 0;
	FILE* fp = fopen(vtk_file_path, "r");
	if (fp == (FILE*)NULL)
	{
		printf("File I/O Error:  Cannot create file %s.\n", vtk_file_path);
		return false;
	}

	int nverts = 0, iverts = 0;
	int ntetrahedras = 0, itetrahedras = 0;
	while ((bufferp = Readline(buffer, fp, &line_count)) != NULL)
	{
		if (nverts == 0)
		{
			Readline(buffer, fp, &line_count); //Unstructured Grid
			Readline(buffer, fp, &line_count); //ASCII
			Readline(buffer, fp, &line_count); //DATASET UNSTRUCTURED_GRID
			Readline(buffer, fp, &line_count); //POINTS xxxx double
			sscanf(bufferp, "%*s %d %*s", &nverts);
			if (nverts < 3)
			{
				printf("Syntax error reading header on line %d in file %s\n",
				       line_count, vtk_file_path);
				fclose(fp);
				return false;
			}
			res.numberOfPoints = nverts;
			res.pointList = new double[nverts * 3];
		}
		else if (nverts > iverts)
		{
			res.pointList[iverts * 3] = (double)strtod(bufferp, &bufferp);
			res.pointList[iverts * 3 + 1] = (double)strtod(bufferp, &bufferp);
			res.pointList[iverts * 3 + 2] = (double)strtod(bufferp, &bufferp);
			iverts++;
		}
		else if (ntetrahedras == 0)
		{
			//CELLS 35186 175930
			sscanf(bufferp, "%*s %d %*d", &ntetrahedras);
			res.tetrahedraList = new Tetrahedra[ntetrahedras];
			res.numberOfTetrahedra = ntetrahedras;
		}
		else if (ntetrahedras > itetrahedras)
		{
			res.tetrahedraList[itetrahedras].pointList = new int[4];
			sscanf(bufferp, "%*d %d %d %d %d",
			       &res.tetrahedraList[itetrahedras].pointList[0],
			       &res.tetrahedraList[itetrahedras].pointList[1],
			       &res.tetrahedraList[itetrahedras].pointList[2],
			       &res.tetrahedraList[itetrahedras].pointList[3]
			);
			itetrahedras++;
		}
	}

	return true;
}

FCriticalSection TetGenWrapper::stageGuard;
ETetGenStage TetGenWrapper::statge = ETetGenStage::Stage_Begin;
FOnTetGenStageChangedSignature TetGenWrapper::OnTetGenStageChanged;
TetGenResult* TetGenWrapper::lastResult = nullptr;
