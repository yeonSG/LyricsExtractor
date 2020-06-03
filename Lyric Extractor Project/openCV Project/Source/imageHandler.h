#pragma once

#include <opencv2/opencv.hpp>
#include "defines.h"

using namespace cv;
using namespace std;

class contourInfo
{
public:
	int coorX_start;		//	*x좌표 시작점
	int coorX_end;			//
	int coorY_start;		//	*y좌표 시작점
	int coorY_end;			//	*y좌표 끝점 
	//int maxValue;
	vector<int> includeValues;
	int getMaxValue();

	int pixelCount;
	bool isRefed = false;	// 참조 되었는지에 대한 여부

	static vector<int> includeValuesDeduplicate(vector<int> includeValues);	//중복제거
	static vector<contourInfo> getContourInfosFromBinImage(Mat binImage, Mat &outImage);	
	static contourInfo getContourInfoFromPixels(vector<Point> pixels);
};

class WeightMat
{
public:
	Mat binImage;
	int frameNum;
	WeightMat();
	WeightMat(Mat image, int frameNum);
};

class contourLineInfo
{
public:
	vector<contourInfo> contours;	// push_back() 으로 순차적으로 넣어 사용할 것
	int coorX_start;	// 모든 컨투어의 x-start의 최소값	
	int coorX_end;		// 모든 컨투어의 x-end 의 최대값
	int coorY_start;	// 모든 컨투어의 y-start의 최소값
	int coorY_end;		// 모든 컨투어의 y-end 의 최대값
	int maxValue;		// 모든 컨투어의 maxValue 의 최대값
	int pixelCount;
	WeightMat weightMat;		// LineInfo에 대한 bin이미지 
	WeightMat weightMat_Unprint;// 해당 시점의 unprint Weight Image
	contourLineInfo();

	int getMaxValue();

	static contourLineInfo getLineinfoFrombinMat(Mat binWeightMat);
};

class contourLineInfoSet
{
public:
	contourLineInfo progress;
	contourLineInfo maximum;
	contourLineInfoSet(contourLineInfo InitLineInfo);
};


class imageHandler
{
public:
	static bool desc_contourInfo(contourInfo a, contourInfo b);
	static bool asc_contourInfo(contourInfo a, contourInfo b);
	static bool desc_contourLineInfo(contourLineInfoSet a, contourLineInfoSet b);
	static bool asc_contourLineInfo(contourLineInfoSet a, contourLineInfoSet b);
	static int getContourLineInfoVolume(contourLineInfo lineInfo);
	static bool isRelation(int a_start, int a_end, int b_start, int b_end);

	// imageHander & ImageProcesser 
	static Mat resizeImageToAnalize(static Mat& sourceImage);
	static Mat getSubtitleImage(static Mat& sourceImage);
	static Mat getResizeAndSubtitleImage(static Mat& sourceImage);
	static Mat getBinaryImage(static Mat& sourceImage);
	static Mat getBlueColorFilteredBinaryImage(static Mat& sourceImage);
	static Mat getMorphImage(static Mat& sourceImage, cv::MorphTypes type);
	static Mat getCannyImageWithBinaryImage(static Mat& binImage);
	static Mat getBorderFloodFilledImage(static Mat& binaryMat, bool toBlack = true);
	static Mat getBorderFloodFilledImageForColor(static Mat& rgbImage, bool toBlack = true);
	static Mat removeNotLyricwhiteArea(static Mat& binaryMat);

	static int getAlinedContoursCount(static Mat& binImage);
	static vector<Rect> getFillteredContours(vector<Rect> contoursRect);
	static Mat getNoiseRemovedImage(static Mat& binaryMat, bool toBlack = true);
	static Mat getDotRemovedImage(static Mat& binaryMat, int dotSize = 10, bool toBlack = true);
	static Mat getDustRemovedImage(static Mat& binaryMat, bool toBlack = true);
	static Mat getMaxColorContoursImage(static Mat& binaryMat, vector<contourLineInfo>& expectedLineInfos_curFrame);
	static Mat getColumMaskImage(int cols, int rows, int maskLength, int targetColum);
	static Mat getWhiteMaskImage(Mat mask, int x, int y, int width, int height);

	static Mat getCompositeBinaryImages(static Mat& subImage);
	static Mat getCompositeBinaryImagesRed(static Mat& subImage);
	static Mat getCompositeBinaryImagesBlue(static Mat& subImage);

	static Mat getDifferenceImage(static Mat& binImageA, static Mat& binImageB);
	static Mat getWhiteToBlackImage(static Mat& beforeImage, static Mat& afterImage);
	static Mat getBlackToWhiteImage(static Mat& beforeImage, static Mat& afterImage);
	static Mat getMaskedImage(static Mat& rgbImage, static Mat& mask);
	static Mat getMaskedImage_binImage(static Mat& binImage, static Mat& mask);
	static Mat getColorToBlackImage(static Mat& mask, Scalar removeColor);

	static Mat getBinImageTargetColored(Mat FCImage, Scalar targetColor);

	static Mat getPaintedBinImage(static Mat& rgbImage);	
	static Mat getPaintedBinImage_inner(static Mat& rgbImage, bool isBluePaint);
	static Mat getPaintedPattern(static Mat& rgbImage, Scalar pattenColor, bool ifBothHV = true);	//
	static Mat getFullyContrastImage(Mat rgbImage);
	static Mat getPixelContrastImage(Mat rgbImage);
	static Mat getPixelContrastImage_byPercent(Mat rgbImage);
	static Mat getFullyContrast_withDilate(Mat rgbImage, Scalar color);
	static Mat getSharpenAndContrastImage(Mat rgbImage);
	static Mat getSharpenImage(Mat srcImage);



	static Mat getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage);
	static Mat getFloodfillImage(Mat target, Mat toWhite);

	static Mat cutColumByHistorgram(Mat binImage);
	static Mat removeSubLyricLine(Mat binImage);
	static Mat removeNotPrimeryLyricLine(Mat binImage);

	static bool isWhite(const Vec3b& ptr);
	static bool isBlack(const Vec3b& ptr);
	static bool isBlue(const Vec3b& ptr);
	static bool isRed(const Vec3b& ptr);
	static bool isSameColor(const Scalar& sColor, const Vec3b& vColor);

	// get information from image
	static vector<int> getHorizontalProjectionData(Mat binImage);
	static Mat getHorizontalProjection(Mat binMat);
	static int getHorizontalAvgPoint(Mat binMat);
	static vector<int> getVerticalProjectionData(Mat binImage);
	static Mat getVerticalProjection(Mat binMat);
	static int getVerticalAvgPoint(Mat binMat);

	static int getWhitePixelCount(Mat binImage);
	static int getWhitePixelAvgCoordinate(Mat binImage, bool isXcoordinate);
	static int getWhitePixelAvgValue(Mat binImage);

	static vector<pair<int, int>> getWhitePixels(Mat binImage);

	static int getLeftistWhitePixel_x(Mat binImage);
	static int getRightistWhitePixel_x(Mat binImage);
	static int getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold);

	static Mat getBiasedColorImage(Mat rgbImage, Color biasedColor);

	static Mat getBWBPatternImage(Mat FCImage);
	static void stackFCImage(Mat FCImage, Mat FCImage_before, Mat& stackBinImage, Mat maskImage);
	static void stackFCImage_BlackToWhite(Mat subImage, Mat subImage_before, Mat& stackBinImage);

	static Vec3b getMostHaveRGB(static Mat rgbImage);
	static Vec3b getMostHaveRGB(vector<Vec3b> vecColor);

	static Mat stackBinImage(Mat stackBinImage, Mat patternImage);
	static uint getSumOfBinImageValues(Mat stackBinImage);
	static uint getMaximumValue(Mat binImage);
	static uint getMinimumValue(Mat binImage);

	static vector<int> getValueArrWithSort(Mat binImage);

	// image 처리 - "PeakFinder.h"
	static Mat getPatternFillImage(Mat rgbImage, Scalar targetColor);	// WBW 패턴
	static Mat getPatternFillImage_2(Mat rgbImage, Scalar targetColor);// 단일컬러로만
	static Mat getFillImage(Mat rgbImage, Scalar targetColor);
	static Mat getFillImage_unPrint(Mat rgbImage, Scalar targetColor);

	static Mat getDepthContourRemovedMat(Mat binImage);

	static int getAvgContourVolume(Mat binMImage);
};

