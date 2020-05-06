#pragma once

#include <opencv2/opencv.hpp>
#include "defines.h"

using namespace cv;
using namespace std;

class imageHandler
{
public:
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
	static Mat getColumMaskImage(int cols, int rows, int maskLength, int targetColum);

	static Mat getCompositeBinaryImages(static Mat& subImage);
	static Mat getCompositeBinaryImagesRed(static Mat& subImage);
	static Mat getCompositeBinaryImagesBlue(static Mat& subImage);

	static Mat getDifferenceImage(static Mat& binImageA, static Mat& binImageB);
	static Mat getWhiteToBlackImage(static Mat& beforeImage, static Mat& afterImage);
	static Mat getBlackToWhiteImage(static Mat& beforeImage, static Mat& afterImage);
	static Mat getMaskedImage(static Mat& rgbImage, static Mat& mask);
	static Mat getColorToBlackImage(static Mat& mask, Scalar removeColor);

	static Mat getBinImageTargetColored(Mat FCImage, Scalar targetColor);

	static Mat getPaintedBinImage(static Mat& rgbImage);	
	static Mat getPaintedBinImage_inner(static Mat& rgbImage, bool isBluePaint);
	static Mat getPaintedPattern(static Mat& rgbImage, Scalar pattenColor, bool ifBothHV = true);	//
	static Mat getFullyContrastImage(Mat rgbImage);
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

	static vector<pair<int, int>> getWhitePixels(Mat binImage);

	static int getLeftistWhitePixel_x(Mat binImage);
	static int getRightistWhitePixel_x(Mat binImage);
	static int getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold);

	static Mat getBiasedColorImage(Mat rgbImage, Color biasedColor);

	static Mat getBWBPatternImage(Mat FCImage);
	static void stackFCImage(Mat FCImage, Mat FCImage_before, Mat& stackBinImage, Mat maskImage);
	static void stackFCImage_BlackToWhite(Mat subImage, Mat subImage_before, Mat& stackBinImage);

	static Vec3b getMostHaveRGB(static Mat rgbImage);

	static Mat stackBinImage(Mat stackBinImage, Mat patternImage);
	static uint getSumOfBinImageValues(Mat stackBinImage);
	static uint getMaximumValue(Mat binImage);

	static vector<int> getValueArrWithSort(Mat binImage);

};

