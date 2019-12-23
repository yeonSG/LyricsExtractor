#pragma once
#include <opencv2/opencv.hpp>
using namespace cv;

class videoHandler
{
public :
	static void printVideoSpec(VideoCapture vc);
	static void printCurrentFrameSpec(VideoCapture vc);

	static String frameToTime(int frame, VideoCapture& vc);
};

