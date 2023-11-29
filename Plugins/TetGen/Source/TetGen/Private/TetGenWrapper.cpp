#include "TetGenWrapper.h"

#include "StringConv.h"
#include "tetgen_lib.h"

bool TetGenWrapper::TetrahedralMeshGeneration(TetGenParam p, TetGenResult& res)
{
	tetgenio in, out;
	tetgenbehavior b;

	FString op = "-pBNEFOAa" + p.max_size + "q" + p.max_radius_edge_ration_bound + "/" + p.min_dihedral_angle_bound;
	op = "-pkq";
	char* argv[3] = {"null", TCHAR_TO_ANSI(*op), TCHAR_TO_ANSI(*p.file_path)};
	if (!b.parse_commandline(3, argv))
	{
		return false;
	}

	if (!in.load_plc(b.infilename, (int)b.object))
	{
		return false;
	}

	//tetrahedralize(&b, &in, &out, NULL, NULL);


	//	Read VTK
	Read_vtk(p.file_path, res);

	return true;
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
