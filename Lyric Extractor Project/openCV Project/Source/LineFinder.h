#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"
#include "PeakFinder.h"

using namespace cv;
using namespace std;

class LineInfo
{
public:
	int frame_start;
	int frame_end;
	Mat maskImage_withWeight;
	bool isValid = false;
	int printColor;
	
	static bool desc(LineInfo a, LineInfo b);
	static bool asc(LineInfo a, LineInfo b);
};

class LineFinder
{
public:
	VideoCapture* videoCapture;

public:
	LineFinder(VideoCapture* vc);

	LineInfo getLine(WeightMat weightPrintImage);
	LineInfo getLine(WeightMat weightPrintImage, Scalar unPrintColor);

	static bool checkValidMask(Mat maskImage);

private:
	int calculateStartTime(WeightMat weightMat);
	Mat getLineMask(Mat weightPrintImage, int startFrame, Scalar unPrintColor);
	int getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor);



};