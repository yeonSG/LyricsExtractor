#pragma once

#include <stdlib.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class Word
{
public:
	int startFrame;
	int endFrame;
	string text;

	int startFrame_ms;
	int endFrame_ms;

private:
	;
};