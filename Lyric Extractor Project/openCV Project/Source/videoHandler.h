#pragma once
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <boost/filesystem/path.hpp>
#include "fileManager.h"
using namespace cv;
using namespace std;

class videoHandler
{
public :
	static double getVideoFPS();
	static int getVideoFrameCount();
	static int getVideoWidth();
	static int getVideoHeight();
	static void printVideoSpec();
	static void printCurrentFrameSpec(VideoCapture vc);

	static String frameToTime(int frame, VideoCapture& vc);

	static VideoCapture* getVideoCapture();

	static bool setVideo(string videoPath);
	static void closeVideo();

	static VideoCapture* videoCapture;
};

