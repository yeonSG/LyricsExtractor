#pragma once

#include <Windows.h>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "fileManager.h"
#include "imageHandler.h"
#include <boost/filesystem/path.hpp>

using namespace cv;
using namespace std;

class analyzer {
public:
	
	/* 이미지 분석 함수 */
	int getContourCount(Mat sourceImage);
	int getWihtePixelCount(Mat binMat);

	int getLeftistWhitePixel_x(Mat binImage);
	int getRightistWhitePixel_x(Mat binImage);
	int getWhitePixelAverage(Mat binImage);

	bool videoAnalization(string videoPath);
	bool videoAnalization1(string videoPath);

	/* 데이터 분석 함수 */
	vector<int> vectorToAverageVector(vector<int> vec, int effectiveRange);

	/* 라인 판별 알고리즘 */
	vector<pair<int, int>> getJudgedLine(vector<int> vecWhitePixelCounts, const vector<int> verticalHistogramAverage);

	vector<int> getPeakFromWhitePixelCounts(vector<int> vecWhitePixelCounts);
	vector<pair<int, int>> getLinesFromPeak(vector<int>peaks, vector<int> vecWhitePixelCounts);
	void linesRejudgeByLineLength(vector<pair<int, int>>& judgedLines, int fps = DEFAULT_FPS);
	bool lineRejudgeByLineLength(int startFrame, int endFrame, int fps = DEFAULT_FPS);
	void lineRejudgeByPixelCount(vector<pair<int, int>>& judgedLines, vector<int> vecWhitePixelCounts);
	void lineRejudgeByVerticalHistogramAverage(vector<pair<int, int>>& judgedLines,const vector<int> verticalHistogramAverage);

	void calibrateLines(vector<pair<int, int>>& lines);
	bool lineCalibration(int& startFrame, int& endFrame, Mat& maskImage);
	/* 라인 판별 알고리즘 끝 */

	/* 이미지 분석 함수image analization  */
	vector<int> getVerticalProjectionData(Mat binImage);
	vector<int> getHorizontalProjectionData(Mat binImage);
	Mat getChangeHistorgramMat(vector<vector<int>> histogramData, int threshold);
	vector<vector<bool>> getChangeHistorgramData(vector<vector<int>> histogramData, int threshold);
	vector<int> getVerticalHistogramAverageData(vector<vector<bool>> histogramData);
	Mat vectorToBinaryMat(vector<vector<bool>> vectorData);
	Mat averageVectorToBinaryMat(vector<int> vectorData, int imageWidth);
	Mat getVerticalHistogramAverageMat(Mat changeHistogramMat);	
	/* 이미지 분석 함수 끝 */ 

	float getAverageOnVectorTarget(vector<int> vec, int target, int range, bool includeZero=true);

	void captureLines(vector<pair<int, int>> lines, string videoPath);
	void catpureBinaryImageOfLinesEnd(vector<pair<int, int>> lines, string videoPath);
	void catpureBinaryImageForOCR(Mat binImage, int lineNum, string videoPath);

	Mat imageToSubBinImage(Mat targetImage);
	Mat getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage);
	Mat getBinImageByFloodfillAlgorismforNoiseRemove(Mat ATImage, Mat compositeImage, int limitX);
	void capturedLinesToText(int lineSize, string videoPath);
	void runOCR(string targetImage, string outFileName);
	wstring stringToWstring(const std::string& s);
	void makeLyrics(vector<pair<int, int>> lines, string videoPath);

	/* Inits */
	bool setVideo(string videoPath);
	void closeVideo();

	Mat getLyricMask(Mat imageToCopy, int startX, int endX);

	Mat getSharpenAndContrastImage(int frameNum);
	Mat getSharpenImage(Mat srcImage);
	Mat getFullyContrastImage(Mat srcImage);
	Mat removeLint(Mat srcImage, Mat refImage);

	//
	Mat getPaintedBinImage(Mat srcImage);
	bool isWhite(const Vec3b& ptr);
	bool isBlack(const Vec3b& ptr);
	bool isBlue(const Vec3b& ptr);
	bool isRed(const Vec3b& ptr);

public:
	VideoCapture *videoCapture;
	int video_Frame;
	int video_Width;
	int video_Height;
};