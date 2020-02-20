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
	static int getAlinedContoursCount(static Mat& binImage);
	static vector<Rect> getFillteredContours(vector<Rect> contoursRect);
	static Mat getNoiseRemovedImage(static Mat& binaryMat, bool toBlack = true);
	static Mat getColumMaskImage(int cols, int rows, int maskLength, int targetColum);

	static Mat getCompositeBinaryImages(static Mat& subImage);
	static Mat getCompositeBinaryImagesRed(static Mat& subImage);
	static Mat getCompositeBinaryImagesBlue(static Mat& subImage);

	static Mat getDifferenceImage(static Mat& binImageA, static Mat& binImageB);

	static Mat getPaintedBinImage(static Mat& rgbImage);	
	static Mat getPaintedBinImage_inner(static Mat& rgbImage, bool isBluePaint);
	static Mat getFullyContrastImage(Mat rgbImage);
	static Mat getSharpenAndContrastImage(Mat rgbImage);
	static Mat getSharpenImage(Mat srcImage);

	static Mat getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage);


	static Mat cutColumByHistorgram(Mat binImage);
	static Mat removeSubLyricLine(Mat binImage);
	static Mat removeNotPrimeryLyricLine(Mat binImage);

	static bool isWhite(const Vec3b& ptr);
	static bool isBlack(const Vec3b& ptr);
	static bool isBlue(const Vec3b& ptr);
	static bool isRed(const Vec3b& ptr);

	// get information from image
	static vector<int> getVerticalProjectionData(Mat binImage);
	static vector<int> getHorizontalProjectionData(Mat binImage);
	static int getWihtePixelCount(Mat binImage);

	static int getLeftistWhitePixel_x(Mat binImage);
	static int getRightistWhitePixel_x(Mat binImage);
	static int getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold);

};

