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
	static Mat getBinaryImage(static Mat& sourceImage);
	static Mat getBlueColorFilteredBinaryImage(static Mat& sourceImage);
	static Mat getMorphImage(static Mat& sourceImage, cv::MorphTypes type);
	static Mat getCannyImageWithBinaryImage(static Mat& binImage);
	static Mat getFloodProcessedImage(static Mat& binaryMat, bool toBlack = true);
	static Mat getColumMaskImage(int cols, int rows, int maskLength, int targetColum);

	static Mat getCompositeBinaryImages(static Mat& subImage);
	static Mat getCompositeBinaryImagesRed(static Mat& subImage);
	static Mat getCompositeBinaryImagesBlue(static Mat& subImage);

	static Mat getDifferenceImage(static Mat& binImageA, static Mat& binImageB);

	static Mat getPaintedBinImage(static Mat& rgbImage);	
	static Mat getFullyContrastImage(Mat rgbImage);
	static bool isWhite(const Vec3b& ptr);
	static bool isBlack(const Vec3b& ptr);
	static bool isBlue(const Vec3b& ptr);
	static bool isRed(const Vec3b& ptr);

	// get information from image
	static vector<int> getVerticalProjectionData(Mat binImage);
	static vector<int> getHorizontalProjectionData(Mat binImage);
	static int getWihtePixelCount(Mat binImage);

};

