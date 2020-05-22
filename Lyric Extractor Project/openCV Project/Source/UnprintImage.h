#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"

using namespace cv;
using namespace std;


class UnprintImage
{
public:
	Mat m_UnprintImage;				// 패턴 누적 이미지
	bool m_isFindUnprintColor = false;
	Scalar m_unPrintColor = Scalar(0, 0, 0);
	
public:
	Mat unprintImage_process(Mat frameImage);

	Scalar getUnprintColor(Mat fImage);
	
private:
	void tick();
	void updateUnprintImage(Mat binImage);
};