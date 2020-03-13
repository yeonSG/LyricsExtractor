#include <Windows.h>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <direct.h>
#include <zlib.h>
#include "testClass.h"
#include "analyzer.h"
#include "fileManager.h"
#include "loger.h"

using namespace std;
using namespace cv;


int main(int, char**)
{
	clock_t startClock = (int)clock();
	loger_init(); 
	_logging("start", severity_level::normal);

	//system("python deblur.py debug_2_upscale.png");
	
	testClass testClass;
	//testClass.test_Image();
	//testClass.test_Image2();
	//testClass.test_Image4();
	//testClass.test_Video3();
	//testClass.test_Video_GetContourMask2("40009.mp4");

	//testClass.test_Video("40009.mp4");

	//testClass.test_Video4("40009.mp4");
	//testClass.test_Video4("40011_forTest.mp4");
	//testClass.test_Video4("40011.mp4");
	//testClass.test_Video4("movie1_forTest.mp4");
	//testClass.test_Video4("movie1_forTest2.mp4");
	//testClass.test_Video4("movie.mp4");
	//testClass.test_Video4("movie1.mp4");

	analyzer ana;
	//ana.videoAnalization1("movie1.mp4");
	//ana.videoAnalization1("40009.mp4");

	//ana.videoAnalization("movie1_forTest2.mp4");

	//ana.videoAnalization("40009_forDebug.mp4");
	//ana.videoAnalization("movie1_forTest2.mp4");
	//ana.videoAnalization("movie1_forTest2.mp4");
	//ana.videoAnalization("40011_forTest.mp4");
	//ana.videoAnalization("40011_forTest_upscale.mp4");
	ana.videoAnalization("movie1.mp4");
	ana.videoAnalization("movie.mp4");
	ana.videoAnalization("40009.mp4");
	ana.videoAnalization("40011.mp4");
	ana.videoAnalization("40006.mp4");
	ana.videoAnalization("40003.mp4");

	//ana.videoAnalization("gramy_vol8_4.mp4");
	ana.videoAnalization("gramy_vol8_6.mp4");
	

	/*named_scope_logging();
	tagged_logging();
	timed_logging();*/
	

	printf("\r\nProcess Successed : %0.1fSec\r\n", (float)(clock() - startClock) / CLOCKS_PER_SEC);
	return 0;
}

//
//#include <stdio.h>
//#include <time.h>
//#include "opencv2\opencv.hpp"
//
//using namespace cv;
//using namespace std;
//
//int main(int argc, char* argv[])
//{
//
//	Mat matOrg;
//	Mat matOut;
//
//	long totalFrames = 0;
//	clock_t prevClock;
//	int key = 0;
//
//	VideoCapture cap("movie.mp4");
//
//	if (!cap.isOpened())
//	{
//		printf("video not opened");
//		return -1;
//	}
//
//	prevClock = clock();
//
//	while (key != 27) {
//
//		cap >> matOrg;
//
//		if (matOrg.empty()) {
//			printf("video frame end\n");
//			return -1;
//		}
//
//		cvtColor(matOrg, matOrg, COLOR_BGR2GRAY);
//
//		clock_t curClock = clock();
//		printf("%.2lf fps\n", (float)CLOCKS_PER_SEC / (float)(curClock - prevClock));
//		prevClock = curClock;
//
//
//		/* Variable */
//		Mat frame;
//		Mat binary;
//		Mat canny;
//		Mat result;
//
//		double boundX = 0, boundY = 0;
//		double area = 0.0;
//
//
//		/* Frame Clone, Resizing */
//		frame = matOrg.clone();
//		resize(frame, frame, Size(), 0.8, 0.8);
//
//		/* Adaptive Binarization : πË∞Ê ∞Ì∑¡«— ¿Ã¡¯»≠ */
//		adaptiveThreshold(frame, binary, 255,
//			ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 21, 20);
//
//
//		/* Canny, Contour, Drawing Contour */
//		Canny(binary, canny, 100, 200, 3);		// ¿±∞˚¿ª ±◊∏≤
//		std::vector<std::vector<cv::Point>> contours;
//		findContours(canny, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);	// ¿±∞˚¿« ¡¬«•∏¶ √ﬂ√‚«‘
//
//		cvtColor(frame, result, COLOR_GRAY2BGR);
//		drawContours(result, contours, -1, Scalar(0, 255, 250), 1);
//
//
//		/* Calculate Pore point */
//		// Get the moments
//		vector<Moments> mu(contours.size());
//
//		for (int i = 0; i < contours.size(); i++)
//			mu[i] = moments(contours[i], false);
//
//		// Get the centroid of figures
//		vector<Point2d> mc(contours.size());
//		for (int i = 0; i < contours.size(); i++)
//			mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
//
//
//		/* Bounding box */
//		vector<Rect> boundRect(contours.size());
//		vector<vector<Point>>contours_poly(contours.size());
//
//		for (int i = 0; i < contours.size(); i++)
//		{
//			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
//			boundRect[i] = boundingRect(Mat(contours_poly[i]));
//		}
//
//
//		/* Calculate Pore Area, Width, Height */
//		/* Drawing Circle, Rectangle, putText */
//		for (int i = 0; i < contours.size(); i++)
//		{
//			area = contourArea(contours[i]);
//			boundX = boundRect[i].width;
//			boundY = boundRect[i].height;
//
//			if (area > 3) {
//
//				circle(result, mc[i], 1.5, Scalar(0, 0, 255), -1, 8, 0);
//				rectangle(result, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 0, 0), 1, 8, 0);
//
//				Point p = mc[i];
//				p.x += 3; p.y += 3;
//				char buf1[100];
//				sprintf_s(buf1, "A:%.1f W:%.1f H:%.1f", area, boundX, boundY);
//				putText(result, buf1, p, FONT_HERSHEY_SIMPLEX, 0.4, Scalar(255, 0, 0), 1, 1);
//
//			}
//		}
//
//		imshow("Area, Width, Height", result);
//
//
//		key = waitKey(1);
//	}
//
//	destroyAllWindows();
//
//}
////√‚√≥: https://eehoeskrap.tistory.com/280 [Enough is not enough]

