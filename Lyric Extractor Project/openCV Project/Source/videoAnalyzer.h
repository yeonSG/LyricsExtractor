#pragma once

#include <Windows.h>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "fileManager.h"
#include "imageHandler.h"
#include <boost/filesystem/path.hpp>

using namespace cv;
using namespace std;

class videoAnalyzer {
public:
	bool videoAnalize(string videoPath);
	void initVariables();


public:
	VideoCapture *videoCapture;
	
	vector<int> vecPaintedPixelCounts;		// 라인 판단용 변수
};