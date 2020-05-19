#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"

using namespace cv;
using namespace std;


class PeakFinder
{
public:
	Mat m_stackBinImage;				// ���� ���� �̹���
	Mat m_contourMaxBinImage;			// stackBinImage���� ��������� max������ ġȯ�� �̹���
	vector<contourInfo> m_contourMaxBinImage_contourInfos;		// contourMaxBinImage�� ������ ����
	vector<contourLineInfo> m_contourMaxBinImage_expectedLineInfo;// contourMaxBinImage�� �����ΰ� ���� �� ����
	int m_frameNumber;
	Mat m_refUnprintColorWeight;

	static const int JUDGE_TIMEOUT;

	uint m_weightSumAtBinImage = 0;
	int m_colorPixelSumAtBinImage = 0;

	vector<pair< contourLineInfo, int>> m_expectedLineInfos;	// pair<��������, line_TimOutCount>		// ����ī��Ʈ�� ��ã�� ���� ������Ŵ
	;
	// 
	// �������� ����
	// timeout üũ

public:
	vector<contourLineInfo> frameImage_process(Mat frameImage, int frameNumber, Scalar targetColor, Mat refUnprintImage);

	void stackBinImageCorrect(Mat validImage);
	
private:
	Mat stackBinImage(Mat stackBinImage, Mat patternImage, Mat refUnprintImage);;

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