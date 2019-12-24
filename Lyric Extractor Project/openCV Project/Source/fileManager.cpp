#include "fileManager.h"

string fileManager::videoName;

/// <summary>
/// videoName의 폴더 트리를 초기화 함.
/// 만약 기존 폴더가 있다면 삭제 진행 후 다시 폴더 생성.
/// </summary>
/// <param name="videoName">Name of the video.</param>
void fileManager::initDirectory(string videoName)
{
	string root = "Output";
	string videoRoot = root + "/" + videoName;
	string capturesPath = videoRoot + "/Captures";
	string linesPath = videoRoot + "/Lines";

	if(isExist(root))
	{
		;	// noting
	}
	else  // not exist.
	{
		createDir(root);// mkdir
	}

	// videoRoot
	if(isExist(videoRoot))
	{
		deleteDirconst(videoRoot, true);	// 하위 모든 파일 삭제
		Sleep(2000);	// 삭제 대기....
	}
	createDir(videoRoot);
	createDir(linesPath);	
	createDir(capturesPath);
	
}

bool fileManager::isExist(string fileName)
{
	if (_access(fileName.c_str(), 0) == 0)	// exist.
		return true;
	else  // not exist.
		return false;
}

void fileManager::createDir(string Path)
{
	char DirName[256];  //생성할 디렉초리 이름
	const char* p = Path.c_str();     //인자로 받은 디렉토리
	char* q = DirName;

	while (*p)
	{
		if (('\\' == *p) || ('/' == *p))   //루트디렉토리 혹은 Sub디렉토리
		{
			if (':' != *(p - 1))
			{
				CreateDirectoryA(DirName, NULL);
			}
		}
		*q++ = *p++;
		*q = '\0';
	}
	CreateDirectoryA(DirName, NULL);
}


// root 디렉토리 하위 모든 파일을 삭제함
int fileManager::deleteDirconst(string& refcstrRootDirectory, bool bDeleteSubdirectories)
{
	bool            bSubdirectory = false;       // Flag, indicating whether
												 // subdirectories have been found
	HANDLE          hFile;                       // Handle to directory
	std::string     strFilePath;                 // Filepath
	std::string     strPattern;                  // Pattern
	WIN32_FIND_DATAA FileInformation;             // File information


	strPattern = refcstrRootDirectory + "\\*.*";
	hFile = ::FindFirstFileA(strPattern.c_str(), &FileInformation);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FileInformation.cFileName[0] != '.')
			{
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (bDeleteSubdirectories)
					{
						// Delete subdirectory
						int iRC = deleteDirconst(strFilePath, bDeleteSubdirectories);
						if (iRC)
							return iRC;
					}
					else
						bSubdirectory = true;
				}
				else
				{
					// Set file attributes
					if (::SetFileAttributesA(strFilePath.c_str(),
						FILE_ATTRIBUTE_NORMAL) == FALSE)
						return ::GetLastError();

					// Delete file
					if (::DeleteFileA(strFilePath.c_str()) == FALSE)
						return ::GetLastError();
				}
			}
		} while (::FindNextFileA(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else
		{
			if (!bSubdirectory)
			{
				// Set directory attributes
				if (::SetFileAttributesA(refcstrRootDirectory.c_str(),
					FILE_ATTRIBUTE_NORMAL) == FALSE)
					return ::GetLastError();

				// Delete directory
				if (::RemoveDirectoryA(refcstrRootDirectory.c_str()) == FALSE)
					return ::GetLastError();
			}
		}
	}

	return 0;
}


char* fileManager::WideCharToChar(LPCWSTR lpWideCharStr)
{
	size_t length = wcslen(lpWideCharStr);
	++length;
	char* lpszConvertedStr = new char[length];
	int nReturnVal = WideCharToMultiByte(CP_OEMCP, 0, lpWideCharStr, -1, lpszConvertedStr, (int)length, NULL, NULL);
	//If not succeed delete pointer
	if (0 == nReturnVal)
	{
		delete[] lpszConvertedStr;
		lpszConvertedStr = NULL;
	}
	return lpszConvertedStr;
}


string fileManager::getSavePath()
{
	string savePath = "./Output/" + videoName + "/";
	
	return savePath;
}

string fileManager::getLineImagePath(string videoName)
{
	string savePath = "./Output/" + videoName + "/Captures/";
	return savePath;
}

string fileManager::getLineImageName(string videoName, int index)
{
	string Name = getLineImagePath(videoName) + to_string(index);
	return Name;
}

bool fileManager::writeVector(string& fileName, vector<int>& vec)
{
	ofstream* outStream = new ofstream((getSavePath() + fileName).data());
	if (outStream == NULL)
		outStream = new ofstream(fileName.data());
	if (outStream->fail())
	{
		cout << "Fail to open files" << endl;
		return false;
	}

	if (outStream->is_open())
	{
		for (int i = 0; i < vec.size(); i++)
			(*outStream) << vec.at(i) << "\n";
		return true;
	}
	else
		return false;

	return false;
}

bool fileManager::writeVector(string& fileName, vector<string>& vec)
{
	ofstream* outStream = new ofstream((getSavePath() + fileName).data());
	if (outStream == NULL)
		outStream = new ofstream(fileName.data());
	if (outStream->fail())
	{
		cout << "Fail to open files" << endl;
		return false;
	}

	if (outStream->is_open())
	{
		for (int i = 0; i < vec.size(); i++)
			(*outStream) << vec.at(i) << "\n";
		return true;
	}
	else
		return false;

	return false;
}

bool fileManager::readLine(string& fileName, string& readLine)
{
	ifstream* _inStream = new ifstream(fileName.data());

	if (_inStream->fail())
	{
		cout << "Fail to open files" << endl;
		return false;
	}

	if (_inStream->is_open())
	{
		if (_inStream->eof())
		{
			cout << "End of file" << endl;
			return false;
		}
		getline(*_inStream, readLine);
	}
	return true;
}
