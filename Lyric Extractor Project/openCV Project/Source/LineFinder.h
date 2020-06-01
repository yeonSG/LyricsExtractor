#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"
#include "PeakFinder.h"

using namespace cv;
using namespace std;

#define LINEERROR_NO_ERROR		0
#define LINEERROR_PEAKFILLTER_WEIGHT	1
#define LINEERROR_PEAKFILLTER_PIXELCOUNT	2
#define LINEERROR_PEAKFILLTER_Y_LENGHTH	3
#define LINEERROR_PEAKFILLTER_X_LENGHTH	4

#define LINEERROR_MASKCHECK_PIXELCOUNT	5
#define LINEERROR_MASKCHECK_FRAMEKINDS 6
#define LINEERROR_MASKCHECK_WEIGHT	7
#define LINEERROR_MASKCHECK_WEIGHT_AVG	8
#define LINEERROR_MASKCHECK_WEIGHT_CONTINEUTY	9
#define LINEERROR_MASKCHECK_Y_LENGHTH	10
#define LINEERROR_MASKCHECK_CONTOUR_VOLUM_AVG	11

#define LINEERROR_ENDFRAME_ENDOVERFLOW	12
#define LINEERROR_ENDFRAME_RANGE	13

#define LINEERROR_MERGE_ERROR	14

//#define LINEERROR_MASKCHECK_PIXELCOUNT4	8

class LineInfo
{
public:
	int frame_start;
	int frame_end;
	Mat maskImage_withWeight;
	bool isValid = false;
	int errorNumber = 0;
	int printColor;

	void errorOccured(int errorType);
	
	static bool desc(LineInfo a, LineInfo b);
	static bool asc(LineInfo a, LineInfo b);

	/*static const int LINEERROR_NO_ERROR;
	static const int LINEERROR_NOT_FOUND_1;
	static const int LINEERROR_NOT_FOUND_2;
	static const int LINEERROR_NOT_FOUND_3;
	static const int LINEERROR_NOT_FOUND_4*/;
};

class LineFinder
{
public:
	VideoCapture* videoCapture;

public:
	LineFinder(VideoCapture* vc);

	LineInfo getLine(WeightMat weightPrintImage);
	LineInfo getLine(WeightMat weightPrintImage, Scalar unPrintColor);

	static LineInfo checkValidMask(LineInfo lineInfo);

private:
	int calculateStartTime(WeightMat weightMat, Scalar unPrintColor);
	Mat getLineMask(Mat weightPrintImage, int startFrame, Scalar unPrintColor);
	int getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor);



};