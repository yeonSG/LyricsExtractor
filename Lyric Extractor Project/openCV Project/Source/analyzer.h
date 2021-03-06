#pragma once

#include <Windows.h>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "fileManager.h"
#include "imageHandler.h"
#include <boost/filesystem/path.hpp>
#include "Line.h"
#include "lyric.h"
#include "Json.h"
#include "MVInformation.h"
#include "LineInfoFinder.h"
#include "OCRHandler.h"

using namespace cv;
using namespace std;

class analyzer {
public:

	void initVariables();
	
	/* 이미지 분석 함수 */
	int getContourCount(Mat sourceImage);

	int getWhitePixelAverage(Mat binImage);

	bool startVideoAnalization(string videoPath);
	bool videoAnalization(string videoPath);
	//bool videoAnalization2(string videoPath);
	bool videoAnalization3(string videoPath);
	bool getUnprintColorRutin(Scalar& color);

	MVInformation findLineInfo(VideoCapture *videoCapture);

	/* 데이터 분석 함수 */
	vector<int> vectorToAverageVector(vector<int> vec, int effectiveRange);

	/* 라인 판별 알고리즘 */
	void getJudgedLine();

	vector<int> getPeakFromWhitePixelCounts(vector<int> vecWhitePixelCounts);
	vector<Line> getLinesFromPeak(vector<int>peaks, vector<int> vecWhitePixelCounts);
	void linesRejudgeByLineLength(int fps = DEFAULT_FPS);
	bool lineRejudgeByLineLength(int startFrame, int endFrame, int fps = DEFAULT_FPS);
	void lineRejudgeByPixelCount(vector<int> vecWhitePixelCounts);
	void lineRejudgeByVerticalHistogramAverage(vector<pair<int, int>>& judgedLines,const vector<int> verticalHistogramAverage);

	void calibrateLines();
	bool lineCalibration(Line& line, static int minStartFrame);
	//bool lineCalibration(int& startFrame, int& endFrame, Mat& maskImage, static int minStartFrame);
	/* 라인 판별 알고리즘 끝 */

	/* 이미지 분석 함수image analization  */
	Mat getChangeHistorgramMat(vector<vector<int>> histogramData, int threshold);
	vector<vector<bool>> getChangeHistorgramData(vector<vector<int>> histogramData, int threshold);
	vector<int> getVerticalHistogramAverageData(vector<vector<bool>> histogramData);
	Mat vectorToBinaryMat(vector<vector<bool>> vectorData);
	Mat averageVectorToBinaryMat(vector<int> vectorData, int imageWidth);
	Mat getVerticalHistogramAverageMat(Mat changeHistogramMat);	
	/* 이미지 분석 함수 끝 */ 

	float getAverageOnVectorTarget(vector<int> vec, int target, int range, bool includeZero=true);

	void captureLines();
	void catpureBinaryImageOfLinesEnd(vector<pair<int, int>> lines, string videoPath);
	void captureBinaryImage(string videoPath, int index, Mat image);
	void catpureBinaryImageForOCR(Mat binImage, int lineNum, string videoPath);

	Mat imageToSubBinImage(Mat targetImage);
	Mat getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage);
	Mat getBinImageByFloodfillAlgorismforNoiseRemove(Mat ATImage, Mat compositeImage, int limitX);
	void capturedLinesToText();
	void readLyricsFromFile();

	/* Inits */
	bool setVideo(string videoPath);
	void closeVideo();

	Mat getLyricMask(Mat imageToCopy, int startX, int endX);

	Mat getFullyContrastImage(Mat srcImage);
	Mat removeLint(Mat srcImage, Mat refImage);


	void wordCalibration();
	void wordCalibrationByOCRText(Line& line);

	vector<pair<int, int>> getPaintedPoint(Line line);

	Mat getDeblurImage(Mat sourceImage, int frameNum);

	void findErrorFromLyrics();

	Mat weightImageToOCRbin(Mat weightMat, Scalar unprintColor, int startFrame);

	bool unPrintColorValidCheck(Scalar foundUnPrintColor);

public:
	VideoCapture *videoCapture = nullptr;		

	vector<int> vecWhitePixelCounts;			// 프래임 별 흰색 개수
	vector<int> vecWhitePixelChangedCounts;		// 이전 프래임 대비 흰색 변화량

	Lyric m_lyric;
};