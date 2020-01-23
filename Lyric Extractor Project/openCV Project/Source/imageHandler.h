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
	static Mat getMorphImage(static Mat& sourceImage, MorphTypes type);
	static Mat getCannyImageWithBinaryImage(static Mat& binImage);
	static Mat getFloodProcessedImage(static Mat& binaryMat, bool toBlack = true);

	static Mat getCompositeBinaryImages(static Mat& subImage);
	static Mat getCompositeBinaryImagesRed(static Mat& subImage);
	static Mat getCompositeBinaryImagesBlue(static Mat& subImage);

	static Mat getDifferenceImage(static Mat& binImageA, static Mat& binImageB);
};

