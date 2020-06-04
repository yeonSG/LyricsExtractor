#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "imageHandler.h"
#include "MVInformation.h"
#include "Line.h"

#include "PeakFinder.h"
#include "LineFinder.h"


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

class LineInfoFinder {
public:
	bool start();
	bool start2();
	vector<PeakInfo> start2_getLinePeak(int PrintTypeNum);
	vector<PeakInfo> start2_useContour(int PrintTypeNum);
	vector<LineInfo> start2_useContour2(int PrintTypeNum, Scalar UnprintColor);
	//Scalar findUnprintColor(int PrintTypeNum);
	bool isPeakFrame(Mat expectedFrame);
	bool isPeakFrame2(Mat expectedFrame);

	bool isLineFrame(Mat expectedFrame);

	bool findUnprintColor(vector<PeakInfo> peaks, Vec3b& findColor);

	Mat getUnprintFillteredstackBinImage(Mat weightPaint, Mat weightUnpaint);

	vector<Line> peakToLine(vector<PeakInfo> peaks, Vec3b unprintColor);

	

	MVInformation getMVInformation();	
	
	LineInfoFinder(VideoCapture* videoCapture);// Constructor

public:
	VideoCapture *videoCapture;
		
	vector<Scalar> vecPrintTypes;
	vector<Scalar> vecUnPrintTypes;

	int m_PrintTypeNumType;

	vector<vector<int>> vecPrintTypes_PatternPixelCount;

	void WriteLineInfo_toLog(vector<Line> lineInfos);

	vector<contourLineInfoSet> line_PeakInfoFilter(vector<contourLineInfoSet> lineInfosSet, vector<LineInfo>& errorLineInfos);
	int getSequentialIncreasedContoursCount(vector<contourInfo> contours);

	vector<LineInfo> mergeAndJudgeLineInfo(vector<LineInfo> lineInfos);
	vector<LineInfo> mergeLineInfo(vector<LineInfo> lineInfos);		// 단색 머지랑, 복합 색 머지랑 달라야 함 (듀엣 곡일 경우 빨,파 함께 진행되는 경우가 있음)
		// 단색 머지 - 부분집합 관계시 하나는 영어자막으로 보고 삭제함
		// 컬러 머지 - 부분집합 관계시 듀엣으로 보고 머지함
	//vector<LineInfo> mergeLineInfo(vector<LineInfo> lineInfos);	// 
	
private:
	MVInformation m_mvInformation;

	Mat getPatternFillImage(Mat rgbImage, Scalar targetColor);
	Mat getPatternFillImage_2(Mat rgbImage, Scalar targetColor);
	Mat getFillImage(Mat rgbImage, Scalar targetColor);

	Mat getWeightCompareImage(Mat weightUnpaint, Mat weightPaint);

	/* for */
	vector<int> getPeak(vector<int> vecPixelCounts);

	Mat stackBinImage(Mat addImage, Mat stackImage);

};