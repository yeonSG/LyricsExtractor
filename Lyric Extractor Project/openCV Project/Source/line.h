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
	//int peak;
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