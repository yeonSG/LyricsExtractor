#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"

using namespace cv;
using namespace std;


class PeakFinder
{
public:
	Mat stackBinImage;				// 패턴 누적 이미지
	Mat contourMaxBinImage;			// stackBinImage에서 컨투어들의 max값으로 치환한 이미지
	vector<contourInfo> contourMaxBinImage_contourInfos;		// contourMaxBinImage의 컨투어 정보
	vector<contourLineInfo> contourMaxBinImage_expectedLineInfo;// contourMaxBinImage의 라인인것 같은 것 정보
	int frameNumber;

	static const int JUDGE_TIMEOUT;

	uint weightSumAtBinImage = 0;
	int colorPixelSumAtBinImage = 0;

	vector<pair< contourLineInfo, int>> expectedLineInfos;	// pair<라인정보, line_TimOutCount>		// 라인카운트는 못찾을 수록 증가시킴
	;
	// 
	// 라인정보 생성
	// timeout 체크

public:
	vector<contourLineInfo> frameImage_process(Mat frameImage, int frameNumber, Scalar targetColor);
	
private:
	void makeContourMaxBinImageAndContourInfos();
	contourInfo getContourInfoFromPixels(vector<Point> pixels);

	void makeExpectedLineInfos();
	vector<contourLineInfo> getLineInfoFromContourInfos(vector<contourInfo> contourInfos);
	vector<contourLineInfo> expectedLineInfoAfterProcess(vector<contourLineInfo> expectedLineInfo);
	Mat getLineInfoAreaCuttedImage(contourLineInfo LineInfo);

	vector<contourLineInfo> getJudgeLineByFrameFlow();

	// calculations
	int getMaxValue(contourLineInfo lineInfo);
};