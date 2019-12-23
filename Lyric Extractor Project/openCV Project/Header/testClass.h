#include <opencv2/opencv.hpp>
#include "videoHandler.h"
#include "defines.h"

using namespace cv;
using namespace std;

struct ExtremePoint
{
	Point left;
	Point right;
	Point top;
	Point bottom;
} typedef extremePoint;

class testClass {
public:
	
	//const int KEY_ESC = 27;

	void test_Image();
	void test_Image2();
	void test_Image3();
	void test_Video();

	void test_Video_CountContours();
	void test_Video_CountContours2();
	void test_Video_CountContours3();

	void test_Video_GetContourMask();
	void test_Video_GetContourMask2(string videoPath);

	void test_Video3();

	void print_videoSpec(VideoCapture vc);
	void print_currentFrameSpec(VideoCapture vc);
	void saveImage(Mat image);

	Mat makeGrayscaleWithOneColor(Mat sourceImage, Color color);

	int getWhitePixels(Mat image);

	vector<int> getVerticalProjectionData(Mat binImage);
	Mat getVerticalProjection(Mat binMat);

	vector<int> getHorizontalProjectionData(Mat binImage);
	Mat getHorizontalProjection(Mat sourceImage);

	static Mat GetFloodFilledImage(Mat binMat, bool toBlack);

	Mat getComparedImageBefore(Mat binMat);

	vector<vector<cv::Point>> ContourFilter(vector<vector<cv::Point>> contours);

	Mat contursFilterAlgorism1(Mat ATImage, Mat mergedImage);
	ExtremePoint getExtremePoint(vector<Point> contour);
	vector<vector<Point>> getContursContainExtremepoint(vector<vector<Point>> targetConturs, vector<extremePoint> extremePoints);
	Mat floodFillFilterAlgorism(Mat ATImage, Mat mergedImage);
};