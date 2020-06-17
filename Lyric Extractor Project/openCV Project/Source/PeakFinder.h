#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"
#include "AccumulateMat.h"

using namespace cv;
using namespace std;


class PeakFinder
{
public:
	AccumulateMat accMat; 		// 입력 색상(B,R,P)에 대한누적이미지

	Mat m_stackBinImage;				// 패턴 누적 이미지
	Mat m_contourMaxBinImage;			// stackBinImage에서 컨투어들의 max값으로 치환한 이미지
	vector<contourInfo> m_contourMaxBinImage_contourInfos;		// contourMaxBinImage의 컨투어 정보
	//vector<contourLineInfo> m_contourMaxBinImage_expectedLineInfo;// contourMaxBinImage의 라인인것 같은 것 정보
	vector<contourLineInfoSet> m_contourMaxBinImage_expectedLineInfo;
	int m_frameNumber;
	Mat m_refUnprintColorWeight;

	static const int JUDGE_TIMEOUT;

	uint m_weightSumAtBinImage = 0;
	int m_colorPixelSumAtBinImage = 0;

	vector<pair< contourLineInfoSet, int>> m_expectedLineInfos;	// pair<라인정보, line_TimOutCount>		// 라인카운트는 못찾을 수록 증가시킴
	;
	// 
	// 라인정보 생성
	// timeout 체크

public:
	vector<contourLineInfoSet> frameImage_process(Mat frameImage, int frameNumber, Scalar targetColor, Mat refUnprintImage);

	void stackBinImageCorrect(contourLineInfoSet lineSet);//Mat validImage)
	
private:
	Mat stackBinImage(Mat stackBinImage, Mat patternImage, Mat refUnprintImage);
	Mat stackBinImage2(Mat stackBinImage, Mat patternImage, Mat refUnprintImage);
	Mat stackBinImage_noiseRemove(Mat stackBinImage, Mat fillImage);

	void makeContourMaxBinImageAndContourInfos();
	Mat getUnprintFillteredstackBinImage(Mat weightPaint, Mat weightUnpaint);
	contourInfo getContourInfoFromPixels(vector<Point> pixels);

	void makeExpectedLineInfos();
	vector<contourLineInfo> getLineInfoFromContourInfos(vector<contourInfo> contourInfos);
	vector<contourLineInfoSet> expectedLineInfoAfterProcess(vector<contourLineInfo> expectedLineInfo);
	Mat getLineInfoAreaCuttedImage(contourLineInfo LineInfo);

	vector<contourLineInfoSet> getJudgeLineByFrameFlow();
	bool lineValidCheck(contourLineInfo managedLine, contourLineInfo checkLine);

	contourLineInfo removeLeftNoise(contourLineInfo linInfo);

	// calculations
	int getMaxValue(contourLineInfo lineInfo);
};