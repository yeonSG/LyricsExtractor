#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"

using namespace cv;
using namespace std;


class PeakFinder
{
public:
	Mat stackBinImage;				// ���� ���� �̹���
	Mat contourMaxBinImage;			// stackBinImage���� ��������� max������ ġȯ�� �̹���
	vector<contourInfo> contourMaxBinImage_contourInfos;		// contourMaxBinImage�� ������ ����
	vector<contourLineInfo> contourMaxBinImage_expectedLineInfo;// contourMaxBinImage�� �����ΰ� ���� �� ����
	int frameNumber;

	static const int JUDGE_TIMEOUT;

	uint weightSumAtBinImage = 0;
	int colorPixelSumAtBinImage = 0;

	vector<pair< contourLineInfo, int>> expectedLineInfos;	// pair<��������, line_TimOutCount>		// ����ī��Ʈ�� ��ã�� ���� ������Ŵ
	;
	// 
	// �������� ����
	// timeout üũ

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