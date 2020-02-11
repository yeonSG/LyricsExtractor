#pragma once

#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"
#include "word.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace cv;

class Line : public Word
{
public:
	// int peak;
	// int linePrintStartRow;	// 라인이 출력되는 위치 시작점
	// int linePrintHeight;		// 라인이 출력되는 위치 시작점부터 끝까지의 길이
	// 어떻게 측정하나?
	// 1. 일단 큰범위로 계산,
	// 2. horizontal 히스토그램 그려서

	bool isValid; 

	vector<Word> words;
	vector<int> spacingWords;		// 스페이스바 위치
	
	Mat maskImage;

	void splitLineTextToWord();

public:
	void getSpacingWordsFromMaskImage();

	static Word lineToWord(Line line);

private:
	;
};