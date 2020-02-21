#include "imageHandler.h"


Mat imageHandler::resizeImageToAnalize(Mat& sourceImage)
{
	Mat resizedImage; 

	if (sourceImage.rows != 720)
	{
		float size = 720 / (float)sourceImage.rows;
		resize(sourceImage, resizedImage, cv::Size(sourceImage.cols * size, 720), 0, 0, cv::INTER_CUBIC);
	}
	else
	{
		resizedImage = sourceImage;
	}
	return resizedImage;
}

/// <summary>
/// 이미지의 가사 부분만 잘라 반환함.
/// 자를 부분은 고정 값이기 때문에 주의해야함 (만약 자를 부분이 동적으로 변한다면 자를 부분 찾을 알고리즘 구현 필요)
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <returns>가사 부분만 자른 이미지</returns>
Mat imageHandler::getSubtitleImage(Mat& sourceImage)
{
	Rect subRect(0, SUBTITLEAREA_Y, sourceImage.cols, sourceImage.rows - SUBTITLEAREA_Y);//SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
	Mat subImage = sourceImage(subRect);
	return subImage;
}

Mat imageHandler::getResizeAndSubtitleImage(Mat& sourceImage)
{
	Mat resizedMat = resizeImageToAnalize(sourceImage);
	Mat subImage = getSubtitleImage(resizedMat);
	return subImage;
}

/// <summary>
/// thresold 함수로 이진화 한 이미지 반환.
/// </summary>
/// <param name="sourceImage">3체널 이미지(컬러).</param>
/// <returns></returns>
Mat imageHandler::getBinaryImage(Mat& sourceImage)
{
	Mat image_gray;
	cvtColor(sourceImage, image_gray, COLOR_BGR2GRAY);
	Mat image_gray_bin;
	threshold(image_gray, image_gray_bin, 146, 255, THRESH_BINARY);	// 146이 테스트로 얻은 최적값
	return image_gray_bin;
}

Mat imageHandler::getBlueColorFilteredBinaryImage(Mat& sourceImage)
{
	// 3-1. Blue(HSV)의 이진화
	Mat hsvImage;
	cvtColor(sourceImage, hsvImage, COLOR_BGR2HSV);
	// 3. 이진화
	Mat image_getBlue;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
	inRange(hsvImage, Scalar(115, 100, 100), Scalar(140, 255, 255), image_getBlue);	// HSV값은 테스트로 얻은 값임.

	return image_getBlue;
}

/// <summary>
/// 모플로지 연산 수행 (Noise 제거)
/// </summary>
/// <param name="sourceImage">이진화 된 이미지.</param>
/// <returns></returns>
Mat imageHandler::getMorphImage(Mat& sourceImage, cv::MorphTypes type)
{
	Mat element(3, 3, CV_8U, Scalar(1));
	element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
	//element = { {0, 1, 0},{1, 1,0},

	Mat image_morp;

	switch (type)
	{
	case MORPH_DILATE:
		dilate(sourceImage, image_morp, element);	// 팽창연산
		break;
	case MORPH_ERODE:
		erode(sourceImage, image_morp, element);	// 팽창연산
		break;
	case MORPH_CLOSE:
		morphologyEx(sourceImage, image_morp, MORPH_CLOSE, element);
		break;
	case MORPH_OPEN:
		morphologyEx(sourceImage, image_morp, MORPH_OPEN, element);
		break;
	}
	//erode()	// 침식연산

	return image_morp;
}


Mat imageHandler::getCannyImageWithBinaryImage(Mat& binImage)
{
	Mat image_canny;
	Canny(binImage, image_canny, 250, 255);
	return image_canny;
}

/// <summary>
/// 바이너리 이미지의 태두리에 floodFill(그림판의 패인트통) 연산을 한 이미지 반환(Noise 제거)
/// </summary>
/// <param name="binaryMat">이진화 이미지</param>
/// <param name="toBlack">true면 태두리를 검정으로 함</param>
/// <returns>테두리에 floodFill 연산의 결과 이미지</returns>
Mat imageHandler::getBorderFloodFilledImage(Mat& binaryMat, bool toBlack)
{
	int nRows = binaryMat.rows;
	int nCols = binaryMat.cols;

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;

	// 상측 
	for (int i = 0; i < nCols; i++)
		if (binaryMat.at<uchar>(2, i) != color)
			floodFill(binaryMat, Point(i, 2), color);

	// 좌측
	for (int i = 0; i < nRows; i++)
		if (binaryMat.at<uchar>(i, 30) != color)
			floodFill(binaryMat, Point(30, i), color);

	// 우측
	for (int i = 0; i < nRows; i++)
		if (binaryMat.at<uchar>(i, nCols - 30) != color)
			floodFill(binaryMat, Point(nCols - 30, i), color);

	// 아래측
	for (int i = 0; i < nCols; i++)
		if (binaryMat.at<uchar>(nRows - 1, i) != color)
			floodFill(binaryMat, Point(i, nRows - 1), color);

	return binaryMat;
}

Mat imageHandler::getBorderFloodFilledImageForColor(Mat& rgbImage, bool toBlack)
{
	int nRows = rgbImage.rows;
	int nCols = rgbImage.cols;

	Vec3b color;
	if (toBlack == true)
	{
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
	}
	else 
	{
		color[0] = 255;
		color[1] = 255;
		color[2] = 255;
	}

	// 상측 
	for (int i = 0; i < nCols; i++)
		if (rgbImage.at<Vec3b>(2, i) != color)
			floodFill(rgbImage, Point(i, 2), color);

	// 좌측
	for (int i = 0; i < nRows; i++)
		if (rgbImage.at<Vec3b>(i, 30) != color)
			floodFill(rgbImage, Point(30, i), color);

	// 우측
	for (int i = 0; i < nRows; i++)
		if (rgbImage.at<Vec3b>(i, nCols - 30) != color)
			floodFill(rgbImage, Point(nCols - 30, i), color);

	// 아래측
	for (int i = 0; i < nCols; i++)
		if (rgbImage.at<Vec3b>(nRows - 1, i) != color)
			floodFill(rgbImage, Point(i, nRows - 1), color);

	return rgbImage;
}

/// <summary>
/// 필터링을 거친 Contours들을 가로 라인으로 뚫었을 때 가장 많이 걸린 개수.
/// </summary>
/// <param name="binImage">The bin image.</param>
/// <returns></returns>
int imageHandler::getAlinedContoursCount(Mat& binImage)
{
	Mat image_debug = binImage.clone();

	// 5. contour 검출
	vector<vector<Point>> contours;
	findContours(binImage, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

	vector<vector<Point>> contours_poly(contours.size());
	vector<Rect> boundRect;
	vector<Rect> boundRect_filtered;

	for (int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);	// contour -> contourPolyDP
		boundRect.push_back(boundingRect(Mat(contours_poly[i])));
		
		rectangle(image_debug, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0));	// forDebug
	}
	boundRect_filtered = getFillteredContours(boundRect);
	
	//drawContours(orgImage, contours_poly, -1, Scalar(0, 255, 255), 1);

	// => Blue에서 가장 많은 contur가 걸리는 선x를 구한다.
	// => Red 에서 가장 많은 contur가 걸리는 선x를 구한다.
	// ==> 더 많은 contur가 걸린 컬러영역이 Painted_Lyric이 된다.
	int biggestCount = 0;
	int biggestCountCol = 0;
	for (int col = 5; col < binImage.cols; col += 3)
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

	 return biggestCount;
}

/// <summary>
/// Gets the filltered contours.
/// </summary>
/// <param name="contoursRect">The contours rect.</param>
/// <returns></returns>
vector<Rect> imageHandler::getFillteredContours(vector<Rect> contoursRect)
{
	vector<Rect> boundRect_filtered;

	for (int i = 0; i < contoursRect.size(); i++)
	{
		if (contoursRect[i].width < 6)
			continue;	// fillter : 너비가 6 이하인 것
		else if (contoursRect[i].height < 6)
			continue;	// fillter : 높이가 6 이하인 것

		int lower, bigger;
		if (contoursRect[i].height > contoursRect[i].width)
		{
			bigger = contoursRect[i].height;
			lower = contoursRect[i].width;
		}
		else
		{
			bigger = contoursRect[i].width;
			lower = contoursRect[i].height;
		}

		if (bigger / lower >= 6)	// 가로새로 비율이 6 이상인 것 
			continue;

		boundRect_filtered.push_back(contoursRect[i]);
	}

	return boundRect_filtered;
}

/// <summary>
/// 깔끔한 MaskImage 얻기 위한 것, 
///  - 가로로 길게 뻗어있는 Object를 삭제
///  - 해당하는 영역 row에 최대 최소 x값을 구해 중앙기준으로 비율이 비슷하다면 노이즈 아닌걸로
/// </summary>
/// <param name="binaryMat">The binary mat.</param>
/// <param name="toBlack">if set to <c>true</c> [to black].</param>
/// <returns></returns>
Mat imageHandler::getNoiseRemovedImage(Mat& binaryMat, bool toBlack)
{
	int nRows = binaryMat.rows;
	int nCols = binaryMat.cols;
	int rightistPoint = getRightistWhitePixel_x(binaryMat);
	int leftistPoint = getLeftistWhitePixel_x(binaryMat);

	threshold(binaryMat, binaryMat, 128, 255, THRESH_BINARY);

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;
		
	for (int row = 0; row < nRows; row++) {
		int count = 0;
		uchar* rowPtr = binaryMat.ptr<uchar>(row);
		for (int i = 0; i < nCols; i++) {
			if (rowPtr[i] != color)
				count++;
			else
				count = 0;

			if (count > 60)	// 60pixel
			{
				Mat rowMat = Mat::zeros(1, nCols, CV_8U);	
				uchar* rowMatPtr = rowMat.ptr<uchar>(0);	// copy row
				for (int c = 0; c < nCols; c++) {
					rowMatPtr[c] = rowPtr[c];
				}
				int rowRightistPoint = getRightistWhitePixel_x(rowMat);
				int rowLeftistPoint = getLeftistWhitePixel_x(rowMat);

				if( (rightistPoint+100 < rowRightistPoint || rowRightistPoint < rightistPoint-100) || 
					(leftistPoint + 100 < rowLeftistPoint || rowLeftistPoint < leftistPoint - 100) )
					floodFill(binaryMat, Point(i, row), color);

				count = 0;
			}
		}
	}


	return binaryMat;
}

/// <summary>
/// 잡영 회피 목적의 흰색 커튼이미지 생성
/// </summary>
/// <param name="cols">The cols.</param>
/// <param name="rows">The rows.</param>
/// <param name="maskLength">커튼의 길이</param>
/// <param name="targetColum">타깃 좌표.</param>
/// <returns></returns>
Mat imageHandler::getColumMaskImage(int cols, int rows, int maskLength, int targetColum)
{
	Mat maskImage = Mat::zeros(rows, cols, CV_8UC1);
	if (maskLength / 2 > targetColum)	
		targetColum = maskLength / 2;	
	else if (cols - (maskLength / 2) < targetColum)	
		targetColum = cols - (maskLength / 2);	
	
	for (int r = 0; r < rows; r++)
	{
		uchar* yPtr = maskImage.ptr<uchar>(r);
		for (int c = targetColum - (maskLength / 2); c < targetColum + (maskLength / 2); c++)
		{
			yPtr[c] = 255;
		}

	}

	return maskImage;
}


/// <summary>
/// hsv와 rgb로 필터링 된 이진 이미지를 반환함
/// ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV)) 
/// </summary>
/// <param name="subImage">The subtitle image.</param>
/// <returns>이진화 된 이미지</returns>
Mat imageHandler::getCompositeBinaryImages(Mat& subImage)
{
	Mat image_merged;
	Mat image_binIR_Red = getCompositeBinaryImagesRed(subImage);
	Mat image_binIR_Blue = getCompositeBinaryImagesBlue(subImage);
	bitwise_or(image_binIR_Blue, image_binIR_Red, image_merged);
	return image_merged;
}

/// <summary>
/// 자막의 빨간색의 hsv와 rgb로 필터링 된 이진 이미지를 반환함
/// ((Red_RGB)AND(Red_HSV))
/// </summary>
/// <param name="subImage">The sub image.</param>
/// <returns>빨간색으로 이진화 된 이미지</returns>
Mat imageHandler::getCompositeBinaryImagesRed(Mat& subImage)
{
	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

	Mat image_binIR_RGB_R;
	inRange(subImage, Scalar(0, 0, 130), Scalar(50, 50, 255), image_binIR_RGB_R);	// binarize by rgb

	Mat image_binIR_HSV_R_0;
	Mat image_binIR_HSV_R_1;
	Mat image_binIR_HSV_R;
	inRange(subImage_hsv, Scalar(0, 140, 180), Scalar(1, 255, 255), image_binIR_HSV_R_0); // binarize by hsv
	inRange(subImage_hsv, Scalar(160, 140, 180), Scalar(179, 255, 255), image_binIR_HSV_R_1); // binarize by hsv
	bitwise_or(image_binIR_HSV_R_0, image_binIR_HSV_R_1, image_binIR_HSV_R);	// 총 빨강색 범위 hue = (0~1 + 160~179)

	Mat image_binIR_HSV_R_Filterd = getBorderFloodFilledImage(image_binIR_HSV_R);
	Mat image_binIR_RGB_R_Filterd = getBorderFloodFilledImage(image_binIR_RGB_R);

	Mat image_binIR_Red;
	bitwise_and(image_binIR_RGB_R_Filterd, image_binIR_HSV_R_Filterd, image_binIR_Red);

	return image_binIR_Red;
}

/// <summary>
/// 자막의 파란색의 hsv와 rgb로 필터링 된 이진 이미지를 반환함
/// ((Blue_RGB)AND(Blue_HSV))
/// </summary>
/// <param name="subImage">The sub image.</param>
/// <returns>파란색의 이진화 된 이미지</returns>
Mat imageHandler::getCompositeBinaryImagesBlue(Mat& subImage)
{
	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

	Mat image_binIR_RGB_B;
	inRange(subImage, Scalar(140, 0, 0), Scalar(255, 40, 50), image_binIR_RGB_B);	// binarize by rgb

	Mat image_binIR_HSV_B;
	inRange(subImage_hsv, Scalar(118, 230, 100), Scalar(140, 255, 255), image_binIR_HSV_B);	// binarize by hsv

	Mat image_binIR_HSV_B_Filterd = getBorderFloodFilledImage(image_binIR_HSV_B);
	Mat image_binIR_RGB_B_Filterd = getBorderFloodFilledImage(image_binIR_RGB_B);
	Mat image_binIR_Blue;
	bitwise_and(image_binIR_RGB_B_Filterd, image_binIR_HSV_B_Filterd, image_binIR_Blue);

	return image_binIR_Blue;
}

/// <summary>
/// 입력된 두개의 이진 이미지의 차이점에 대한 이미지 반환
///	A	B	=	C
///	0	0	=	0
///	0	1	=	1
///	1	0	=	1
///	1	1	=	0
/// </summary>
/// <param name="binImageA">The bin image A.</param>
/// <param name="binImageB">The bin image B.</param>
/// <returns></returns>
Mat imageHandler::getDifferenceImage(Mat& binImageA, Mat& binImageB)
{
	Mat xorImage;
	bitwise_xor(binImageA, binImageB, xorImage);

	return xorImage;
}

/// <summary>
/// 색칠된 부분찾아 흑백으로 표시한 이미지
/// 흰점 : 단순화된 이미지(FullyContrast)에서 흰점 기준 좌우상하 검사함
/// 조건 : 흰점기준으로 한쪽은 빨강or파랑이 2점 이상 연속되고, 반대쪽은 검정이 2점 이상 연속됨 
/// </summary>
/// <param name="srcImage">The source image.</param>
/// <returns></returns>
Mat imageHandler::getPaintedBinImage(Mat& rgbImage)
{
	Mat fullyContrastImage = getFullyContrastImage(rgbImage);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage;
	//cvtColor(fullyContrastImage, outImage, COLOR_BGR2GRAY);
	outImage = Mat::zeros(rgbImage.rows, rgbImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int whiteStart = x;
			int whiteEnd = x;
			bool isRight = false;	// 조건만족?
			if (isWhite(yPtr_FCImage[x]))////if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	
			{
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (whiteEnd < width-1)	// 어디까지 흰색이냐
				{
					Vec3b v3p = yPtr_FCImage[whiteEnd + 1];
					if (isWhite(v3p))
						whiteEnd++;
					else
						break;
				}

				if (whiteStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[whiteStart - 1];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_m = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteStart - 2]; //Vec3b v3p_ = yPtr_FCImage[whiteEnd + 2]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_m = 2;	// Black
						}
					}
				}

				if (whiteEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[whiteEnd + 1];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteEnd + 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_p = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteEnd + 2]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_p = 2;	// Black
						}
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족

				if (isRight)	// 조건에 만족함
				{
					for (int i = whiteStart; i <= whiteEnd; i++)
					{
						if( (i+4>whiteEnd) && (i-4<whiteStart) )
							yPtr[i] = 255;
					}
				}
				x = whiteEnd;
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
			if (isWhite(fullyContrastImage.ptr<Vec3b>(y)[x]))
			{
				int whiteStart = y;
				int whiteEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (whiteEnd < height-1)	// 어디까지 흰색이냐
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(whiteEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					if (isWhite(v3p))
						whiteEnd++;
					else
						break;
				}

				if (whiteStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(whiteStart - 1)[x];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_m = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteStart - 2)[x];
						if (isBlack(v3p_))
						{
							color_m = 2;	// Black
						}
					}
				}

				if (whiteEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(whiteEnd + 1)[x];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_p = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteEnd + 2)[x];
						if (isBlack(v3p_))
						{
							color_p = 2;	// Black
						}
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족

				if (isRight)	// 조건에 만족함
				{
					for (int i = whiteStart; i <= whiteEnd; i++)
					{
						if ((i + 4 > whiteEnd) && (i - 4 < whiteStart))
							outImage.ptr<uchar>(i)[x] = 255;
					}
				}
				y = whiteEnd;
			}
		}
	}

	return outImage;
}

/*
	흰 파(빨) 흰 조합인 곳을 찾아냄
*/
Mat imageHandler::getPaintedBinImage_inner(Mat& rgbImage, bool isBluePaint)
{
	Mat fullyContrastImage = getFullyContrastImage(rgbImage);
	//fullyContrastImage = getBorderFloodFilledImageForColor(fullyContrastImage);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage_painted;
	outImage_painted = Mat::zeros(rgbImage.rows, rgbImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_painted = outImage_painted.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if ((isBlue(yPtr_FCImage[x]) && isBluePaint) || (isRed(yPtr_FCImage[x]) && !isBluePaint))
			{
				int color_m = -1; /// black==2, white==1, other==-1
				int color_p = -1;
				while (colorEnd<width-1)
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if ((isBlue(v3p) && isBluePaint) || (isRed(v3p) && !isBluePaint))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorStart - 1];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_m = 1;	// W
						}
					}
				}

				if (colorEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorEnd + 2];
						if (isWhite(v3p_))
						{
							color_p = 1;	// W
						}
					}
				}
				if ((color_p == 1 && color_m == 1))	// 한쪽이 흰색, 반대쪽도 흰색
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
			if((isBlue(fullyContrastImage.ptr<Vec3b>(y)[x]) && isBluePaint) || (isRed(fullyContrastImage.ptr<Vec3b>(y)[x]) && !isBluePaint))
			{
				int colorStart = y;
				int colorEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (colorEnd < height - 1)	// 어디까지 컬러인가
				{
					//Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];
					if ((isBlue(v3p) && isBluePaint) || (isRed(v3p) && !isBluePaint))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorStart - 1)[x];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(colorStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_m = 1;	// W
						}
					}
				}

				if (colorEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(colorEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_p = 1;	// W
						}
					}
				}

				if ((color_p == 1 && color_m == 1))	// 한쪽이 흰색, 반대쪽도 흰색
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

Mat imageHandler::getFullyContrastImage(Mat rgbImage)
{
	// RGB 분리
	Mat bgr[3];
	split(rgbImage, bgr);
	// 각각의 128 임계로 한 이미지 얻음
	threshold(bgr[0], bgr[0], 127, 255, THRESH_BINARY);
	threshold(bgr[1], bgr[1], 127, 255, THRESH_BINARY);
	threshold(bgr[2], bgr[2], 127, 255, THRESH_BINARY);
	// RGB 합침
	Mat mergedImage;
	merge(bgr, 3, mergedImage);
return mergedImage;
}

Mat imageHandler::getSharpenAndContrastImage(Mat rgbImage)
{			// 빵구매꾸기 위해 두번 함
	Mat sharpenImage;
	sharpenImage = getSharpenImage(rgbImage);
	Mat contrastImage;
	contrastImage = getFullyContrastImage(sharpenImage);

	Mat reTouch;
	GaussianBlur(contrastImage, reTouch, Size(3, 3), 2);
	Mat sharpenImage1;
	sharpenImage1 = getSharpenImage(reTouch);
	Mat contrastImage1;
	contrastImage1 = getFullyContrastImage(sharpenImage1);

	return contrastImage1;
}

Mat imageHandler::getSharpenImage(Mat srcImage)
{
	double sigma = 5, threshold = 0, amount = 5;		// setting (for sharpen)
	// sharpen image using "unsharp mask" algorithm
	Mat blurred;
	GaussianBlur(srcImage, blurred, Size(), sigma, sigma);
	Mat lowContrastMask = abs(srcImage - blurred) < threshold;
	Mat sharpened = srcImage * (1 + amount) + blurred * (-amount);
	//srcImage.copyTo(sharpened, lowContrastMask);

	return sharpened;
}

/// <summary>
/// ATImage에 compositeImage의 흰색점인 좌표에 floodfill 연산을 하고 나온 결과물을 반환
/// </summary>
/// <param name="ATImage">AdoptedThresold()의 결과이미지.</param>
/// <param name="compositeImage"> ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV))의 결과이미지.</param>
/// <returns></returns>
Mat imageHandler::getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage)
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
			if (compositeImage.at<uchar>(Point(x, y)) == 255 && color == whiteColor)	// 좌표색이 흰색이면
				floodFill(filteredImage_BGR, Point(x, y), redColor);
		}
	}
	// 2. 빨간색으로 inRange() 하여 다시 흰색 이미지를 얻음
	Mat filteredImageToWhite;
	inRange(filteredImage_BGR, Scalar(0, 0, 254), Scalar(0, 0, 255), filteredImageToWhite);	// binarize by rgb

	return filteredImageToWhite;
}

// BinImage의 최소, 최대값 구한 후 해당 값 +-10 Vertical까지 자름
Mat imageHandler::cutColumByHistorgram(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);
	int highPoint = 0;
	int lowPoint = projection.size();
	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)
		{
			highPoint = i;
			break;
		}
	}
	for (int i = projection.size()-1; i >= 0; i--)
	{
		if (projection[i] != 0)
		{
			lowPoint = i;
			break;
		}
	}

	if (highPoint == 0)
		return binImage;
	else
	{
		if (highPoint - 10 > 0)
			highPoint = highPoint - 10;
		else
			highPoint = 0;

		if (lowPoint + 10 < binImage.rows)
			lowPoint = lowPoint + 10;
		else
			lowPoint = binImage.rows;

		Rect subRect(0, highPoint, binImage.cols, lowPoint- highPoint);
		Mat subImage = binImage(subRect);
		return subImage;

	}
}

// 이진 이미지에서 발음 부분을 지우기 위한 함수
Mat imageHandler::removeSubLyricLine(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);

	vector<pair<int, int>> islands;
	vector<int> islandsVolum;

	// 섬의 갯수 구함, 
	// 섬이 2개 이상이면서, 가장 아래섬이 흰점수 2등 이면
	// 보조 lyric으로 판별하여 2등 제거
	/*	or 가장 큰 volume의 Lyric만 남김 */
	bool blackZone = false;
	int startPoint = 0;
	int endPoint = 0;
	int volume = 0;

	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)	// Not blackZone
		{
			if (blackZone == true)	// blackZone -> Not BlackZone
			{
				startPoint = i;
			}

			volume += projection[i];
			blackZone = false;
		}
		else // blackZone
		{
			if (blackZone == false)	// Not blackZone -> blackZone
			{	// save island
				islands.push_back(make_pair(startPoint, i));
				islandsVolum.push_back(volume);
				volume = 0;
			}
			//
			blackZone = true;
		}
	}

	Mat maskingImage = binImage.clone();

	if (islands.size() >= 2)
	{
		vector<int> islandsVolum_sort;
		islandsVolum_sort.assign(islandsVolum.begin(), islandsVolum.end());	// copy vector

		sort(islandsVolum_sort.begin(), islandsVolum_sort.end(), greater<int>());

		if (islandsVolum_sort[1] == islandsVolum[islandsVolum.size() - 1])	// 가장 아래섬이 흰점수 2등
		{
			int startPoint = islands[islands.size() - 1].first;
			int finishPoint = islands[islands.size() - 1].second;
			
			for (int i = startPoint; i <= finishPoint; i++)
			{
				for (int c = 0; c < maskingImage.cols; c++)
				{
					if (maskingImage.at<uchar>(i, c) != 0)	// (rows, cols)
						floodFill(maskingImage, Point(c, i), 0);	// (cols, rows)
				}
			}
			return maskingImage;
		}
	}

	return maskingImage;
}

Mat imageHandler::removeNotPrimeryLyricLine(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);

	vector<pair<int, int>> islands;
	vector<int> islandsVolum;

	// 섬의 갯수 구함, 
	// 섬이 2개 이상이면서, 가장 아래섬이 흰점수 2등 이면
	// 보조 lyric으로 판별하여 2등 제거
	/*	or 가장 큰 volume의 Lyric만 남김 */
	bool blackZone = false;
	int startPoint = 0;
	int endPoint = 0;
	int volume = 0;

	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)	// Not blackZone
		{
			if (blackZone == true)	// blackZone -> Not BlackZone
			{
				startPoint = i;
			}

			volume += projection[i];
			blackZone = false;
		}
		else // blackZone
		{
			if (blackZone == false)	// Not blackZone -> blackZone
			{	// save island
				islands.push_back(make_pair(startPoint, i));
				islandsVolum.push_back(volume);
				volume = 0;
			}
			//
			blackZone = true;
		}
	}

	Mat maskingImage = binImage.clone();

	if (islands.size() > 1)
	{
		int biggistIsland = 0;
		int biggistIslandVolume = 0;
		for (int i = 0; i < islandsVolum.size(); i++)
		{
			if (biggistIslandVolume < islandsVolum[i])
			{
				biggistIsland = i;
				biggistIslandVolume = islandsVolum[i];
			}
		}

		int primaryLyricStartPoint = islands[biggistIsland].first;
		int primaryLyricEndPoint = islands[biggistIsland].second;

		for (int i = 0; i < projection.size(); i++)
		{
			if (i<primaryLyricStartPoint || i> primaryLyricEndPoint)
			{
				for (int c = 0; c < maskingImage.cols; c++)
				{
					if (maskingImage.at<uchar>(i, c) != 0)	// (rows, cols)
						floodFill(maskingImage, Point(c, i), 0);	// (cols, rows)
				}
			}
		}
	}

	return maskingImage;
}

bool imageHandler::isWhite(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 255 && ptr[2] == 255)
		return true;
	return false;
}

bool imageHandler::isBlack(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool imageHandler::isBlue(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool imageHandler::isRed(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 255)
		return true;
	return false;
}

/// <summary>
/// 이진 이미지의 Vertical projection 데이터로 변환 
/// </summary>
/// <param name="binImage">변환할 이진 이미지.</param>
/// <returns>Vertical projection 데이터</returns>
vector<int> imageHandler::getVerticalProjectionData(Mat binImage)
{
	int nRows = binImage.rows;
	int nCols = binImage.cols;

	vector<int> counts;
	for (int i = 0; i < binImage.cols; i++)	// inital
		counts.push_back(0);

	for (int j = 0; j < nRows; j++) {
		uchar* rowPtr = binImage.ptr<uchar>(j);
		for (int i = 0; i < nCols; i++) {
			if (rowPtr[i] != 0)
				//if (binImage.at<uchar>(j, i) != 0)
				counts[i]++;
		}
	}
	return counts;
}

/// <summary>
/// 이진 이미지의 Horizontal projection 데이터로 변환 
/// </summary>
/// <param name="binImage">변환할 이진 이미지.</param>
/// <returns>Horizontal projection 데이터</returns>
vector<int> imageHandler::getHorizontalProjectionData(Mat binImage)
{
	int nRows = binImage.rows;
	int nCols = binImage.cols;

	vector<int> counts;
	for (int i = 0; i < binImage.rows; i++)	// inital
		counts.push_back(0);

	for (int j = 0; j < nCols; j++) {
		for (int i = 0; i < nRows; i++) {
			if (binImage.at<uchar>(i, j) != 0)
				counts[i]++;
		}
	}
	return counts;
}

/// <summary>
/// Mat의 흰색 점의 개수를 반환함
/// </summary>
/// <param name="binImage">The binary mat.</param>
/// <returns>흰색 점의 개수</returns>
int imageHandler::getWihtePixelCount(Mat binImage)
{
	int height = binImage.rows;
	int width = binImage.cols;
	int whiteCount = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// 흑색이 아닌경우
				whiteCount++;
	}
	printf("whiteCount:%d\r\n", whiteCount);
	return whiteCount;
}

int imageHandler::getLeftistWhitePixel_x(Mat binImage)
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

int imageHandler::getRightistWhitePixel_x(Mat binImage)
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

/// <summary>
/// Gets the rightist white pixel x.
/// </summary>
/// <param name="binImage">The bin image.</param>
/// <param name="targetStartX">대상이 될시작지점.</param>
/// <param name="range">대상 x 부터 +range까지 검사.</param>
/// <param name="threshold"> 대상이 될 col의 흰점 최소 갯수.</param>
/// <returns></returns>
int imageHandler::getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold)
{
	// get Vertical Projection
	// 대상 x 부터 x+range 까지 검사한 것 중 thresold보다 큰 값,
	// 만약 최대 x가 결과라면 그 앞쪽까지 더 확인?
	vector<int> vecVProjection = imageHandler::getVerticalProjectionData(binImage);

	// 뒤에서부터 검사
	for (int i = targetStartX + range; i > targetStartX; i--)
	{
		if (vecVProjection.size() <= i)
			break;

		if (vecVProjection[i] >= threshold)
		{
			if (targetStartX + range == i)	//
				i = getRightistWhitePixel_x(binImage, targetStartX + range, range, threshold);
			return i;
		}
	}

	return targetStartX;	// 못찾으면 값 유지 
}


