#pragma once
#include <conio.h.>
#include <fstream>
#include <io.h>
#include <iostream>
#include <Shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;

class fileManager
{
public:
	static string videoName;
	
	static void initDirectory(string videoName);

	static bool isExist(string fileName);
	static void createDir(string Path);
	static int deleteDirconst(string& refcstrRootDirectory, bool bDeleteSubdirectories = true);
	
	static char* WideCharToChar(LPCWSTR lpWideCharStr);

	static string getSavePath();
	static string getLineImagePath(string videoName);
	static string getLineImageName(string videoName, int index);

	static bool writeVector(string& fileName, vector<int>& vec);
	static bool writeVector(string& fileName, vector<string>& vec);
	
	static bool readLine(string& fileName, string& readLine);

};

