#include "OCRHandler.h"

#include "loger.h"
/// <summary>
/// OCR 수행함
///  Command line 예 : tesseract/tesseract_5.0.exe Output/movie.mp4/Captures/Line18_bin.jpg 0 -L tha+eng -c tessedit_char_blacklist=ABCDE--oem 1 --psm 7
///  Options :
///  OCR Engine modes(–oem) :
///  	0 - Legacy engine only.
///  	1 - Neural nets LSTM engine only.
///  	2 - Legacy + LSTM engines.
///  	3 - Default, based on what is available.
///  Page segmentation modes(–psm) :
///  	0 - Orientation and script detection(OSD) only.
///  	1 - Automatic page segmentation with OSD.
///  	2 - Automatic page segmentation, but no OSD, or OCR.
///  	3 - Fully automatic page segmentation, but no OSD. (Default)
///  	4 - Assume a single column of text of variable sizes.
///  	5 - Assume a single uniform block of vertically aligned text.
///  	6 - Assume a single uniform block of text.
///  	7 - Treat the image as a single text line.
///  	8 - Treat the image as a single word.
///  	9 - Treat the image as a single word in a circle.
///  	10 - Treat the image as a single character.
///  	11 - Sparse text.Find as much text as possible in no particular order.
///  	12 - Sparse text with OSD.
///  	13 - Raw line.Treat the image as a single text line, bypassing hacks that are Tesseract - specific.
/// </summary>
/// <param name="targetImage">OCR에 입력될 이미지파일 경로.</param>
/// <param name="outFileName">출력 OCR 파일 경로.</param>
void OCRHandler::runOCR(string targetImage, string outFileName)
{
	// tesseract_5.0.exe . . 
	// $tesseract o.jpg o.out -l kor -l tha+eng --oem 1 --psm 7 -c 
	// 인자 : "인풋이미지" + "아웃.txt경로" + "-l tha+eng"
	string procName = "tesseract\\tesseract_5.0.exe";		// tesseract 경로
	string options = " -l tha+eng --oem 1 --psm 7";
	string tessdataPath = " --tessdata-dir tesseract\\tessdata";
	string params = " -c tessedit_char_blacklist=\"|:;/\" -c preserve_interword_spaces=1  -c load_system_dawg=0 -c load_freq_dawg=0 -c tosp_min_sane_kn_sp=10";	// 블랙리스트, 띄어쓰기 제거
	string commandString = procName + " " + targetImage + " " + outFileName + tessdataPath + options + params;
	// tesseract_5.0.exe "TARGET" "OUTIMAGE -l tha+eng --oem 1 --psm 7 -c tessedit_char_blacklist=\"|:;/\" -c preserve_interword_spaces=1  -c load_system_dawg=0 -c load_freq_dawg=0  -c tosp_min_sane_kn_sp=10" 
	wstring args = stringToWstring(commandString);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HANDLE H;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));	// 프로그램 매모리 할당

	printf("runOCR : %s  \r\n", commandString.c_str());
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "runOCR() :" << commandString;

	if (!CreateProcess(NULL, (LPWSTR)args.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		fprintf(stderr, "Fail. \r\n");
		return;
	}
	H = pi.hProcess;
	WaitForMultipleObjects(1, &H, true, INFINITE);	// 모든 child가 완료될때까지 대기
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (fileManager::isExist(outFileName + ".txt") != true)
	{
		printf("OCR failed");
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "OCR failed : path - " << outFileName;

	}
	//ExitProcess(0);
}

/// <summary>
/// std::string to std::wstring 
/// </summary>
/// <param name="s">string.</param>
/// <returns>wstring</returns>
wstring OCRHandler::stringToWstring(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}