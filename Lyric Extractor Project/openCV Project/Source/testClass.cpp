#include <opencv2/opencv.hpp>
#include "testClass.h"

using namespace std;
using namespace cv;

void testClass::test_Image()
{
	Mat orgImage = imread("movie.mp4_Histogram.jpg");
	Mat image_canny;
	Mat houghImg;


	Canny(orgImage, image_canny, 100, 200);

	vector<Vec4i> lines;
	int threshold = 150; // r,θ 평면에서 몇개의 곡선이 한점에서 만났을 때 직선으로 판단할지에 대한 최소값
	HoughLinesP(image_canny, lines, 1, CV_PI / 180, threshold);

	houghImg = orgImage.clone();
	for (size_t i = 0; i < lines.size(); i++)
	{
		Vec4i l = lines[i];
		line(houghImg, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 0, 255), 1);
	}

	imshow("orgImage", orgImage);
	imshow("image_canny", image_canny);
	imshow("houghImg", houghImg);
	//imshow("orgImage2", orgImage2);


	waitKey(0);
}

void testClass::test_Image2()
{
	Mat orgImage = imread("test_movieCapture_3.png");

	int value = 146;
	while (true)
	{
		printf("value = %d\r\n", value);
		Mat image_bin;
		threshold(orgImage, image_bin, value, 255, THRESH_BINARY);

		// 0. cvtColor - 흑백화
		Mat image_gray;
		cvtColor(orgImage, image_gray, COLOR_BGR2GRAY);

		Mat image_gray_bin;
		threshold(image_gray, image_gray_bin, value, 255, THRESH_BINARY);	// 146이 최적값
		Mat image_gray_binOstu;
		threshold(image_gray, image_gray_binOstu, value, 255, THRESH_BINARY|THRESH_OTSU);	
		
		/* RGB 각각의 이진이미지를 구해 AND 연산 함. -> 연산시간 너무 김*/
		Mat image_red_gray, image_green_gray, image_blue_gray;
		image_blue_gray = makeGrayscaleWithOneColor(orgImage, BLUE);
		image_green_gray = makeGrayscaleWithOneColor(orgImage, GREEN);
		image_red_gray = makeGrayscaleWithOneColor(orgImage, RED);
		image_blue_gray = orgImage;
		image_green_gray = orgImage;
		image_red_gray = orgImage;
		Mat image_red_gray_bin, image_green_gray_bin, image_blue_gray_bin;
		threshold(image_red_gray, image_red_gray_bin, value, 255, THRESH_BINARY);	
		threshold(image_green_gray, image_green_gray_bin, value, 255, THRESH_BINARY);	
		threshold(image_blue_gray, image_blue_gray_bin, value-10, 255, THRESH_BINARY);	
		cvtColor(image_red_gray_bin, image_red_gray_bin, COLOR_BGR2GRAY);
		cvtColor(image_green_gray_bin, image_green_gray_bin, COLOR_BGR2GRAY);
		cvtColor(image_blue_gray_bin, image_blue_gray_bin, COLOR_BGR2GRAY);
				
		Mat image_bin_bitwised;
		bitwise_and(image_gray_bin, image_red_gray_bin, image_bin_bitwised);
		bitwise_and(image_bin_bitwised, image_green_gray_bin, image_bin_bitwised);
		bitwise_and(image_bin_bitwised, image_blue_gray_bin, image_bin_bitwised);
		threshold(image_bin_bitwised, image_bin_bitwised, 250, 255, THRESH_BINARY);
		


		//// 1. Canny - Edge 검출 : threshold값 조정
		Mat image_canny;
		Canny(image_bin_bitwised, image_canny, 100, 200);

		//// 2. findContours - 윤곽 배열 얻음
		Mat image_contours;	// = image_canny;
		image_contours = image_canny.clone();
		vector<vector<cv::Point>> contours;
		findContours(image_canny, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);	// Canny Image에서 Contour를 찾음
		//drawContours(image_contours, contours, -1, Scalar(0, 255, 250), 1);
		vector<vector<Point>> contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());

		// Bind rectangle to every rectangle.
		for (int i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}
	
		drawContours(image_contours, contours, -1, Scalar(0, 255, 255));
		for (int i = 0; i < contours.size(); i++)
		{
			rectangle(image_contours, boundRect[i].tl(), boundRect[i].br(), Scalar(255, 0, 0));
		}

		// 3. someAlgorithm - 글자배열로 추려냄 (특정 x축에 연속되는 곳)

		// 4. binarize - 해당 부분만 이진화 하여 글씨만 남은 이미지 추출
		
		//imshow("image_bin_bitwised", image_bin_bitwised);

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			value--;
		else if (key == 'd')	// next frame
			value++;
		else if (key == 'w')	// +50th frame
			value += 10;
		else if (key == 's')	// -50th frame
			value -= 10;
	}

}

void testClass::test_Image3()
{

}

void testClass::test_Video()
{
	VideoCapture vc("movie.mp4");
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}
	videoHandler::printVideoSpec(vc);

	Mat orgImage;

	while (vc.read(orgImage))
	{

		imshow("Video", orgImage);

		videoHandler::printCurrentFrameSpec(vc);
		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 -1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame+50-1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame-50-1);
	}
	
	vc.release();

}

void testClass::test_Video_CountContours()
{
	// 1. 프레임별 반복
	// 2. 가사부분만 자름 (임시 고정값 사용)
	// 3. 파란색 부분만 이진화
	// 4. canny
	// 5. contouring
	// 6. pirnt contour Count
	// 7. Contours 겟수 분석하여 가사가 시작되는 시간 - 가사가 끝나는 시간 배열에 저장 (시간 또는 프레임번호)
	VideoCapture vc("movie.mp4");
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}
	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	while (vc.read(orgImage))
	{
		// 2. 가사부분만 자름 (임시 고정값 사용)
		Rect subRect(0, 475, orgImage.cols, SUBTITLEAREA_LENGTH);
		Mat subImage = orgImage(subRect);
		//HSV로 변환
		Mat subImage_hsv;
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

		// 3. 이진화
		Mat image_getBlue;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
		inRange(subImage_hsv, Scalar(115, 100, 100), Scalar(140, 255, 255), image_getBlue);	// HSV값은 테스트로 얻은 값임...

		// 3-1 노이즈 제거
		Mat element5(5, 5, CV_8U, Scalar(1));
		element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		//Mat image_close;
		//morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);
		Mat image_open;
		morphologyEx(image_getBlue, image_open, MORPH_OPEN, element5);
		
		// 4. canny
		Mat image_canny;
		Canny(image_open, image_canny, 250, 255);
		// 5. contouring
		vector<vector<cv::Point>> contours;
		findContours(image_canny, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);	// Canny Image에서 Contours를 찾음
		// 6. pirnt contour Count
		printf("Contours Count: %3d \r\n", contours.size());
		

		//// 2. findContours - 윤곽 배열 얻음
		Mat image_contours;	// = image_canny;
		image_contours = subImage.clone();
		//drawContours(image_contours, contours, -1, Scalar(0, 255, 250), 1);
		vector<vector<Point>> contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());

		// Bind rectangle to every rectangle.
		for (int i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}
		drawContours(image_contours, contours, -1, Scalar(0, 255, 255));
		for (int i = 0; i < contours.size(); i++)
		{
			rectangle(image_contours, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0));
		}

			   
		imshow("Video", image_canny);
		videoHandler::printCurrentFrameSpec(vc);
		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
	}

	vc.release();

}

void testClass::test_Video_CountContours2()
{
	// 1. 프레임별 반복
	// 2. 가사부분만 자름 (임시 고정값 사용)
	// 3. 이진화
	// 4. canny
	// 5. contouring
	// 6. pirnt contour Count
	// 7. Contours 겟수 분석하여 가사가 시작되는 시간 - 가사가 끝나는 시간 배열에 저장 (시간 또는 프레임번호)
	VideoCapture vc("movie.mp4");
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}
	
	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	while (vc.read(orgImage))
	{
		// 2. 가사부분만 자름 (임시 고정값 사용)
		Rect subRect(0, 475, orgImage.cols, SUBTITLEAREA_LENGTH);
		Mat subImage = orgImage(subRect);
		// 3. 이진화
		// 3-1. grayscale의 이진화
		Mat image_gray;
		cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		Mat image_gray_bin;
		threshold(image_gray, image_gray_bin, 146, 255, THRESH_BINARY);	// 146이 최적값

		// 3-1. Blue(HSV)의 이진화
		Mat subImage_hsv;
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		// 3. 이진화
		Mat image_getBlue;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
		inRange(subImage_hsv, Scalar(115, 100, 100), Scalar(140, 255, 255), image_getBlue);	// HSV값은 테스트로 얻은 값임...

		// 4. 부피 계산	getWihtePixelCount(Mat binMat);
		getWhitePixels(image_getBlue);				
		
		// 3-3. 3-1과 3-2의 And
		Mat image_Mask;
		bitwise_or(image_gray_bin,image_getBlue, image_Mask);
		imshow("image_getBlue", image_getBlue);
		// 3-1 노이즈 제거
		Mat element5(5, 5, CV_8U, Scalar(1));
		element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		//Mat image_close;
		//morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);
		Mat image_dilation;
		dilate(image_Mask, image_dilation, element5);

		imshow("image_gray_bin", image_gray_bin);
		Mat graph;
		graph = getVerticalProjection(image_gray_bin);
		imshow("graph", graph);

		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);
		videoHandler::printCurrentFrameSpec(vc);

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
	}
}

void testClass::test_Video_CountContours3()
{
	// 1. 프레임별 반복
	// 2. 가사부분만 자름 (임시 고정값 사용)
	// 3. adaptiveThreshold로 이진화
	// 4. canny (태두리 검출)
	// 5. contouring 전처리 (발견된 contours의 패턴이 Line이 나타날 때의 패턴인가?)
	// 5. contouring (태두리 검출, 의미있는 태두리만 남김[일정 크기, x,y축이 벗어나지 않는 것 등등] )
	// 6. pirnt contour Count : 
	// 7. 
	VideoCapture vc("movie1.mp4");
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}
	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	while (vc.read(orgImage))
	{
		// 2. 가사부분만 자름 (임시 고정값 사용)
		Rect subRect(0, 475, orgImage.cols, SUBTITLEAREA_LENGTH);
		Mat subImage = orgImage(subRect);

		Mat image_binAdopt;
		Mat image_gray;
		//cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		//adaptiveThreshold(image_gray, image_binAdopt, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
		inRange(subImage, Scalar(195, 195, 195), Scalar(255, 255, 255), image_binAdopt);
		imshow("image_binAdopt", image_binAdopt);

		// 3-1 노이즈 제거
		//Mat element5(5, 5, CV_8U, Scalar(1));
		//element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		////Mat image_close;
		////morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);
		Mat image_morph = image_binAdopt;
		//morphologyEx(image_binAdopt, image_morph, MORPH_OPEN, element5);
		//morphologyEx(image_morph, image_morph, MORPH_CLOSE, element5);
		////erode(image_morph, image_morph, element5);
		//
		//imshow("image_morph", image_morph);

		// 4. canny
		Mat image_canny;
		Canny(image_morph, image_canny, 250, 255);
		// 5. contouring
		vector<vector<cv::Point>> contours;
		findContours(image_canny, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);	// Canny Image에서 Contours를 찾음
		// Contour 필터링
		vector<vector<cv::Point>> filteredContours = ContourFilter(contours);
		// 유효한 contour로 마스크 얻기
		// 마스크로 다시

		// 6. pirnt contour Count
		printf("Contours Count(non filtered): %3d(%d) \r\n", filteredContours.size(), contours.size());


		//// 2. findContours - 윤곽 배열 얻음
		Mat image_contours;	
		image_contours = subImage.clone();
		vector<vector<Point>> contours_poly(filteredContours.size());
		vector<Rect> boundRect(filteredContours.size());

		// Bind rectangle to every rectangle.
		for (int i = 0; i < filteredContours.size(); i++)
		{
			approxPolyDP(Mat(filteredContours[i]), contours_poly[i], 1, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}
		for (int i = 0; i < filteredContours.size(); i++)	// draw
		{
			drawContours(image_contours, filteredContours, -1, Scalar(0, 255, 255), 1);
			//rectangle(image_contours, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0));
			
			Moments mu = moments(filteredContours[i]);
			Point2d mc = Point2d(mu.m10 / mu.m00, mu.m01 / mu.m00);
			Point p = mc;
			p.x += 3; p.y += 3;
			char buf1[100];
			sprintf_s(buf1, "A:%.1f", contourArea(filteredContours[i]));	// 이 값이 특정크기 이하, 이상 삭제
			putText(image_contours, buf1, p, FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 255), 1, 1);
		}


		imshow("image_canny", image_canny);
		imshow("image_contours", image_contours);
		videoHandler::printCurrentFrameSpec(vc);
		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
	}

	vc.release();

}

/*
 1. test_Video_GetContourMask
*/
void testClass::test_Video_GetContourMask()
{
	VideoCapture vc("movie1.mp4");
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}
	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	while (vc.read(orgImage))
	{
		Rect subRect(0, 475, orgImage.cols, SUBTITLEAREA_LENGTH);
		Mat subImage = orgImage(subRect);

		Mat image_binAT, image_binIR;
		Mat image_gray;
		cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		adaptiveThreshold(image_gray, image_binAT, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
		Mat image_moph_AT = image_binAT;
		// 3-1 노이즈 제거
		Mat element5(7, 7, CV_8U, Scalar(1));
		element5 = getStructuringElement(MORPH_ELLIPSE, Point(1, 7));	// @이 값 조정해서 지난 텍스트가 사라지지 않도록 조정해보자
		erode(image_binAT, image_moph_AT, element5);	// 침
		
		Mat image_floodFilled_AT = GetFloodFilledImage(image_moph_AT.clone(), true);
		imshow("image_floodFilled_AT", image_floodFilled_AT);

		inRange(subImage, Scalar(185, 185, 185), Scalar(255, 255, 255), image_binIR);

		//Mat element5(5, 5, CV_8U, Scalar(1));
		element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 5));
		Mat image_floodFilled_IR = GetFloodFilledImage(image_binIR.clone(), true);
		dilate(image_floodFilled_IR, image_floodFilled_IR, element5);	// 팽
		/*morphologyEx(image_floodFilled_IR, image_floodFilled_IR, MORPH_CLOSE, element5);
		morphologyEx(image_floodFilled_IR, image_floodFilled_IR, MORPH_OPEN, element5);*/

		imshow("image_floodFilled_IR", image_floodFilled_IR);

		// 3-1. Blue(HSV)의 이진화
		Mat subImage_hsv;
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		// 3. 이진화
		Mat image_binIR_HSV;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
		inRange(subImage_hsv, Scalar(115, 100, 100), Scalar(140, 255, 255), image_binIR_HSV);	// Blue 검출HSV값은 테스트로 얻은 값임...
		//inRange(subImage_hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), image_binIR_HSV);	// 160-179 HSV값은 테스트로 얻은 값임...
		//inRange(subImage_hsv, Scalar(0, 100, 100), Scalar(0, 255, 255), image_binIR_HSV);	// 160-179 HSV값은 테스트로 얻은 값임...
		//inRange(subImage_hsv, Scalar(0, 0, 65), Scalar(180, 15, 255), image_binIR_HSV);
		element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		erode(subImage_hsv, subImage_hsv, element5);	// 침
		Mat image_floodFilled_IR_HSV = GetFloodFilledImage(image_binIR_HSV.clone(), true);
		imshow("image_floodFilled_IR_HSV", image_floodFilled_IR_HSV);


		Mat image_And;
		bitwise_and(image_floodFilled_IR, image_floodFilled_AT, image_And);
		imshow("image_And", image_And);

		Mat graphCol;
		graphCol = getVerticalProjection(image_And);
		imshow("VerticalProjection", graphCol);

		Mat graphRow;
		graphRow = getHorizontalProjection(image_And);
		imshow("HorizontalProjection", graphRow);


		// 3-1 노이즈 제거
		//Mat element5(5, 5, CV_8U, Scalar(1));
		element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		//Mat image_close;
		//morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);
		Mat image_morph = image_And;
		/*morphologyEx(image_bin, image_morph, MORPH_OPEN, element5);
		morphologyEx(image_morph, image_morph, MORPH_CLOSE, element5);*/
		////erode(image_morph, image_morph, element5);
		//
		//imshow("image_morph", image_morph);

		// 4. canny
		Mat image_canny;
		Canny(image_morph, image_canny, 250, 255);
		// 5. contouring
		vector<vector<cv::Point>> contours;
		findContours(image_canny, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);	// Canny Image에서 Contours를 찾음
		// Contour 필터링
		vector<vector<cv::Point>> filteredContours = ContourFilter(contours);
		// 유효한 contour로 마스크 얻기
		// 마스크로 다시

		// 6. pirnt contour Count
		printf("Contours Count(non filtered): %3d(%d) \r\n", filteredContours.size(), contours.size());


		//// 2. findContours - 윤곽 배열 얻음
		Mat image_contours;
		image_contours = subImage.clone();
		vector<vector<Point>> contours_poly(filteredContours.size());
		vector<Rect> boundRect(filteredContours.size());

		// Bind rectangle to every rectangle.
		for (int i = 0; i < filteredContours.size(); i++)
		{
			approxPolyDP(Mat(filteredContours[i]), contours_poly[i], 1, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}
		for (int i = 0; i < filteredContours.size(); i++)	// draw
		{
			drawContours(image_contours, filteredContours, -1, Scalar(0, 255, 255), 1);
			//rectangle(image_contours, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0));

			Moments mu = moments(filteredContours[i]);
			Point2d mc = Point2d(mu.m10 / mu.m00, mu.m01 / mu.m00);
			Point p = mc;
			p.x += 3; p.y += 3;
			char buf1[100];
			sprintf_s(buf1, "A:%.1f", contourArea(filteredContours[i]));	// 이 값이 특정크기 이하, 이상 삭제
			putText(image_contours, buf1, p, FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 255), 1, 1);
		}


		imshow("image_canny", image_canny);
		imshow("image_contours", image_contours);
		videoHandler::printCurrentFrameSpec(vc);
		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
		else if (key == 'r')	// +10th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 10 - 1);
		else if (key == 'f')	// -10th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 10 - 1);
	}

	vc.release();
}

void testClass::test_Video_GetContourMask2(string videoPath)
{
	VideoCapture vc(videoPath);
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}
	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	while (vc.read(orgImage))
	{
		Rect subRect(0, 475, orgImage.cols, SUBTITLEAREA_LENGTH);
		Mat subImage = orgImage(subRect);

		Mat image_binAT;
		Mat image_gray;
		cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		adaptiveThreshold(image_gray, image_binAT, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
		Mat image_moph_AT = image_binAT;
		// 3-1 노이즈 제거
		//Mat element5(7, 7, CV_8U, Scalar(1));
		//element5 = getStructuringElement(MORPH_ELLIPSE, Point(1, 7));	// @이 값 조정해서 지난 텍스트가 사라지지 않도록 조정해보자
		//erode(image_binAT, image_moph_AT, element5);	// 침

		Mat image_floodFilled_AT = GetFloodFilledImage(image_moph_AT.clone(), true);
		Mat image_floodFilled_AT2 = GetFloodFilledImage(image_floodFilled_AT.clone(), false);
		imshow("image_floodFilled_AT2", image_floodFilled_AT2);

		Mat image_floodFilled_AT2_Not;
		bitwise_not(image_floodFilled_AT2, image_floodFilled_AT2_Not);
		imshow("image_floodFilled_AT2_Not", image_floodFilled_AT2_Not);	// 이 이미지랑

		// 4. canny 엣지 검출
		Mat image_canny;
		Canny(image_floodFilled_AT2_Not, image_canny, 250, 255);
		// 5. contour 검출
		vector<vector<cv::Point>> contours;
		findContours(image_canny, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
		printf("Contours Count: %3d\r\n", contours.size());

		Mat image_contours;
		image_contours = subImage.clone();

		for (int i = 0; i < contours.size(); i++)	// draw
		{
			drawContours(image_contours, contours, -1, Scalar(0, 255, 255), 1);
		}
		imshow("image_contours", image_contours);
		
		Mat image_binIR_RGB_R;
		inRange(subImage, Scalar(0, 0, 130), Scalar(50, 50, 255), image_binIR_RGB_R);
		Mat image_binIR_RGB_B;
		inRange(subImage, Scalar(140, 0, 0), Scalar(255, 40, 50), image_binIR_RGB_B);

		Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		Mat image_binIR_HSV_R_0;
		Mat image_binIR_HSV_R_1;
		Mat image_binIR_HSV_R;
		inRange(subImage_hsv, Scalar(0, 140, 180), Scalar(1, 255, 255), image_binIR_HSV_R_0);
		inRange(subImage_hsv, Scalar(160, 140, 180), Scalar(179, 255, 255), image_binIR_HSV_R_1);
		bitwise_or(image_binIR_HSV_R_0, image_binIR_HSV_R_1, image_binIR_HSV_R);
		Mat image_binIR_HSV_B;
		inRange(subImage_hsv, Scalar(118, 240, 100), Scalar(140, 255, 255), image_binIR_HSV_B);
		
		Mat image_binIR_HSV_R_Filterd = GetFloodFilledImage(image_binIR_HSV_R.clone(), true);
		Mat image_binIR_RGB_R_Filterd = GetFloodFilledImage(image_binIR_RGB_R.clone(), true);
		Mat image_binIR_HSV_B_Filterd = GetFloodFilledImage(image_binIR_HSV_B.clone(), true);
		Mat image_binIR_RGB_B_Filterd = GetFloodFilledImage(image_binIR_RGB_B.clone(), true);

		Mat image_binIR_Red;
		bitwise_and(image_binIR_RGB_R_Filterd, image_binIR_HSV_R_Filterd, image_binIR_Red);
		Mat image_binIR_Blue;
		bitwise_and(image_binIR_RGB_B_Filterd, image_binIR_HSV_B_Filterd, image_binIR_Blue);
		
		// 두개 or한 이미지로 사용.
		Mat image_merged;
		bitwise_or(image_binIR_Blue, image_binIR_Red, image_merged);

		Mat image_sontursFilterAlgorism1;	// getBinImageByFloodfillAlgorism
		image_sontursFilterAlgorism1 = floodFillFilterAlgorism(image_floodFilled_AT2_Not, image_merged);

		imshow("subImage", subImage);

		imshow("image_binIR_RGB_R", image_binIR_RGB_R);
		imshow("image_binIR_HSV_R", image_binIR_HSV_R);

		imshow("image_binIR_RGB_B", image_binIR_RGB_B);
		imshow("image_binIR_HSV_B", image_binIR_HSV_B);

		imshow("image_binIR_Red", image_binIR_Red);
		imshow("image_binIR_Blue", image_binIR_Blue);

		imshow("image_merged", image_merged);
		imshow("image_sontursFilterAlgorism1", image_sontursFilterAlgorism1);


		videoHandler::printCurrentFrameSpec(vc);
		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
		else if (key == 'r')	// +10th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 10 - 1);
		else if (key == 'f')	// -10th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 10 - 1);
	}

	vc.release();
}

void testClass::test_Video3()
{
	VideoCapture vc("movie1.mp4");
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return;
	}

	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	Mat beforeBinImage;
	while (vc.read(orgImage))
	{
		// 2. 가사부분만 자름 (임시 고정값 사용)
		Rect subRect(0, 475, orgImage.cols, SUBTITLEAREA_LENGTH);
		Mat subImage = orgImage(subRect);
		// 3. 이진화
		// 3-1. grayscale의 이진화
		//Mat image_gray;
		//cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		//Mat image_gray_bin;
		//threshold(subImage, image_gray_bin, 200, 255, THRESH_BINARY);	// 146이 최적값
		//imshow("image_gray_bin", image_gray_bin);

		Mat image_bin_inRange;	// BGR order
		inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), image_bin_inRange);	
		Mat adopt, adopt_g;
		Mat image_gray;
		cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		adaptiveThreshold(image_gray, adopt, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
		imshow("adopt", adopt);
		//imshow("image_bin_inRange", image_bin_inRange);
		//imshow("subImage", subImage);

		//getWhitePixels(image_bin_inRange);


		Mat andImage;
		if (beforeBinImage.empty() == false)
		{
			Mat xorImage;
			bitwise_xor(adopt, beforeBinImage, xorImage);
			imshow("xorImage", xorImage);

			// 3-1 노이즈 제거
			Mat element5(5, 5, CV_8U, Scalar(1));
			element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
			//Mat image_close;
			//morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);
			Mat image_open;
			morphologyEx(xorImage, image_open, MORPH_OPEN, element5);
			imshow("image_open", image_open);

			andImage = xorImage;

			Mat graph_Cols;
			Mat graph_Rows;
			graph_Cols = getVerticalProjection(andImage);
			imshow("graph_Cols", graph_Cols);
			graph_Rows = getHorizontalProjection(andImage);
			imshow("graph_rows", graph_Rows);
		}

		int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);
		videoHandler::printCurrentFrameSpec(vc);

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc.set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);

		beforeBinImage = adopt.clone();
	}
}

void testClass::print_videoSpec(VideoCapture vc)
{
	double fps = vc.get(CAP_PROP_FPS);
	int frameCount = vc.get(CAP_PROP_FRAME_COUNT);
	int frameWidth = vc.get(CAP_PROP_FRAME_WIDTH);
	int frameHeight = vc.get(CAP_PROP_FRAME_HEIGHT);

	printf("Video fps			: %f \r\n", fps);
	printf("Video frameCount	: %d \r\n", frameCount);
	printf("Video frameWidth	: %d \r\n", frameWidth);
	printf("Video frameHeight	: %d \r\n", frameHeight);
	printf("Video expected length : %d \r\n", frameCount);
}

void testClass::print_currentFrameSpec(VideoCapture vc)
{
	double curMsec = vc.get(CAP_PROP_POS_MSEC);
	int curSec = curMsec / 1000;
	int curMin = curSec % 60;
	int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

	printf("current time(msec)	: %02d:%02d.%03d(%.0f) \r\n", curMin, curSec, ((int)curMsec)%1000 , curMsec);
	printf("current frame	: %.0d \r\n", curFrame);
}

void testClass::saveImage(Mat image)
{
	
}

Mat testClass::makeGrayscaleWithOneColor(Mat sourceImage, Color color)
{
	Mat outImage;
	sourceImage.copyTo(outImage);

	int nRows = sourceImage.rows;
	int nCols = sourceImage.cols;
	uchar chBlue, chGreen, chRed;

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			chBlue = (sourceImage.at<cv::Vec3b>(j, i)[0]);
			chGreen = (sourceImage.at<cv::Vec3b>(j, i)[1]);
			chRed = (sourceImage.at<cv::Vec3b>(j, i)[2]);

			outImage.at<cv::Vec3b>(j, i)[0] = sourceImage.at<cv::Vec3b>(j, i)[color];
			outImage.at<cv::Vec3b>(j, i)[1] = sourceImage.at<cv::Vec3b>(j, i)[color];
			outImage.at<cv::Vec3b>(j, i)[2] = sourceImage.at<cv::Vec3b>(j, i)[color];
		}
	}
	return outImage;
}

int testClass::getWhitePixels(Mat image)
{
	int height = image.rows;
	int width = image.cols;
	int whiteCount = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = image.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (0 != yPtr[x])	// 흑색이 아닌경우
				whiteCount++;
	}
	printf("whiteCount:%d\r\n", whiteCount);
	return whiteCount;
}

vector<int> testClass::getVerticalProjectionData(Mat binImage)
{
	vector<int> counts;
	for (int i = 0; i < binImage.cols; i++)	// inital
		counts.push_back(0);

	int nRows = binImage.rows;
	int nCols = binImage.cols;

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			if (binImage.at<uchar>(j, i) != 0)
				counts[i]++;
		}
	}
	return counts;
}

// for visible check
Mat testClass::getVerticalProjection(Mat binMat)
{
	vector<int> counts;
	counts = getVerticalProjectionData(binMat);

	int nRows = binMat.rows;
	int nCols = binMat.cols;
	
	Mat outImage;
	binMat.copyTo(outImage);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			outImage.at<uchar>(j, i) = 0;	// init;

			if(counts[i]>j)
				outImage.at<uchar>(j, i) = 255;	
		}
	}

	return outImage;
}

vector<int> testClass::getHorizontalProjectionData(Mat binImage)
{
	int nRows = binImage.rows;
	int nCols = binImage.cols;

	vector<int> counts;
	for (int i = 0; i < nRows; i++)	// inital
		counts.push_back(0);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			if (binImage.at<uchar>(j, i) != 0)
				counts[j]++;
		}
	}
	return counts;
}

Mat testClass::getHorizontalProjection(Mat binMat)
{
	vector<int> counts;
	counts = getHorizontalProjectionData(binMat);

	int nRows = binMat.rows;
	int nCols = binMat.cols;

	Mat outimage;
	binMat.copyTo(outimage);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			outimage.at<uchar>(j, i) = 0;	// init;

			if (counts[j] > i)
				outimage.at<uchar>(j, i) = 255;
		}
	}

	return outimage;
}

// 상측, 하측 흰색점 발견되는곳마다 floodfill 해서 이미지 리턴
Mat testClass::GetFloodFilledImage(Mat binMat, bool toBlack)
{
	int nRows = binMat.rows;
	int nCols = binMat.cols;

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;
	
	// 상측 
	for (int i = 0; i < nCols; i++)
		if (binMat.at<uchar>(2, i) != color)
			floodFill(binMat, Point(i, 2), color);

	// 좌측
	for (int i = 0; i < nRows; i++)
		if (binMat.at<uchar>(i, 30) != color)
			floodFill(binMat, Point(30, i), color);
	
	// 우측
	for (int i = 0; i < nRows; i++)
		if (binMat.at<uchar>(i, nCols - 30) != color)
			floodFill(binMat, Point(nCols -30, i), color);

	// 아래측
	for (int i = 0; i < nCols; i++)
		if (binMat.at<uchar>(nRows-1, i) != color)
			floodFill(binMat, Point(i, nRows - 1), color);
	
	return binMat;
}

// 이전에 호출한 Mat과 비교하여 차이를 반환함
Mat testClass::getComparedImageBefore(Mat binMat)
{
	static Mat image_before;

	if (image_before.empty())
		image_before = binMat;

	Mat xorImage;
	bitwise_xor(binMat, image_before, xorImage);
	image_before = binMat;
	return xorImage;
}

vector<vector<cv::Point>> testClass::ContourFilter(vector<vector<cv::Point>> contours)
{
	vector<vector<cv::Point>> filteredContours;

	for (int i = 0; i < contours.size(); i++)
	{
		vector<cv::Point> contour = contours[i];
		// 부피가 작은것
		if (contourArea(contour) < 10)
			continue;
		//// 부피가 큰 것
		////if (contourArea(contour) > 10000)
		////	continue;
		//Rect Rect = boundingRect(contour);
		//float aspectRatio = Rect.width / Rect.height;	// 10/2
		//// 가로로 긴 것
		//if (aspectRatio > 7.0)
		//	continue;
		//// 세로로 긴 것
		//if (aspectRatio < 0.2)
		//	continue;
		//if (contourArea(contour) < 1000)
		//		continue;

		printf("%3d Area: %d\r\n", i, (int)contourArea(contour));

		filteredContours.push_back(contour);
	}
	
	return filteredContours;
}

/// <summary>
/// 두 이미지에서 mergedImage에 해당하는 contour가 속한 ATImage의 contour만 남기는 알고리즘
/// </summary>
/// <param name="ATImage">At image.</param>
/// <param name="mergedImage">The merged image.</param>
/// <returns></returns>
Mat testClass::contursFilterAlgorism1(Mat ATImage, Mat mergedImage)
{
	vector<vector<Point>> contours_ATImage;
	vector<vector<Point>> contours_mergedImage;

	// 1. 각각의 Contur를 구함
	Mat image_cannyAT, image_cannyMG;
	Canny(ATImage, image_cannyAT, 250, 255);
	Canny(mergedImage, image_cannyMG, 250, 255);

	findContours(image_cannyAT, contours_ATImage, RETR_LIST, CHAIN_APPROX_SIMPLE);	// Canny Image에서 Contours를 찾음
	findContours(image_cannyMG, contours_mergedImage, RETR_LIST, CHAIN_APPROX_SIMPLE);	// Canny Image에서 Contours를 찾음
	
	// 2.mergeImage의 Conturs의 Extreme Point를 구함
	vector<extremePoint> extremePoints;
	for (int i = 0; i < contours_mergedImage.size(); i++)
	{
		extremePoints.push_back(getExtremePoint(contours_mergedImage[i]));
	}
	// 3. ATImage의 Contur에 속한 Contour들만 따로 뺌 ( ->filteredConturs )
	vector<vector<Point>> filteredConturs = getContursContainExtremepoint(contours_ATImage, extremePoints);

	// 4. 필터링된 컨투어로 이미지를 그려 return
	//Mat filteredImage(filteredConturs);
	Mat filteredImage = ATImage.clone();
	cvtColor(filteredImage, filteredImage, COLOR_GRAY2BGR);

	{
		// 다시!
		// 1. outimage에 mergedImage에 흰색인 점 좌표에 빨간색으로 floodfill() 수행 
		// 2. 빨간색으로 inRange() 하여 다시 흰색 이미지를 얻음
		// 3. return
		int height = filteredImage.rows;
		int width = filteredImage.cols;
		Vec3b whiteColor = { 255, 255, 255 };
		Vec3b redColor = { 0, 0, 255 };
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				Vec3b color = filteredImage.at<Vec3b>(Point(x, y));
				if (mergedImage.at<uchar>(Point(x, y)) == 255 && color== whiteColor)	// 좌표색이 흰색이면
					floodFill(filteredImage, Point(x, y), redColor);
			}
		}
	}

	Mat filteredImageToWhite;
	inRange(filteredImage, Scalar(0, 0, 254), Scalar(0, 0, 255), filteredImageToWhite);	// binarize by rgb

	return filteredImageToWhite;
}

/// <summary>
/// 입력된 Contour의Extreme Point 를 반환 ( Contour의 최상단, 최하단, 최좌단, 최우단의 Point)
/// </summary>
/// <param name="contour">The contour.</param>
/// <returns>ExptremePoint</returns>
ExtremePoint testClass::getExtremePoint(vector<Point> contour)
{
	ExtremePoint ep;

	// compare x axis
	auto val = std::minmax_element(contour.begin(), contour.end(), [](Point const& a, Point const& b) {
		return a.x < b.x;
		});

	std::cout << " leftMost [ " << val.first->x << ", " << val.first->y << " ]" << std::endl;
	std::cout << " RightMost [ " << val.second->x << ", " << val.second->y << " ]" << std::endl;
	ep.left.x = val.first->x;
	ep.left.y = val.first->y;
	ep.right.x = val.second->x;
	ep.right.y = val.second->y;

	// compare y axis
	val = std::minmax_element(contour.begin(), contour.end(), [](Point const& a, Point const& b) {
		return a.y < b.y;
		});

	std::cout << " TopMost [ " << val.first->x << ", " << val.first->y << " ]" << std::endl;
	std::cout << " BottomMost [ " << val.second->x << ", " << val.second->y << " ]" << std::endl;
	ep.top.x = val.first->x;
	ep.top.y = val.first->y;
	ep.bottom.x = val.second->x;
	ep.bottom.y = val.second->y;

	return ep;
}

/// <summary>
/// 입력된 Contours 중 입력된 extremePoint를 포함하는 Contours만 골라서 반환.
/// 망한 함수임 : 모든 Contour가 부피를 갖는 것이 아닌, 줄인 Contour가 존재..
/// </summary>
/// <param name="targetConturs">The target conturs.</param>
/// <param name="extremePoints">The extreme points.</param>
/// <returns></returns>
vector<vector<Point>> testClass::getContursContainExtremepoint(vector<vector<Point>> targetConturs, vector<extremePoint> extremePoints)
{
	vector<vector<Point>> filterdContours;
	for (int i = 0; i < targetConturs.size(); i++)	// 타겟 컨투어 순회
	{
		for (int pointIndex = 0; pointIndex < targetConturs[i].size(); pointIndex++)	// 컨투어의 포인트들 순회
		{
			Point p = targetConturs[i][pointIndex];
			bool isFound = false;

			for (int ep = 0; ep < extremePoints.size(); ep++)
			{
				if (p.x == extremePoints[ep].top.x && p.y == extremePoints[ep].top.y+1 ||
					p.x == extremePoints[ep].bottom.x && p.y == extremePoints[ep].bottom.y-1 ||
					p.x == extremePoints[ep].left.x+1 && p.y == extremePoints[ep].left.y ||
					p.x == extremePoints[ep].right.x-1 && p.y == extremePoints[ep].right.y )
				{
					filterdContours.push_back(targetConturs[i]);
					isFound = true; break;
				}
			}
			if (isFound)
				break;
		}
	}
	printf("TargetContours.size() :%d \r\n", targetConturs.size());
	printf("extremePoints.size() :%d \r\n", extremePoints.size());
	printf("filterdContours.size() :%d \r\n", filterdContours.size());

	return filterdContours;
}

/// <summary>
/// ATImage에 mergedImage의 흰색점인 좌표에 floodfill 연산을 하고 나온 결과물을 반환
/// </summary>
/// <param name="ATImage">AdoptedThresold()의 결과이미지.</param>
/// <param name="mergedImage"> ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV))의 결과이미지.</param>
/// <returns></returns>
Mat testClass::floodFillFilterAlgorism(Mat ATImage, Mat mergedImage)
{
	Mat filteredImage_BGR = ATImage.clone();
	cvtColor(filteredImage_BGR, filteredImage_BGR, COLOR_GRAY2BGR);

	// 1. outimage에 mergedImage에 흰색인 점 좌표에 빨간색으로 floodfill() 수행 
	int height = filteredImage_BGR.rows;
	int width = filteredImage_BGR.cols;
	Vec3b whiteColor = { 255, 255, 255 };
	Vec3b redColor = { 0, 0, 255 };
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Vec3b color = filteredImage_BGR.at<Vec3b>(Point(x, y));
			if (mergedImage.at<uchar>(Point(x, y)) == 255 && color == whiteColor)	// 좌표색이 흰색이면
				floodFill(filteredImage_BGR, Point(x, y), redColor);
		}
	}

	// 2. 빨간색으로 inRange() 하여 다시 흰색 이미지를 얻음
	Mat filteredImageToWhite;
	inRange(filteredImage_BGR, Scalar(0, 0, 254), Scalar(0, 0, 255), filteredImageToWhite);	// binarize by rgb

	return filteredImageToWhite;
}
