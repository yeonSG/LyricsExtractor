#pragma once

#include <Windows.h>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "fileManager.h"
#include "imageHandler.h"

using namespace cv;
using namespace std;

class analyzer {
public:
	
	int getContourCount(Mat sourceImage);

	// Utils
	int getWihtePixelCount(Mat binMat);

	bool videoContoursAnalyzation(string videoPath);
	bool videoContoursAnalyzation1(string videoPath);
	bool videoContoursAnalyzation2(string videoPath);
	bool videoContoursAnalyzation3(string videoPath);

	//Mat getWhitePixelCountArr
	vector<int> vectorToAverageVector(vector<int> vec, int effectiveRange);
	vector<pair<int, int>> getJudgedLine(vector<int> vec);
	vector<pair<int, int>> getJudgedLine2(vector<int> vec);
	vector<pair<int, int>> getJudgedLine3(vector<int> vec);
	vector<pair<int, int>> getJudgedLine4(vector<int> vecWhitePixelCounts, const vector<int> verticalHistogramAverage);

	/* ���� �Ǻ� �˰��� */
	vector<int> getPeakFromWhitePixelCounts(vector<int> vecWhitePixelCounts);
	vector<pair<int, int>> getLinesFromPeak(vector<int>peaks, vector<int> vecWhitePixelCounts);
	void lineRejudgeByVerticalHistogramAverage(vector<pair<int, int>>& judgedLines,const vector<int> verticalHistogramAverage);
	/* ���� �Ǻ� �˰��� �� */

	/* �̹��� �м� �Լ�image analization  */
	vector<int> getVerticalProjectionData(Mat binImage);
	vector<int> getHorizontalProjectionData(Mat binImage);
	Mat getChangeHistorgramMat(vector<vector<int>> histogramData, int threshold);
	vector<vector<bool>> getChangeHistorgramData(vector<vector<int>> histogramData, int threshold);
	vector<int> getVerticalHistogramAverageData(vector<vector<bool>> histogramData);
	Mat vectorToBinaryMat(vector<vector<bool>> vectorData);
	Mat averageVectorToBinaryMat(vector<int> vectorData, int imageWidth);
	Mat getVerticalHistogramAverageMat(Mat changeHistogramMat);	
	/* �̹��� �м� �Լ� �� */ 

	float getAverageOnVectorTarget(vector<int> vec, int target, int range, bool includeZero=true);

	void captureLines(vector<pair<int, int>> lines, string videoPath);
	Mat ImageToSubBinImage(Mat targetImage);
	Mat getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage);
	void capturedLinesToText(int lineSize, string videoPath);
	void runOCR(string targetImage, string outFileName);
	wstring s2ws(const std::string& s);
	void makeLyrics(vector<pair<int, int>> lines, string videoPath);

	/* Inits */
	bool setVideo(string videoPath);
	void closeVideo();

public:
	VideoCapture *videoCapture;
	int video_Frame;
	int video_Width;
	int video_Height;

};