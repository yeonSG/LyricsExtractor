#include <opencv2/opencv.hpp>
#include "videoHandler.h"
#include "imageHandler.h"
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
	void test_Video(string videoPath);

	void test_Video_CountContours();
	void test_Video_CountContours2();
	void test_Video_CountContours3();

	void test_Video_GetContourMask();
	void test_Video_GetContourMask2(string videoPath);

	void test_Video3();

	// 깔끔한 마스크 얻기 위한 테스트
	// (흰색기준)
	void test_Video4(string videoPath);	
	Mat getFullContrastIMage(Mat srcImage);
	Mat getFullContrastImage_YCbCr(Mat srcImage);
	Mat removeLint(Mat srcImage, Mat refImage);

	/*
	FullyContrast(FC)된 이미지로 누적
	누적이미지는 흑백이미지이며, 각 픽셀은 다음 규칙을 따른다.
		픽셀이 0일 때 :
			이전 FC픽셀이 흰색-> 다음 FC픽셀이 파랑or빨강이면 Set.
		픽셀이 1일 때 :
			다음 FC픽셀이 파랑or빨강이면 유지.
			다음 FC픽셀이 파랑or빨강이 아니면 Clear.
	*/
	// void testX();
	void stackFCImage(Mat FCImage, Mat FCImage_before ,Mat& stackBinImage);
	bool isWhite(const Vec3b& ptr);
	bool isBlack(const Vec3b& ptr);
	bool isBlue(const Vec3b& ptr);
	bool isRed(const Vec3b& ptr);
	/*
	FullyContrast(FC)된 이미지에서
	흰 점을 찾음
		가로연산
			흰점에서 흰점이 아닐때까지 -한 곳의 색과 흰점에서 흰점이 아닐때까지 +한 곳의 색이
			검-파 또는 검-빨 이면 Set
		세로연산
			흰점에서 흰점이 아닐때까지 -한 곳의 색과 흰점에서 흰점이 아닐때까지 +한 곳의 색이
			검-파 또는 검-빨 이면 Set
	*/
	Mat AlgolismMk1(Mat FCImage);
		


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

	int getLeftistWhitePixel_x(Mat binImage);
	int getRightistWhitePixel_x(Mat binImage);
	int getWhitePixelAverage(Mat binImage);
};