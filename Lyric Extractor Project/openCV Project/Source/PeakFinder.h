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
	AccumulateMat accMat; 		// �Է� ����(B,R,P)�� ���Ѵ����̹���

	Mat m_stackBinImage;				// ���� ���� �̹���
	Mat m_contourMaxBinImage;			// stackBinImage���� ��������� max������ ġȯ�� �̹���
	vector<contourInfo> m_contourMaxBinImage_contourInfos;		// contourMaxBinImage�� ������ ����
	//vector<contourLineInfo> m_contourMaxBinImage_expectedLineInfo;// contourMaxBinImage�� �����ΰ� ���� �� ����
	vector<contourLineInfoSet> m_contourMaxBinImage_expectedLineInfo;
	int m_frameNumber;
	Mat m_refUnprintColorWeight;

	static const int JUDGE_TIMEOUT;

	uint m_weightSumAtBinImage = 0;
	int m_colorPixelSumAtBinImage = 0;

	vector<pair< contourLineInfoSet, int>> m_expectedLineInfos;	// pair<��������, line_TimOutCount>		// ����ī��Ʈ�� ��ã�� ���� ������Ŵ
	;
	// 
	// �������� ����
	// timeout üũ

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