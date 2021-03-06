#include <opencv2/opencv.hpp>
#include "testClass.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>

using namespace std;
using namespace cv;


void decodeBoundingBoxes(const Mat& scores, const Mat& geometry, float scoreThresh,
	std::vector<RotatedRect>& detections, std::vector<float>& confidences)
{
	detections.clear();
	CV_Assert(scores.dims == 4); CV_Assert(geometry.dims == 4); CV_Assert(scores.size[0] == 1);
	CV_Assert(geometry.size[0] == 1); CV_Assert(scores.size[1] == 1); CV_Assert(geometry.size[1] == 5);
	CV_Assert(scores.size[2] == geometry.size[2]); CV_Assert(scores.size[3] == geometry.size[3]);

	const int height = scores.size[2];
	const int width = scores.size[3];
	for (int y = 0; y < height; ++y)
	{
		const float* scoresData = scores.ptr<float>(0, 0, y);
		const float* x0_data = geometry.ptr<float>(0, 0, y);
		const float* x1_data = geometry.ptr<float>(0, 1, y);
		const float* x2_data = geometry.ptr<float>(0, 2, y);
		const float* x3_data = geometry.ptr<float>(0, 3, y);
		const float* anglesData = geometry.ptr<float>(0, 4, y);
		for (int x = 0; x < width; ++x)
		{
			float score = scoresData[x];
			if (score < scoreThresh)
				continue;

			// Decode a prediction.
			// Multiple by 4 because feature maps are 4 time less than input image.
			float offsetX = x * 4.0f, offsetY = y * 4.0f;
			float angle = anglesData[x];
			float cosA = std::cos(angle);
			float sinA = std::sin(angle);
			float h = x0_data[x] + x2_data[x];
			float w = x1_data[x] + x3_data[x];

			Point2f offset(offsetX + cosA * x1_data[x] + sinA * x2_data[x],
				offsetY - sinA * x1_data[x] + cosA * x2_data[x]);
			Point2f p1 = Point2f(-sinA * h, -cosA * h) + offset;
			Point2f p3 = Point2f(-cosA * w, sinA * w) + offset;
			RotatedRect r(0.5f * (p1 + p3), Size2f(w, h), -angle * 180.0f / (float)CV_PI);
			detections.push_back(r);
			confidences.push_back(score);
		}
	}
}

void fourPointsTransform(const Mat& frame, Point2f vertices[4], Mat& result)
{
	const Size outputSize = Size(100, 32);

	Point2f targetVertices[4] = { Point(0, outputSize.height - 1),
								  Point(0, 0), Point(outputSize.width - 1, 0),
								  Point(outputSize.width - 1, outputSize.height - 1),
	};
	Mat rotationMatrix = getPerspectiveTransform(vertices, targetVertices);

	warpPerspective(frame, result, rotationMatrix, outputSize);
}

void decodeText(const Mat& scores, std::string& text)
{
	static const std::string alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";
	Mat scoresMat = scores.reshape(1, scores.size[0]);

	std::vector<char> elements;
	elements.reserve(scores.size[0]);

	for (int rowIndex = 0; rowIndex < scoresMat.rows; ++rowIndex)
	{
		Point p;
		minMaxLoc(scoresMat.row(rowIndex), 0, 0, 0, &p);
		if (p.x > 0 && static_cast<size_t>(p.x) <= alphabet.size())
		{
			elements.push_back(alphabet[p.x - 1]);
		}
		else
		{
			elements.push_back('-');
		}
	}

	if (elements.size() > 0 && elements[0] != '-')
		text += elements[0];

	for (size_t elementIndex = 1; elementIndex < elements.size(); ++elementIndex)
	{
		if (elementIndex > 0 && elements[elementIndex] != '-' &&
			elements[elementIndex - 1] != elements[elementIndex])
		{
			text += elements[elementIndex];
		}
	}
}


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
	Mat orgImage = imread("debug_2.jpg");
	Mat im1 = imageHandler::getResizeAndSubtitleImage(orgImage);
	Mat blue = imageHandler::getPaintedBinImage_inner(im1, true);
	Mat red = imageHandler::getPaintedBinImage_inner(im1, false);

	//int alignContoursCount_blue = getAlinedContoursCount(outImage_blue);
	//int alignContoursCount_red = getAlinedContoursCount(outImage_red);
}

void testClass::test_Image4()
{
	Mat orgImage = imread("pic.png");
	Mat contourImage = orgImage.clone();

	cvtColor(orgImage, orgImage, COLOR_BGR2GRAY);
	//resize(orgImage, orgImage, cv::Size(orgImage.cols*4, orgImage.rows * 4), 0, 0, cv::INTER_CUBIC);
	//threshold(orgImage, orgImage, 200, 255, THRESH_BINARY);	

	// get
	// 4. canny 엣지 검출
	Mat image_canny;
	Canny(orgImage, image_canny, 50, 255);
	string filename = "edge.png";
	imwrite(filename, image_canny);
	saveImage(image_canny);

	// 5. contour 검출
	vector<vector<Point>> contours;
	//findContours(image_canny, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
	//findContours(orgImage, contours, RETR_CCOMP, CHAIN_APPROX_SIMPLE, Point(0, 0));
	findContours(orgImage, contours, RETR_LIST, CHAIN_APPROX_SIMPLE, Point(0, 0));

	vector<vector<Point>> contours_poly(contours.size());
	vector<Rect> boundRect(contours.size());
	vector<Rect> boundRect_filtered;

	for (int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);	// contour -> contourPolyDP
		boundRect[i] = boundingRect(Mat(contours_poly[i]));
		printf("%d size\r\n", boundRect[i].size());
		printf("%d area\r\n", boundRect[i].area());
		printf("%d %d br\r\n", boundRect[i].br().x, boundRect[i].br().y);
		printf("%d %d tl\r\n", boundRect[i].tl().x, boundRect[i].tl().y);
		/*
		if (boundRect[i].width < 6)
			continue;
		else if (boundRect[i].height < 6)
			continue;
		// 가로새로 비율 6배 이상인것 // 큰수/작은수 >6
		int lower, bigger;
		if (boundRect[i].height > boundRect[i].width)
		{
			bigger = boundRect[i].height;
			lower = boundRect[i].width;
		}
		else
		{
			bigger = boundRect[i].width;
			lower = boundRect[i].height;
		}

		if (bigger / lower >= 6)
			continue;
			
		boundRect_filtered.push_back(boundRect[i]);
		*/
		rectangle(contourImage, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0));
	}
	//drawContours(orgImage, contours_poly, -1, Scalar(0, 255, 255), 1);

	// => Blue에서 가장 많은 contur가 걸리는 선x를 구한다.
	// => Red 에서 가장 많은 contur가 걸리는 선x를 구한다.
	// ==> 더 많은 contur가 걸린 컬러영역이 Painted_Lyric이 된다.
	/*
	int biggestCount = 0;
	int biggestCountCol = 0;
	for (int col = 5; col < orgImage.cols; col += 3)
	{
		int count = 0;
		for (int j = 0; j < boundRect_filtered.size(); j++)
		{
			int rectY = boundRect_filtered[j].y;
			int rectHeight = boundRect_filtered[j].height;
			if (rectY < col && rectY + rectHeight > col)	// Rect안에 
				count++;
		}
		if (biggestCount < count)
		{
			biggestCount = count;
			biggestCountCol = col;
		}
	}
	 
	printf("%d \r\n", biggestCount);
	*/

	// 1. 정렬
	// 2. 모든 점 기준으로 진행
	/*
		for(int i=0;i<rect.size(); i++)
		{
			int count=0;		//
			targetRect = rect[i];
			for(intj=i+1; j<rect.size(); j++)
			{
				// x거리
				// y
			}
		}

	*/

	// 컨투어링
	// 사각형만들기
	// 알고리즘
	// 결과도출
	// 좌우 끝점의 대칭정도
	//
	return;
}

void testClass::test_Video_captureFrame(string videoPath)
{
	VideoCapture* vc;
	if (!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();
	fileManager::createDir("saveImage\\" + videoPath);
	Mat orgImage;
	int frame = 1600;

	for (int i = 0; i < 100; i++)
	{
		vc->set(CAP_PROP_POS_FRAMES, (double)(frame+i));
		vc->read(orgImage);
		char s[9] = { 0, };	// 00000000.png
		printf("%08d", i);
		sprintf_s(s, sizeof(s), "%08d", i);

		string filename = "saveImage\\"+videoPath + "\\" + s + ".png";
		imwrite(filename, orgImage);
		// orgImage 저장
	}

	vc->release();

}

void testClass::test_Video(string videoPath)
{
	VideoCapture* vc;
	if(!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;
	int curFrame = 0;

	int vc_fps = (int)vc->get(CAP_PROP_FPS);
	int frameMsec = 1000 / vc_fps;

	while (vc->read(orgImage))
	{
		orgImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		imshow("Video", orgImage);

		//Mat image_gray, image_binAT;
		//cvtColor(orgImage, image_gray, COLOR_BGR2GRAY);
		//adaptiveThreshold(image_gray, image_binAT, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
		//imshow("image_binAT", image_binAT);

		//Mat fullyContrastImage = imageHandler::getSharpenAndContrastImage(orgImage);
		//Mat fullyContrastImage_white;
		//inRange(fullyContrastImage, Scalar(250, 250, 250), Scalar(255, 255, 255), fullyContrastImage_white);
		//imshow("fullyContrastImage_white", fullyContrastImage_white);

		//Mat andImage;
		//bitwise_and(image_binAT, fullyContrastImage_white, andImage);

		videoHandler::printCurrentFrameSpec(*vc);
		//int curFrame = (int)vc->get(CAP_PROP_POS_MSEC);
		//curFrame = (int)vc->get(CAP_PROP_POS_MSEC);
		cout << "curFrame : " << curFrame << endl;
		cout << "curMSec : " << 1000/25*curFrame << endl;
		cout << "CAP_PROP_POS_FRAMES : " << vc->get(CAP_PROP_POS_FRAMES)  << endl;
		cout << "CAP_PROP_POS_MSEC: " << vc->get(CAP_PROP_POS_MSEC) << endl;

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			curFrame -= 1;
		else if (key == 'd')	// next frame
			curFrame += 1;
		else if (key == 'w')	// +50th frame
			curFrame += 50;
		else if (key == 's')	// -50th frame
			curFrame -= 50;
		else if (key == 'r')	// +10th frame
			curFrame += 10;
		else if (key == 'f')	// -10th frame
			curFrame -= 10;
		else if (key == 'e')	// +500th frame
			curFrame += 500;
		else if (key == 'q')	// -500th frame
			curFrame -= 500;
		else if (key == '?')
			videoHandler::printVideoSpec();

		vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);
		//vc->set(CAP_PROP_POS_MSEC, (double)curFrame* frameMsec);	// frame*0.1 msec
	}
	
	vc->release();

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
	VideoCapture* vc;
	if (!videoHandler::setVideo("movie.mp4"))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;
	while (vc->read(orgImage))
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
		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
	}

	vc->release();

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
	VideoCapture* vc;
	if (!videoHandler::setVideo("vie.mp4"))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();
	
	videoHandler::printVideoSpec();

	Mat orgImage;
	while (vc->read(orgImage))
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

		// 4. 부피 계산	getWhitePixelCount(Mat binMat);
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

		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);
		videoHandler::printCurrentFrameSpec(*vc);

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
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
	VideoCapture* vc;
	if (!videoHandler::setVideo("movie1.mp4"))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;
	while (vc->read(orgImage))
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
		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
	}

	vc->release();

}

/*
 1. test_Video_GetContourMask
*/
void testClass::test_Video_GetContourMask()
{
	VideoCapture* vc;
	if (!videoHandler::setVideo("movie1.mp4"))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;
	while (vc->read(orgImage))
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
		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
		else if (key == 'r')	// +10th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 10 - 1);
		else if (key == 'f')	// -10th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 10 - 1);
	}

	vc->release();
}

void testClass::test_Video_GetContourMask2(string videoPath)
{
	VideoCapture* vc;
	if (!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;
	while (vc->read(orgImage))
	{
		orgImage = imageHandler::resizeImageToAnalize(orgImage);
		Rect subRect(0, SUBTITLEAREA_Y, orgImage.cols, SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
		Mat subImage = orgImage(subRect);
		double mask3[3][3] = { {-1,-1,-1}, {-1,9,-1}, {-1,-1,-1} };	// shapen mask
		Mat kernal3 = Mat(3, 3, CV_64FC1, mask3);
		Mat subImage1;
		Mat subImage2;
		Mat subImage3;
		filter2D(subImage, subImage1, -1, kernal3);	// shapen
		filter2D(subImage1, subImage2, -1, kernal3);	// shapen
		filter2D(subImage2, subImage3, -1, kernal3);	// shapen
		imshow("1", subImage);
		imshow("2", subImage1);

		Mat bgr[3];
		split(subImage, bgr);
		imshow("blueC", bgr[0]);

		//
		Mat blur1;
		GaussianBlur(subImage1, blur1, Size(5, 5), 5, 5);
		imshow("3", blur1);
		Mat blur1_gray;
		cvtColor(blur1, blur1_gray, COLOR_BGR2GRAY);
		Mat sobel1_x, sobel1_y;
		Sobel(blur1_gray, sobel1_x, CV_16S, 1, 0, 3);
		convertScaleAbs(sobel1_x, sobel1_x);
		Sobel(blur1_gray, sobel1_y, CV_16S, 0, 1, 3);
		convertScaleAbs(sobel1_y, sobel1_y);
		Mat gred;
		addWeighted(sobel1_x, 0.5, sobel1_y, 0.5, 0, gred);
		imshow("sobel", gred);

		Mat blur2;
		medianBlur(blur1, blur2, 5);
		double maskBlur[3][3] = { {1,1,1}, {1,8,1}, {1,1,1} };	// 
		Mat kernalBlur = Mat(3, 3, CV_64FC1, maskBlur);
		Mat blur3;
		filter2D(blur2, blur3, -1, kernalBlur);
		Mat blur4;
		filter2D(blur3, blur4, -1, kernalBlur);

		Mat image_binAT;
		Mat image_binAT3, image_binAT7;
		Mat image_gray;
		cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
		adaptiveThreshold(image_gray, image_binAT, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 9, 5);
		adaptiveThreshold(image_gray, image_binAT3, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 9, 5);
		adaptiveThreshold(image_gray, image_binAT7, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 9, 5);
		imshow("image_binAT3", image_binAT3);
		imshow("image_binAT", image_binAT);
		imshow("image_binAT7", image_binAT7);
		Mat image_moph_AT = image_binAT;
		// 3-1 노이즈 제거
		//Mat element5(7, 7, CV_8U, Scalar(1));
		//element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));	// @이 값 조정해서 지난 텍스트가 사라지지 않도록 조정해보자
		//erode(image_binAT.clone(), image_moph_AT, element5);	// 침

		Mat subImage_hsv_forFA;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색(파랑, 빨강), 
		cvtColor(subImage, subImage_hsv_forFA, COLOR_BGR2HSV);
		inRange(subImage_hsv_forFA, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv_forFA);

		Mat image_moph_AT_Not;
		bitwise_not(image_moph_AT, image_moph_AT_Not);

		Mat image_floodFilled_AT = GetFloodFilledImage(image_moph_AT.clone(), true);
		Mat image_floodFilled_AT2 = GetFloodFilledImage(image_floodFilled_AT.clone(), false);
		imshow("image_floodFilled_AT2", image_floodFilled_AT2);

		Mat image_floodFilled_AT2_Not;
		bitwise_not(image_floodFilled_AT2, image_floodFilled_AT2_Not);

		//imshow("image_moph_AT_Not", image_moph_AT_Not);	// 이 이미지랑
		imshow("image_floodFilled_AT2_Not", image_floodFilled_AT2_Not);	// 이 이미지랑

		// 4. canny 엣지 검출
		Mat image_canny;
		Canny(image_floodFilled_AT2_Not, image_canny, 250, 255);
		// 5. contour 검출
		vector<vector<cv::Point>> contours;
		findContours(image_canny, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
		//printf("Contours Count: %3d\r\n", contours.size());

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

		Mat subImage_hsv, subImage_hsv1;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색, 
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		cvtColor(subImage1, subImage_hsv1, COLOR_BGR2HSV);
		Mat image_HSV_S, image_HSV_S1;
		inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), image_HSV_S);
		//inRange(subImage_hsv1, Scalar(0, 0, 100), Scalar(255, 150, 255), image_HSV_S1);	흰색 범위
		inRange(subImage_hsv1, Scalar(0, 170, 100), Scalar(255, 255, 255), image_HSV_S1);
		imshow("image_HSV_S", image_HSV_S);
		imshow("image_HSV_S1", image_HSV_S1);

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
		printf("WhitePixelCount: %d\r\n",getWhitePixels(image_merged));

		Mat image_sontursFilterAlgorism1;	// getBinImageByFloodfillAlgorism
		Mat image_sontursFilterAlgorism2;	// getBinImageByFloodfillAlgorism
		image_sontursFilterAlgorism1 = floodFillFilterAlgorism(image_floodFilled_AT2_Not, image_merged);	// at 대신
		image_sontursFilterAlgorism2 = floodFillFilterAlgorism(subImage_hsv_forFA, image_merged);
		// 1. image_sontursFilterAlgorism2 floodfill
		Mat image_sontursFilterAlgorism2_floodfile = GetFloodFilledImage(image_sontursFilterAlgorism2, true);
		// 2. 위 이미지분석 (흰점의 좌측, 우측 점 중 작은것)
		int maskImage_leftist_x = getLeftistWhitePixel_x(image_sontursFilterAlgorism2_floodfile);
		int maskImage_rightest_x = getRightistWhitePixel_x(image_sontursFilterAlgorism2_floodfile);
		int maskImage_middle_x = image_sontursFilterAlgorism2_floodfile.cols/2;//(maskImage_leftist_x + maskImage_rightest_x) / 2;


		imshow("subImage", subImage);

		imshow("image_binIR_RGB_R", image_binIR_RGB_R);
		imshow("image_binIR_HSV_R", image_binIR_HSV_R);

		imshow("image_binIR_RGB_B", image_binIR_RGB_B);
		imshow("image_binIR_HSV_B", image_binIR_HSV_B);

		imshow("image_binIR_Red", image_binIR_Red);
		imshow("image_binIR_Blue", image_binIR_Blue);

		imshow("image_merged", image_merged);
		imshow("image_sontursFilterAlgorism1", image_sontursFilterAlgorism1);
		imshow("image_sontursFilterAlgorism2", image_sontursFilterAlgorism2);
		imshow("image_sontursFilterAlgorism2_floodfile", image_sontursFilterAlgorism2_floodfile);


		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);
		else if (key == 'r')	// +10th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 10 - 1);
		else if (key == 'f')	// -10th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 10 - 1);
		else if (key == 'e')	// +500th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 500 - 1);
		else if (key == 'q')	// -500th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 500 - 1);
		else if (key == '?')
			videoHandler::printVideoSpec();
	}

	vc->release();
}

void testClass::test_Video3()
{
	VideoCapture* vc;
	if (!videoHandler::setVideo("movie1.mp4"))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();
	
	Mat orgImage;
	Mat beforeBinImage;
	while (vc->read(orgImage))
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

		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);
		videoHandler::printCurrentFrameSpec(*vc);

		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1 - 1);
		else if (key == 'd')	// next frame
			; // vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 1);
		else if (key == 'w')	// +50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame + 50 - 1);
		else if (key == 's')	// -50th frame
			vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 50 - 1);

		beforeBinImage = adopt.clone();
	}
}

void testClass::test_Video4(string videoPath)
{
	double sigma = 5, threshold = 0, amount = 5;		// setting (for sharpen)
	VideoCapture* vc;
	if (!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;
	Mat stackBinImage;
	Mat FCImage_before;
	Mat bin_before;
	Mat testBinBefore;
	//Mat testmask = imread("Line9_Bin.jpg", IMREAD_GRAYSCALE);
//	Mat asd = imageHandler::getNoiseRemovedImage(testmask, false);
	//bitwise_not(testmask, testmask);

	while (vc->read(orgImage))
	{
	videoHandler::printCurrentFrameSpec(*vc);
	int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

	if (orgImage.rows != 720)
		orgImage = imageHandler::resizeImageToAnalize(orgImage);
	
	Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
	imshow("subImage", subImage);

	Mat testbinImage = imageHandler::getPaintedBinImage(subImage);
	int countW = imageHandler::getWhitePixelCount(testbinImage);
	printf("whiteC:%d \r\n", countW);
	//if(testBinBefore.empty())
	//	testBinBefore = testBin ;
	//Mat TestDifferenceImage = imageHandler::getDifferenceImage(testBin, testBinBefore);
	//testBinBefore = testBin;

	//Mat testimage_dilateion;
	//Mat element(3, 3, CV_8U, Scalar(1));
	//element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
	//erode(TestDifferenceImage, testimage_dilateion, element);	// 침식연산 - 노이즈제거 
	//imshow("testimage_dilateion", testimage_dilateion);

	/*if (stackBinImage.empty() == true)
		stackBinImage = imageHandler::getCompositeBinaryImages(subImage);*/

	Mat CompositeBinaryImage = imageHandler::getCompositeBinaryImages(subImage);
	imshow("CompositeBinaryImage", CompositeBinaryImage);
	printf("whiteCount : %d \r\n", imageHandler::getWhitePixelCount(CompositeBinaryImage));
	//if (beforeFCImage.empty() == true)
		
	

	// sharpen image using "unsharp mask" algorithm
	Mat blurred; 
	GaussianBlur(subImage, blurred, Size(), sigma, sigma);
	imshow("blurred", blurred);
	Mat lowContrastMask = abs(subImage - blurred) < threshold;
	Mat sharpened = subImage * (1 + amount) + blurred * (-amount);
	subImage.copyTo(sharpened, lowContrastMask);

	imshow("sharpened", sharpened);
	imshow("lowContrastMask", lowContrastMask);

	Mat fullContrastimage;
	fullContrastimage = getFullContrastIMage(sharpened);
	imshow("fullContrastimage", fullContrastimage);
	
	Mat fullContrastimage_NotBulr;
	fullContrastimage_NotBulr = getFullContrastIMage(subImage);
	imshow("fullContrastimage_NotBulr", fullContrastimage_NotBulr);

	Mat fullContrastimage_NotBulr_bin;
	inRange(fullContrastimage_NotBulr, Scalar(254, 254, 254), Scalar(255, 255, 255), fullContrastimage_NotBulr_bin);
	imshow("fullContrastimage_NotBulr_bin", fullContrastimage_NotBulr_bin);

	if (bin_before.empty() == true)
	{
		bin_before = fullContrastimage_NotBulr_bin;
	}
	Mat DifferenceImage = imageHandler::getDifferenceImage(fullContrastimage_NotBulr_bin, bin_before);
	imshow("DifferenceImage", DifferenceImage);

	Mat fullContrastimage_yCbCr;
	fullContrastimage_yCbCr = getFullContrastImage_YCbCr(subImage);
	imshow("fullContrastiㅇmage_yCbCr", fullContrastimage_yCbCr);
	
	if (stackBinImage.empty() == true)
		stackBinImage = DifferenceImage.clone();	//dummy
	if (FCImage_before.empty() != true)
	{
		//stackFCImage(fullContrastimage_NotBulr, FCImage_before, stackBinImage);
		//imshow("stackBinImage", stackBinImage);
	}		

	Mat algorismMk1 = AlgolismMk1(fullContrastimage_NotBulr);
	imshow("AlgolismMk1", algorismMk1);

	Mat algorismMk2 = imageHandler::getPaintedBinImage_inner(fullContrastimage_NotBulr, true);
	imshow("algorismMk2", algorismMk2);

	Mat algoANDDiff;
	bitwise_and(algorismMk1, DifferenceImage, algoANDDiff);
	imshow("algoANDDiff", algoANDDiff);


	Mat reTouch;
	GaussianBlur(fullContrastimage, reTouch, Size(3, 3), 2);
	imshow("reTouch", reTouch);

	Mat blurred1;
	GaussianBlur(reTouch, blurred1, Size(), sigma, sigma);
	Mat lowContrastMask1 = abs(reTouch - blurred1) < threshold;
	Mat sharpened1 = reTouch * (1 + amount) + blurred1 * (-amount);
	reTouch.copyTo(sharpened1, lowContrastMask1);

	Mat fullContrastimage1;
	fullContrastimage1 = getFullContrastIMage(sharpened1);
	imshow("fullContrastimage1", fullContrastimage1);


	Mat subImage_hsv;
	Mat subImage_gray;
	cvtColor(fullContrastimage1, subImage_gray, COLOR_BGR2GRAY);
	cvtColor(fullContrastimage1, subImage_hsv, COLOR_BGR2HSV);
	Mat hsvfiltered;
	inRange(subImage_hsv, Scalar(0, 0, 254), Scalar(255, 255, 255), hsvfiltered);

	Mat outImage;
	outImage = removeLint(hsvfiltered, subImage_gray);
	imshow("outImage", outImage);


	//int key = waitKey(1);
	int key = waitKey(0);
	if (key == KEY_ESC)
		break;
	else if (key == 'a')	// before frame
		curFrame -= 1;
	else if (key == 'd')	// next frame
		curFrame += 1;
	else if (key == 'w')	// +50th frame
		curFrame += 50;
	else if (key == 's')	// -50th frame
		curFrame -= 50;
	else if (key == 'r')	// +10th frame
		curFrame += 10;
	else if (key == 'f')	// -10th frame
		curFrame -= 10;
	else if (key == 'e')	// +500th frame
		curFrame += 500;
	else if (key == 'q')	// -500th frame
		curFrame -= 500;
	else if (key == '?')
		videoHandler::printVideoSpec();
	else if (key == '7')
		sigma+=1;
	else if (key == '4')
		sigma-=1;
	else if (key == '8')
		threshold+=1;
	else if (key == '5')
		threshold-=1;
	else if (key == '9')
		amount+=1;
	else if (key == '6')
		amount-=1;
	else if( key== '0')
		sigma = 5, threshold = 0, amount = 5;

	vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);
	printf("sigma=%f, threshold=%f, amount=%f\r\n", sigma, threshold, amount);

	FCImage_before = fullContrastimage_NotBulr.clone();
	bin_before = fullContrastimage_NotBulr_bin.clone();
	}

vc->release();
}

Mat testClass::getFullContrastIMage(Mat srcImage)
{
	// RGB 분리
	Mat bgr[3];
	split(srcImage, bgr);
	// 각각의 128 임계로 한 이미지 얻음
	threshold(bgr[0], bgr[0], 127, 255, THRESH_BINARY);
	threshold(bgr[1], bgr[1], 127, 255, THRESH_BINARY);
	threshold(bgr[2], bgr[2], 127, 255, THRESH_BINARY);
	// RGB 합침
	Mat mergedImage;
	merge(bgr, 3, mergedImage);
	return mergedImage;
}

Mat testClass::getFullContrastImage_YCbCr(Mat srcImage)
{
	Mat yCbCrMat;
	cvtColor(srcImage, yCbCrMat, COLOR_BGR2YCrCb);
	vector<Mat> ycrcb_planes;

	split(yCbCrMat, ycrcb_planes);
	float alpha = (200 - 100) / 10;	// alpha = (x-100) / 10;
	ycrcb_planes[0] = ycrcb_planes[0] + (ycrcb_planes[0] - 128) * alpha;

	Mat dst_ycrcb;
	merge(ycrcb_planes, dst_ycrcb);

	Mat dst;
	cvtColor(dst_ycrcb, dst, COLOR_YCrCb2BGR);

	return dst;
}

Mat testClass::removeLint(Mat srcImage, Mat refImage)
{
	int height = srcImage.rows;
	int width = srcImage.cols;
	Mat outLintedImage = srcImage.clone();

	for (int y = 1; y < height-1; y++)
	{
		uchar* yPtr = srcImage.ptr<uchar>(y);
		uchar* yPtr_up = srcImage.ptr<uchar>(y-1);
		uchar* yPtr_down = srcImage.ptr<uchar>(y+1);
		for (int x = 1; x < width - 1; x++)
		{
			if (refImage.ptr<uchar>(y)[x] == 255)	// 완전 흰색인경우 연산에서 제외
				continue;

			bool isLint = false;
			if (0 != yPtr[x])	// 흑색이 아닌경우
			{
				if (yPtr[x - 1] == 0 && yPtr[x + 1] == 0)	// 좌 우
					isLint = true;
				else if (yPtr_up[x] == 0 && yPtr_down[x] == 0) // 상 하
					isLint = true;
				else if (yPtr_up[x - 1] == 0 && yPtr_down[x + 1] == 0)	// 좌상 우하
					isLint = true;
				else if (yPtr_down[x - 1] == 0 && yPtr_up[x + 1] == 0) // 좌하 우상
					isLint = true;

				if (isLint)
					outLintedImage.ptr<uchar>(y)[x] = 0;
			}
		}
	}
	return outLintedImage;
}

/*
라인 찾아내는 조건
 - 조건1. 갑자기 diff Dot 수가 증가 
 - 조건2. diff Dot들의 hight히스토그램에서 특정부분에 군집됨
 - (모든 프레임을 기록하고 가장 큰 변화가 생기는 지점 순으로 두 조건이 맞는 곳을 분석한다.)
*/
void testClass::test_getTypeRoutin2(string videoPath)
{
	vector<pair<int, int>> diffDotCount;	// <frame, count>
	vector<Scalar> vecPrintTypes;
	vecPrintTypes.push_back(Scalar(255, 0, 0));	// Blue
	vecPrintTypes.push_back(Scalar(0, 0, 255));	// Red
	vecPrintTypes.push_back(Scalar(255, 0, 255));	// Purple

	Mat stackBinImage;
	Mat stackBinImage_before, stackBinImage_diff;
	Mat FCImage_before;
	Mat subImage_before;
	VideoCapture* vc;
	if (!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;

	while (vc->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);

		Mat binImage = imageHandler::getPaintedBinImage(subImage);
		imshow("subImage", subImage);
		imshow("fullyContrastImage", fullyContrastImage);
		imshow("binImage", binImage);
		Scalar color(255, 0, 0);
		Mat FC_withDilate = imageHandler::getFullyContrast_withDilate(subImage, color);
		imshow("FC_withDilate", FC_withDilate);


		if (stackBinImage.empty() == true)
			stackBinImage = Mat::zeros(subImage.rows, subImage.cols, CV_8U);	//dummy
		if (FCImage_before.empty() != true)
		{
			stackBinImage_before = stackBinImage.clone();
			imageHandler::stackFCImage_BlackToWhite(subImage, subImage_before, stackBinImage);
			//stackBinImage_diff = imageHandler::getDifferenceImage(stackBinImage, stackBinImage_before);
			stackBinImage_diff = imageHandler::getWhiteToBlackImage(stackBinImage_before, stackBinImage);
			stackBinImage_diff = imageHandler::getBorderFloodFilledImage(stackBinImage_diff);
			imshow("stackBinImage", stackBinImage);	// stackBinImage 이거의 화이트카운트
			imshow("stackBinImage_diff", stackBinImage_diff);
			// stackBinImage_diff 노이즈 연산
			Mat diff_Denoise = imageHandler::removeNotLyricwhiteArea(stackBinImage_diff);
			imshow("diff_Denoise", diff_Denoise);

			int dotCount = imageHandler::getWhitePixelCount(diff_Denoise);
			printf("dotCount : %d \r\n", dotCount);
			// diffDotCount.push_back(make_pair(curFrame, dotCount))
			
			Mat HorizontalProjection = getHorizontalProjection(stackBinImage_diff);
			imshow("HorizontalProjection", HorizontalProjection);
			vector<int> verticalProjectionData = imageHandler::getHorizontalProjectionData(stackBinImage_diff);

			Mat VerticalProjection = getVerticalProjection(stackBinImage_diff);
			imshow("VerticalProjection", VerticalProjection);

			// get mask image
		}

		//Mat bluePatternBin = imageHandler::getPaintedPattern(subImage, Scalar(255, 0, 0));
		//Mat redPatternBin = imageHandler::getPaintedPattern(subImage, Scalar(0, 0, 255));
		//imshow("bluePatternBin", bluePatternBin);
		//imshow("redPatternBin", redPatternBin);

		//Mat FC_blue;
		//inRange(fullyContrastImage, Scalar(254, 0, 0), Scalar(255, 0, 0), FC_blue);	// 파랑만이미지
		//cvtColor(FC_blue, FC_blue, COLOR_GRAY2BGR);
		//Mat bluePatternFullfill;
		//bluePatternFullfill = imageHandler::getFloodfillImage(FC_blue, bluePatternBin);
		//imshow("bluePatternFullfill", bluePatternFullfill);
		//Mat erodeImage = imageHandler::getMorphImage(bluePatternFullfill, MORPH_ERODE);	// 침식연산
		//int pixelCount = imageHandler::getWhitePixelCount(erodeImage);
		//
		//printf("pixelCount (blue) : %d \r\n", pixelCount);

		for (int i = 0; i < vecPrintTypes.size(); i++)
		{
			Mat PatternBin = imageHandler::getPaintedPattern(subImage, vecPrintTypes[i]);	// 내부에서 dilate함

			Mat FC_Bin;
			Scalar patternMin = vecPrintTypes[i];
			for (int i = 0; i < 3; i++)
				if (patternMin[i] != 0)
					patternMin[i] = patternMin[i] - 1;

			inRange(fullyContrastImage, patternMin, vecPrintTypes[i], FC_Bin);	// 파랑만이미지
			cvtColor(FC_Bin, FC_Bin, COLOR_GRAY2BGR);
			Mat PatternFullfill;
			PatternFullfill = imageHandler::getFloodfillImage(FC_Bin, PatternBin);	// FullCont 이미지에 패턴으로 인식한 좌표로 패인트통연산
			PatternFullfill = imageHandler::getBorderFloodFilledImage(PatternFullfill);
			Mat erodeImage = imageHandler::getMorphImage(PatternFullfill, MORPH_ERODE);	// 침식연산
			Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(erodeImage);	// 사각박스있는곳 제거

			int pixelCount = imageHandler::getWhitePixelCount(erodeImage_Denoise);

			printf("pixelCount (colorType=%d) : %d \r\n", i, pixelCount);
			if (i == 0)	// debug	// 0= 파랑, 1=빨강, 2=퍼플
			{
				imshow("PatternFullfill", PatternFullfill);
				imshow("erodeImage", erodeImage);

				Mat maskedImage = getMaskedImage(subImage, PatternFullfill);
				imshow("maskedImage", maskedImage);	// 디버깅 확인용 : 색감 확인용으로 사용가능
			}
			// erode의 결과에서 contour 연산 추가하여 정확도 높일 수 있음


		}
		


		//int countW = imageHandler::getWhitePixelCount(binImage);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			curFrame -= 1;
		else if (key == 'd')	// next frame
			curFrame += 1;
		else if (key == 'w')	// +50th frame
			curFrame += 50;
		else if (key == 's')	// -50th frame
			curFrame -= 50;
		else if (key == 'r')	// +10th frame
			curFrame += 10;
		else if (key == 'f')	// -10th frame
			curFrame -= 10;
		else if (key == 'e')	// +500th frame
			curFrame += 500;
		else if (key == 'q')	// -500th frame
			curFrame -= 500;
		else if (key == '?')
			videoHandler::printVideoSpec();

		vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);

		FCImage_before = fullyContrastImage.clone();
		subImage_before = subImage.clone();
	}

	vc->release();
}
/*
 
*/
void testClass::test_getTypeRoutin(string videoPath)
{
	VideoCapture* vc;
	if (!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;

	while (vc->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
		Mat FC_withDilate = getFullyContrast_withDilate(subImage, Scalar(255, 0, 0));
		Mat BWBPatternImage = getBWBPatternImage(FC_withDilate.clone());	// ## YS - 이거 카운팅해서 가장 높은숫자로 사용?/

		Mat printImage_blue = imageHandler::getPaintedBinImage_inner(FC_withDilate, true);
		Mat printImage_red = imageHandler::getPaintedBinImage_inner(FC_withDilate, false);
		Mat printImage;

		Mat BWBPaintImage;
		Mat FC_WhiteArea;
//		inRange(fullyContrastImage, Scalar(254, 254, 254), Scalar(255, 255, 255), FC_WhiteArea);	// binarize by rgb

		BWBPaintImage = imageHandler::getFloodfillImage(fullyContrastImage.clone(), BWBPatternImage.clone());
		

		//BWBPatternImage // 의 흰점 
	

		imshow("subImage", subImage);
//		imshow("fullyContrastImage", fullyContrastImage);
		imshow("FC_withDilate", FC_withDilate);
		imshow("BWBPatternImage", BWBPatternImage);
//		imshow("printImage_blue", printImage_blue);
//		imshow("printImage_red", printImage_red);
		imshow("BWBPaintImage", BWBPaintImage);
		
		//int countW = imageHandler::getWhitePixelCount(binImage);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			curFrame -= 1;
		else if (key == 'd')	// next frame
			curFrame += 1;
		else if (key == 'w')	// +50th frame
			curFrame += 50;
		else if (key == 's')	// -50th frame
			curFrame -= 50;
		else if (key == 'r')	// +10th frame
			curFrame += 10;
		else if (key == 'f')	// -10th frame
			curFrame -= 10;
		else if (key == 'e')	// +500th frame
			curFrame += 500;
		else if (key == 'q')	// -500th frame
			curFrame -= 500;
		else if (key == '?')
			videoHandler::printVideoSpec();

		vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);

	}

	vc->release();
}


void testClass::test_whiteCountImage(string videoPath)
{
	Mat stackBinImage;
	Mat FCImage_before;
	Mat BWBPatternImage_before;
	VideoCapture* vc;
	if (!videoHandler::setVideo(videoPath))
		return;
	videoHandler::printVideoSpec();
	vc = videoHandler::getVideoCapture();

	Mat orgImage;

	while (vc->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*vc);
		int curFrame = (int)vc->get(CAP_PROP_POS_FRAMES);

		Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
		Mat FC_withDilate = getFullyContrast_withDilate(subImage, Scalar(255, 0, 0));
		// fully contrast에 추가 연산 -->>
		// 1. 이미지에서 파란색만 추출 -> 이진이미지 생성
		// 2. 이진이미지 delite 실행(확산)
		// 3. 2번 이미지에 흰점인곳 && fully contrastd의 흰점이 아닌곳 => 파란색칠
		// 4. 이미지 사용
		Mat BWBPatternImage = getBWBPatternImage(FC_withDilate.clone());	// ## YS - 이거 카운팅해서 가장 높은숫자로 사용?/
		Mat printImage_blue = imageHandler::getPaintedBinImage_inner(FC_withDilate, true);
		Mat printImage_red = imageHandler::getPaintedBinImage_inner(FC_withDilate, false);
		Mat printImage;
		bitwise_or(printImage_blue, printImage_red, printImage);
		
		Mat binImage = imageHandler::getPaintedBinImage(subImage);
		imshow("subImage", subImage);
		imshow("fullyContrastImage", fullyContrastImage);
		imshow("FC_withDilate", FC_withDilate);
		imshow("BWBPatternImage", BWBPatternImage);
		imshow("printImage_blue", printImage_blue);
		imshow("printImage_red", printImage_red);
		imshow("binImage", binImage);

		 

		if (stackBinImage.empty() == true)
			stackBinImage = Mat::zeros(subImage.rows, subImage.cols, CV_8U);	//dummy
		if (FCImage_before.empty() != true && BWBPatternImage_before.empty() != true)
		{
			stackFCImage(FC_withDilate, FCImage_before, stackBinImage, BWBPatternImage_before);
			imshow("stackBinImage", stackBinImage);	// stackBinImage 이거의 화이트카운트
			Mat mergedImage;
			bitwise_and(printImage, stackBinImage, mergedImage);
			imshow("mergedImage", mergedImage);
			imageHandler::getWhitePixelCount(stackBinImage);
		}


		//int countW = imageHandler::getWhitePixelCount(binImage);

		//int key = waitKey(1);
		int key = waitKey(0);
		if (key == KEY_ESC)
			break;
		else if (key == 'a')	// before frame
			curFrame -= 1;
		else if (key == 'd')	// next frame
			curFrame += 1;
		else if (key == 'w')	// +50th frame
			curFrame += 50;
		else if (key == 's')	// -50th frame
			curFrame -= 50;
		else if (key == 'r')	// +10th frame
			curFrame += 10;
		else if (key == 'f')	// -10th frame
			curFrame -= 10;
		else if (key == 'e')	// +500th frame
			curFrame += 500;
		else if (key == 'q')	// -500th frame
			curFrame -= 500;
		else if (key == '?')
			videoHandler::printVideoSpec();

		vc->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);

		FCImage_before = FC_withDilate.clone();
		BWBPatternImage_before = BWBPatternImage.clone();
	}

	vc->release();
}

Mat testClass::getBWBPatternImage(Mat FCImage)
{
	int height = FCImage.rows;
	int width = FCImage.cols;

	Mat outImage_painted;
	outImage_painted = Mat::zeros(FCImage.rows, FCImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_painted = outImage_painted.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = FCImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if (isWhite(yPtr_FCImage[x]))
			{
				int color_m = -1; /// black==2, white==1, other==-1
				int color_p = -1;
				while (colorEnd < width - 1)
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isWhite(v3p))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorStart - 1];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_m = 2;	// B
						}
					}
				}

				if (colorEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorEnd + 2];
						if (isBlack(v3p_))
						{
							color_p = 2;	// B
						}
					}
				}
				if ((color_p == 2 && color_m == 2))	// 검-흰-검 패턴
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
						yPtr_painted[i] = 255;
				}
				x = colorEnd;
			}
		}
	}

	// 열연산
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//if (isWhite(outImage.ptr<uchar>(y)[x]))	// 이미 흰색인곳
			//	continue;

			bool isRight = false;	// 조건만족?
			if (isWhite(FCImage.ptr<Vec3b>(y)[x]))
			{
				int colorStart = y;
				int colorEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (colorEnd < height - 1)	// 어디까지 컬러인가
				{
					//Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					Vec3b v3p = FCImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isWhite(v3p))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = FCImage.ptr<Vec3b>(colorStart - 1)[x];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = FCImage.ptr<Vec3b>(colorStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_m = 2;	// Black
						}
					}
				}

				if (colorEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = FCImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = FCImage.ptr<Vec3b>(colorEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_p = 2;	// B
						}
					}
				}

				if ((color_p == 2 && color_m == 2))	// 검-흰-검
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
					{
						outImage_painted.ptr<uchar>(i)[x] = 255;
					}
				}
				y = colorEnd;
			}
		}
	}

	threshold(outImage_painted, outImage_painted, 200, 255, THRESH_BINARY);

	return outImage_painted;
}

Mat testClass::getMaskedImage(Mat subImage, Mat mask)
{
	int height = subImage.rows;
	int width = subImage.cols;

	Mat outImage_masked = subImage.clone();
	Vec3b blackColor = { 0,0,0 };
	// 행연산
	for (int y = 0; y < height; y++)
	{
			uchar* yPtr_mask = mask.ptr<uchar>(y);	//in
			Vec3b* yPtr_out = outImage_masked.ptr<Vec3b>(y); //
			for (int x = 0; x < width; x++)
			{
				int colorStart = x;
				int colorEnd = x;
				bool isRight = false;	// 조건만족?

				if (yPtr_mask[x]!=255)
				{
					yPtr_out[x] = blackColor;
				}
			}
		}

	return outImage_masked;
}

/// <summary>
/// Stacks the fc image.
/// </summary>
/// <param name="FCImage">Fully Contrast image.</param>
/// <param name="FCImage_before"> Fully Contrast image before.</param>
/// <param name="stackBinImage"> 이전 이진 이미지.</param>
/// <param name="maskImage"> 마스크.</param>
void testClass::stackFCImage(Mat FCImage, Mat FCImage_before, Mat& stackBinImage, Mat maskImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	for (int y = 0; y < height ; y++)
	{
		uchar* yPtr = stackBinImage.ptr<uchar>(y);
		Vec3d* yPtr_FC = FCImage.ptr<Vec3d>(y);
		Vec3d* yPtr_FC_before = FCImage_before.ptr<Vec3d>(y);
		uchar* yPtr_Mask = maskImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
		{
			/*printf("%d ", yPtr_FC_before[x][0]);
			printf("%d ", yPtr_FC_before[x][1]);
			printf("%d \r\n", yPtr_FC_before[x][2]);*/
			/*
			On << mask ==1, before==w, cur==color
			bin이 0일때
				
			*/

			// 누적법
			if (yPtr[x] == 0)	// 0인곳 
			{
				if (yPtr_Mask[x] != 255)	// 패턴에 부합하는곳
					yPtr[x] = 0;
				else if (isWhite(FCImage_before.at<cv::Vec3b>(y, x)) && isBlue(FCImage.at<cv::Vec3b>(y, x)))
				{
					//yPtr[x] = 10;
					yPtr[x] = 255;
				}
				else if (isWhite(FCImage_before.at<cv::Vec3b>(y, x)) && isRed(FCImage.at<cv::Vec3b>(y, x)))
				{
					//yPtr[x] = 10;
					yPtr[x] = 255;
				}
			}
			else
			{
				if (isBlue(FCImage.at<cv::Vec3b>(y, x)) || isRed(FCImage.at<cv::Vec3b>(y, x)))
				{
					//if (yPtr[x] <= 245)	// 증감
					//	yPtr[x] += 10;
					//else
						yPtr[x] = 255;
				}
				else
					yPtr[x] = 0;
			}

			//if (yPtr[x] == 0)
			//{
			//	if (isWhite(FCImage_before.ptr<Vec3d>(y)[x]) && isBlue(FCImage.ptr<Vec3d>(y)[x])
			//		|| isWhite(FCImage_before.ptr<Vec3d>(y)[x]) && isRed(FCImage.ptr<Vec3d>(y)[x]))
			//		yPtr[x] = 255;
			//}getHorizontalProjection
			//else
			//{
			//	if (isBlue(FCImage.ptr<Vec3d>(y)[x]) || isRed(FCImage.ptr<Vec3d>(y)[x]))
			//		;	// 유지
			//	else
			//		yPtr[x] = 0;
			//}

		}
	}
	;
}

void testClass::stackFCImage_BlackToWhite(Mat subImage, Mat subImage_before, Mat& stackBinImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	int newDot = 0;
	int delDot = 0;
	Mat subImage_black;
	Mat subImage_white;
	Mat subImageBefore_black;
	Mat subImageBefore_white;
	inRange(subImage, Scalar(0, 0, 0), Scalar(50, 50, 50), subImage_black);
	inRange(subImage, Scalar(200, 200, 200), Scalar(255, 255, 255), subImage_white);
	inRange(subImage_before, Scalar(0, 0, 0), Scalar(50, 50, 50), subImageBefore_black);
	inRange(subImage_before, Scalar(200, 200, 200), Scalar(255, 255, 255), subImageBefore_white);

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = stackBinImage.ptr<uchar>(y);
		//Vec3d* yPtr_FC = FCImage.ptr<Vec3d>(y);
		//Vec3d* yPtr_FC_before = FCImage_before.ptr<Vec3d>(y);
		uchar* yPtr_sub_black = subImage_black.ptr<uchar>(y);
		uchar* yPtr_sub_white = subImage_white.ptr<uchar>(y);
		uchar* yPtr_subBefore_black = subImageBefore_black.ptr<uchar>(y);
		uchar* yPtr_subBefore_white = subImageBefore_white.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{

			// 누적법
			if (yPtr[x] == 0)	// 0인곳  On 조건
			{
				if(yPtr_subBefore_black[x]==0 && yPtr_sub_white[x]==255)
				//if (isBlack(FCImage_before.at<cv::Vec3b>(y, x)) && isWhite(FCImage.at<cv::Vec3b>(y, x)))
				{
					yPtr[x] = 255;
					newDot++;
				}
			}
			else
			{
				if(yPtr_sub_white[x]==255)
				//if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	// 유지
				{
					yPtr[x] = 255;
				}
				else  // 0이 아닌곳 Off 함
				{
					yPtr[x] = 0;
					delDot++;
				}
			}

		}
	}
	cout << "newDot = " << newDot << " delDot = " << delDot << endl;
	;
}

bool testClass::isWhite(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 255 && ptr[2] == 255)
		return true;
	return false;
}

bool testClass::isBlack(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool testClass::isBlue(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool testClass::isRed(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 255)
		return true;
	return false;
}

// getPaintedBinImage()
Mat testClass::AlgolismMk1(Mat FCImage)
{
	int height = FCImage.rows;
	int width = FCImage.cols;
	Mat outImage;
	cvtColor(FCImage, outImage, COLOR_BGR2GRAY);
	
	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = FCImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			bool isRight = false;	// 조건만족?
			if(isWhite( yPtr_FCImage[x]) )////if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	
			{
				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;
				while (x - count > 0)	// 아래쪽 색 확인
				{
					Vec3b v3p = yPtr_FCImage[x - count];
					if (isWhite(v3p))
					{
						count++;//x_m--;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (x - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x - (count + 1)]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_m = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (x - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x - (count + 1)]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlack(v3p_))
							{
								color_m = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_m = -1;	// Other Color
						break;
					}
				}

				count = 1;
				while (x+ count < width)	// 위쪽 색 확인
				{
					Vec3b v3p = yPtr_FCImage[x + count];//FCImage.at<cv::Vec3b>(y, x + count);
					if (isWhite(v3p))
					{
						count++;//x_p++;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (x + (count + 1) < width)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_p = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (x + (count + 1) < width)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlack(v3p_))
							{
								color_p = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_p = -1;	// Other Color
						break;
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족
			}

			if (isRight)	// 조건에 만족함
				yPtr[x] = 255;
			else
				yPtr[x] = 0;
		}
	}

	// 열연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = FCImage.ptr<Vec3b>(y); //

		for (int x = 0; x < width; x++)
		{
			if (isWhite(yPtr[x]))	// 이미 흰색인곳
				continue;

			bool isRight = false;	// 조건만족?
			if (isWhite(yPtr_FCImage[x]))
			{
				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (y - count > 0)	// 아래쪽 색 확인
				{
					Vec3b v3p = FCImage.ptr<Vec3b>(y - count)[x];
					if (isWhite(v3p))
					{
						count++;//x_m--;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (y - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = FCImage.ptr<Vec3b>(y - (count + 1))[x];//yPtr_FCImage[x - (count + 1)]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_m = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (y - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = FCImage.ptr<Vec3b>(y - (count + 1))[x]; //yPtr_FCImage[x - (count + 1)]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlack(v3p_))
							{
								color_m = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_m = -1;	// Other Color
						break;
					}
				}

				count = 1;
				while (y + count < height)	// 위쪽 색 확인
				{
					Vec3b v3p = FCImage.ptr<Vec3b>(y + count)[x]; //yPtr_FCImage[x + count];//FCImage.at<cv::Vec3b>(y, x + count);
					if (isWhite(v3p))
					{
						count++;//x_p++;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (y + (count + 1) < height)	// 연속되는지 확인
						{
							Vec3b v3p_ = FCImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_p = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (y + (count + 1) < height)	// 연속되는지 확인
						{
							Vec3b v3p_ = FCImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlack(v3p_))
							{
								color_p = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_p = -1;	// Other Color
						break;
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족
			}

			if (isRight)	// 조건에 만족함
				yPtr[x] = 255;
			//else
			//	yPtr[x] = 0;
		}
		// yPtr[5] = 255;
	}

	return outImage;
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
	//printf("whiteCount:%d\r\n", whiteCount);
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

	double sum = 0;
	for (int i = 0; i < counts.size(); i++)
		sum += counts[i];
	int avg = 0;
	if (counts.size() != 0)
		avg = sum / counts.size();

	int nRows = binMat.rows;
	int nCols = binMat.cols;
	
	Mat outImage;
	binMat.copyTo(outImage);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			outImage.at<uchar>(j, i) = 0;	// init;

			if (counts[i] > j)
			{
				if (j > avg)
					outImage.at<uchar>(j, i) = 255;
				else
					outImage.at<uchar>(j, i) = 255 / 2;
			}
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
	
	double sum = 0;
	for (int i = 0; i < counts.size(); i++)
		sum += counts[i];
	int avg = 0;
	if(counts.size()!=0)
		avg = sum / counts.size();
	
	int nRows = binMat.rows;
	int nCols = binMat.cols;

	Mat outimage;
	binMat.copyTo(outimage);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			outimage.at<uchar>(j, i) = 0;	// init;

			if (counts[j] > i)
			{
				if( i > avg )
					outimage.at<uchar>(j, i) = 255;
				else
					outimage.at<uchar>(j, i) = 255/2;
			}
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

int testClass::getLeftistWhitePixel_x(Mat binImage)
{
	int leftist_x = 0;

	//int height = binImage.rows;
	//int width = binImage.cols;

	for (int width = 0; width < binImage.cols; width++)
	{
		for (int hight = 0; hight < binImage.rows; hight++)
		{
			if (binImage.at<uchar>(hight, width) != 0)
				return width;
		}
	}
	return leftist_x;
}

int testClass::getRightistWhitePixel_x(Mat binImage)
{
	int rightist_x = 0;

	//int height = binImage.rows;
	//int width = binImage.cols;

	for (int width = binImage.cols - 1; width > 0; width--)
	{
		for (int hight = 0; hight < binImage.rows; hight++)
		{
			if (binImage.at<uchar>(hight, width) != 0)
				return width;
		}
	}
	return 	rightist_x = 0;
}

int testClass::getWhitePixelAverage(Mat binImage)
{
	int height = binImage.rows;
	int width = binImage.cols;
	int whiteCount = 0;
	int whitePixelXSum = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// 흑색이 아닌경우
			{
				whiteCount++;
				whitePixelXSum += x;
			}
	}

	if (whiteCount == 0)
		return 0;
	else
		return whitePixelXSum / whiteCount;
}

// Color 딜레이트 연산하는데 흰색부분이 아닌곳만 함
Mat testClass::getFullyContrast_withDilate(Mat rgbImage, Scalar color)
{
	Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);
	
	Scalar targetColorMin = color;
	for (int i = 0; i < 3; i++)
	{
		if (targetColorMin[i] != 0)
			targetColorMin[i] = targetColorMin[i] - 1;
	}
	
	Mat colorMat;
	//inRange(fullyContrastImage, Scalar(254, 0, 0), Scalar(255, 0, 0), blue);	// binarize by rgb
	inRange(fullyContrastImage, targetColorMin, color, colorMat);

	Mat colorMat_dilate = imageHandler::getMorphImage(colorMat, MORPH_DILATE);

	int height = rgbImage.rows;
	int width = rgbImage.cols;

	for (int y = 0; y < height; y++)
	{
		Vec3b* yPtr_FC = fullyContrastImage.ptr<Vec3b>(y);
		uchar* yPtr_colorMat = colorMat_dilate.ptr<uchar>(y);
		
		for (int x = 0; x < width; x++)
		{
			if (yPtr_colorMat[x] == 255 )
			{
				if (!isWhite(yPtr_FC[x]))
				{
					Vec3b& ptr = fullyContrastImage.at<Vec3b>(y, x);
					//ptr[0] = 255;	// B
					//ptr[1] = 0;	// G
					//ptr[2] = 0;	// R
					
					ptr[0] = color[0];
					ptr[1] = color[1];
					ptr[2] = color[2];

				}

			}
		}
	}
	return fullyContrastImage;
}
