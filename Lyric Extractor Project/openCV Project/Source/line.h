#pragma once

#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "word.h"

using namespace std;
using namespace cv;

class Line
{
public:
	//int peak;
	int startFrame;
	int endFrame;
	bool isValid; 
	vector<Word> words;
	string text;

	Mat maskImage;

private:
	;
};