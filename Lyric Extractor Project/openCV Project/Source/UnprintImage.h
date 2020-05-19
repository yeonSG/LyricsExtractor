#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"

using namespace cv;
using namespace std;


class UnprintImage
{
public:
	Mat m_UnprintImage;				// ���� ���� �̹���
	
public:
	Mat unprintImage_process(Mat frameImage, Scalar targetColor);
	
private:
	void tick();
	void updateUnprintImage(Mat binImage);
};