#pragma once

#include <stdlib.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

/*
	가사 한 줄에대한 데이터


*/

class Line 
{
public:
	int peak;
	int startFrame;
	int endFrame;
	string lyrics;
	;

private:
	;
};