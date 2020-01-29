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
	Rect subRect(0, SUBTITLEAREA_Y, sourceImage.cols, SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
	Mat subImage = sourceImage(subRect);
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
Mat imageHandler::getMorphImage(Mat& sourceImage, MorphTypes type)
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
Mat imageHandler::getFloodProcessedImage(Mat& binaryMat, bool toBlack)
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

	Mat image_binIR_HSV_R_Filterd = getFloodProcessedImage(image_binIR_HSV_R);
	Mat image_binIR_RGB_R_Filterd = getFloodProcessedImage(image_binIR_RGB_R);

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
	inRange(subImage_hsv, Scalar(118, 240, 100), Scalar(140, 255, 255), image_binIR_HSV_B);	// binarize by hsv

	Mat image_binIR_HSV_B_Filterd = getFloodProcessedImage(image_binIR_HSV_B);
	Mat image_binIR_RGB_B_Filterd = getFloodProcessedImage(image_binIR_RGB_B);
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
	Mat changedToWhiteImage;
	Mat xorImage;
	bitwise_xor(binImageA, binImageB, xorImage);
	//Mat notImage;
	//bitwise_not(binImageB, notImage);
	//bitwise_and(xorImage, binImageB, changedToWhiteImage);
	return xorImage;
	//return changedToWhiteImage;
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
	cvtColor(fullyContrastImage, outImage, COLOR_BGR2GRAY);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			bool isRight = false;	// 조건만족?
			if (isWhite(yPtr_FCImage[x]))////if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	
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
				while (x + count < width)	// 위쪽 색 확인
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
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //

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
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(y - count)[x];
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
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y - (count + 1))[x];//yPtr_FCImage[x - (count + 1)]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
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
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y - (count + 1))[x]; //yPtr_FCImage[x - (count + 1)]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
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
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(y + count)[x]; //yPtr_FCImage[x + count];//FCImage.at<cv::Vec3b>(y, x + count);
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
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
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
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
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
	}

	return outImage;
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


