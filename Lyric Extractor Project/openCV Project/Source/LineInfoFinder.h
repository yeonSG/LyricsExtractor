#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "imageHandler.h"
#include "MVInformation.h"
#include "Line.h"


using namespace cv;
using namespace std;

class FrameInfo
{
public:
	int frame;

	int pixelCount;
	int avgCoordinate;			// pixelCount의 요소들에 대한 평균 좌표(x)

	int added_pixelCount;
	int added_avgCoordinate;	// 이전 비교프래임 대비 추가된 pixelCount의 요소들에 대한 평균 좌표(x)
};

class PeakInfo
{
public:
	int frameNum;
	int maxWeightPixel;
	Mat PeakImage;
};

class LineInfo
{
public:
	int startFrame;
	int endFrame;
	Mat binaryImage;

};


class LineInfoFinder {
public:
	bool start();
	bool start2();
	vector<PeakInfo> start2_getLinePeak(int PrintTypeNum);
	bool isPeakFrame(Mat expectedFrame);
	bool isPeakFrame2(Mat expectedFrame);

	bool isLineFrame(Mat expectedFrame);

	vector<Line> peakToLine(vector<PeakInfo> peaks);

	

	MVInformation getMVInformation();	
	
	LineInfoFinder(VideoCapture* videoCapture);// Constructor

public:
	VideoCapture *videoCapture;
		
	vector<Scalar> vecPrintTypes;
	vector<Scalar> vecUnPrintTypes;

	vector<vector<int>> vecPrintTypes_PatternPixelCount;

	void WriteLineInfo_toLog(vector<Line> lineInfos);
	
private:
	MVInformation m_mvInformation;

	Mat getPatternFillImage(Mat rgbImage, Scalar targetColor);
	Mat getFillImage(Mat rgbImage, Scalar targetColor);

	/* for */
	vector<int> getPeak(vector<int> vecPixelCounts);

	Mat stackBinImage(Mat addImage, Mat stackImage);

};