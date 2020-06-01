#pragma once

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "imageHandler.h"

using namespace cv;
using namespace std;


class AccumulateMat
{
public:
	Mat m_AccImage;				// ���� �̹���
	
public:
	Mat accumulateProcess(Mat frameImage);
	
private:
	void tick(Mat binImage);
	void updateAccImage(Mat binImage);
};