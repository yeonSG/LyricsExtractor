#pragma once

#include <opencv2/opencv.hpp>
#include "defines.h"

using namespace cv;
using namespace std;

class imageHandler
{
public:
	// imageHander & ImageProcesser 
	static Mat getSubtitleImage(Mat sourceImage);
	static Mat getBinaryImage(Mat sourceImage);
	static Mat getBlueColorFilteredBinaryImage(Mat sourceImage);
	static Mat getMorphImage(Mat sourceImage);
	static Mat getCannyImageWithBinaryImage(Mat binImage);
	static Mat getFloodProcessedImage(Mat binaryMat, bool toBlack = true);

	static Mat getCompositeBinaryImages(Mat subImage);
	static Mat getCompositeBinaryImagesRed(Mat subImage);
	static Mat getCompositeBinaryImagesBlue(Mat subImage);

	static Mat getDifferenceImage(Mat binImageA, Mat binImageB);

};

