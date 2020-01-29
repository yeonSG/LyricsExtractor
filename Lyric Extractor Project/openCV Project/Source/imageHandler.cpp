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
/// �̹����� ���� �κи� �߶� ��ȯ��.
/// �ڸ� �κ��� ���� ���̱� ������ �����ؾ��� (���� �ڸ� �κ��� �������� ���Ѵٸ� �ڸ� �κ� ã�� �˰��� ���� �ʿ�)
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <returns>���� �κи� �ڸ� �̹���</returns>
Mat imageHandler::getSubtitleImage(Mat& sourceImage)
{
	Rect subRect(0, SUBTITLEAREA_Y, sourceImage.cols, SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
	Mat subImage = sourceImage(subRect);
	return subImage;
}

/// <summary>
/// thresold �Լ��� ����ȭ �� �̹��� ��ȯ.
/// </summary>
/// <param name="sourceImage">3ü�� �̹���(�÷�).</param>
/// <returns></returns>
Mat imageHandler::getBinaryImage(Mat& sourceImage)
{
	Mat image_gray;
	cvtColor(sourceImage, image_gray, COLOR_BGR2GRAY);
	Mat image_gray_bin;
	threshold(image_gray, image_gray_bin, 146, 255, THRESH_BINARY);	// 146�� �׽�Ʈ�� ���� ������
	return image_gray_bin;
}

Mat imageHandler::getBlueColorFilteredBinaryImage(Mat& sourceImage)
{
	// 3-1. Blue(HSV)�� ����ȭ
	Mat hsvImage;
	cvtColor(sourceImage, hsvImage, COLOR_BGR2HSV);
	// 3. ����ȭ
	Mat image_getBlue;	// Scalar (H=����(180'), S=ä��(255), V=��(255))
	inRange(hsvImage, Scalar(115, 100, 100), Scalar(140, 255, 255), image_getBlue);	// HSV���� �׽�Ʈ�� ���� ����.

	return image_getBlue;
}

/// <summary>
/// ���÷��� ���� ���� (Noise ����)
/// </summary>
/// <param name="sourceImage">����ȭ �� �̹���.</param>
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
		dilate(sourceImage, image_morp, element);	// ��â����
		break;
	case MORPH_ERODE:
		erode(sourceImage, image_morp, element);	// ��â����
		break;
	case MORPH_CLOSE:
		morphologyEx(sourceImage, image_morp, MORPH_CLOSE, element);
		break;
	case MORPH_OPEN:
		morphologyEx(sourceImage, image_morp, MORPH_OPEN, element);
		break;
	}
	//erode()	// ħ�Ŀ���

	return image_morp;
}


Mat imageHandler::getCannyImageWithBinaryImage(Mat& binImage)
{
	Mat image_canny;
	Canny(binImage, image_canny, 250, 255);
	return image_canny;
}

/// <summary>
/// ���̳ʸ� �̹����� �µθ��� floodFill(�׸����� ����Ʈ��) ������ �� �̹��� ��ȯ(Noise ����)
/// </summary>
/// <param name="binaryMat">����ȭ �̹���</param>
/// <param name="toBlack">true�� �µθ��� �������� ��</param>
/// <returns>�׵θ��� floodFill ������ ��� �̹���</returns>
Mat imageHandler::getFloodProcessedImage(Mat& binaryMat, bool toBlack)
{
	int nRows = binaryMat.rows;
	int nCols = binaryMat.cols;

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;

	// ���� 
	for (int i = 0; i < nCols; i++)
		if (binaryMat.at<uchar>(2, i) != color)
			floodFill(binaryMat, Point(i, 2), color);

	// ����
	for (int i = 0; i < nRows; i++)
		if (binaryMat.at<uchar>(i, 30) != color)
			floodFill(binaryMat, Point(30, i), color);

	// ����
	for (int i = 0; i < nRows; i++)
		if (binaryMat.at<uchar>(i, nCols - 30) != color)
			floodFill(binaryMat, Point(nCols - 30, i), color);

	// �Ʒ���
	for (int i = 0; i < nCols; i++)
		if (binaryMat.at<uchar>(nRows - 1, i) != color)
			floodFill(binaryMat, Point(i, nRows - 1), color);

	return binaryMat;
}


/// <summary>
/// hsv�� rgb�� ���͸� �� ���� �̹����� ��ȯ��
/// ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV)) 
/// </summary>
/// <param name="subImage">The subtitle image.</param>
/// <returns>����ȭ �� �̹���</returns>
Mat imageHandler::getCompositeBinaryImages(Mat& subImage)
{
	Mat image_merged;
	Mat image_binIR_Red = getCompositeBinaryImagesRed(subImage);
	Mat image_binIR_Blue = getCompositeBinaryImagesBlue(subImage);
	bitwise_or(image_binIR_Blue, image_binIR_Red, image_merged);
	return image_merged;
}

/// <summary>
/// �ڸ��� �������� hsv�� rgb�� ���͸� �� ���� �̹����� ��ȯ��
/// ((Red_RGB)AND(Red_HSV))
/// </summary>
/// <param name="subImage">The sub image.</param>
/// <returns>���������� ����ȭ �� �̹���</returns>
Mat imageHandler::getCompositeBinaryImagesRed(Mat& subImage)
{
	Mat subImage_hsv;	// Scalar (H=����(180'), S=ä��(255), V=��(255))
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

	Mat image_binIR_RGB_R;
	inRange(subImage, Scalar(0, 0, 130), Scalar(50, 50, 255), image_binIR_RGB_R);	// binarize by rgb

	Mat image_binIR_HSV_R_0;
	Mat image_binIR_HSV_R_1;
	Mat image_binIR_HSV_R;
	inRange(subImage_hsv, Scalar(0, 140, 180), Scalar(1, 255, 255), image_binIR_HSV_R_0); // binarize by hsv
	inRange(subImage_hsv, Scalar(160, 140, 180), Scalar(179, 255, 255), image_binIR_HSV_R_1); // binarize by hsv
	bitwise_or(image_binIR_HSV_R_0, image_binIR_HSV_R_1, image_binIR_HSV_R);	// �� ������ ���� hue = (0~1 + 160~179)

	Mat image_binIR_HSV_R_Filterd = getFloodProcessedImage(image_binIR_HSV_R);
	Mat image_binIR_RGB_R_Filterd = getFloodProcessedImage(image_binIR_RGB_R);

	Mat image_binIR_Red;
	bitwise_and(image_binIR_RGB_R_Filterd, image_binIR_HSV_R_Filterd, image_binIR_Red);

	return image_binIR_Red;
}

/// <summary>
/// �ڸ��� �Ķ����� hsv�� rgb�� ���͸� �� ���� �̹����� ��ȯ��
/// ((Blue_RGB)AND(Blue_HSV))
/// </summary>
/// <param name="subImage">The sub image.</param>
/// <returns>�Ķ����� ����ȭ �� �̹���</returns>
Mat imageHandler::getCompositeBinaryImagesBlue(Mat& subImage)
{
	Mat subImage_hsv;	// Scalar (H=����(180'), S=ä��(255), V=��(255))
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
/// �Էµ� �ΰ��� ���� �̹����� �������� ���� �̹��� ��ȯ
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
/// ��ĥ�� �κ�ã�� ������� ǥ���� �̹���
/// ���� : �ܼ�ȭ�� �̹���(FullyContrast)���� ���� ���� �¿���� �˻���
/// ���� : ������������ ������ ����or�Ķ��� 2�� �̻� ���ӵǰ�, �ݴ����� ������ 2�� �̻� ���ӵ� 
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

	// �࿬��
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			bool isRight = false;	// ���Ǹ���?
			if (isWhite(yPtr_FCImage[x]))////if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	
			{
				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;
				while (x - count > 0)	// �Ʒ��� �� Ȯ��
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
						if (x - (count + 1) > 0)	// ���ӵǴ��� Ȯ��
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
						if (x - (count + 1) > 0)	// ���ӵǴ��� Ȯ��
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
				while (x + count < width)	// ���� �� Ȯ��
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
						if (x + (count + 1) < width)	// ���ӵǴ��� Ȯ��
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
						if (x + (count + 1) < width)	// ���ӵǴ��� Ȯ��
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
					isRight = true;	// ���Ǹ���
			}

			if (isRight)	// ���ǿ� ������
				yPtr[x] = 255;
			else
				yPtr[x] = 0;
		}
	}

	// ������
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //

		for (int x = 0; x < width; x++)
		{
			if (isWhite(yPtr[x]))	// �̹� ����ΰ�
				continue;

			bool isRight = false;	// ���Ǹ���?
			if (isWhite(yPtr_FCImage[x]))
			{
				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (y - count > 0)	// �Ʒ��� �� Ȯ��
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
						if (y - (count + 1) > 0)	// ���ӵǴ��� Ȯ��
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
						if (y - (count + 1) > 0)	// ���ӵǴ��� Ȯ��
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
				while (y + count < height)	// ���� �� Ȯ��
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
						if (y + (count + 1) < height)	// ���ӵǴ��� Ȯ��
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
						if (y + (count + 1) < height)	// ���ӵǴ��� Ȯ��
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
					isRight = true;	// ���Ǹ���
			}

			if (isRight)	// ���ǿ� ������
				yPtr[x] = 255;
			//else
			//	yPtr[x] = 0;
		}
	}

	return outImage;
}

Mat imageHandler::getFullyContrastImage(Mat rgbImage)
{
	// RGB �и�
	Mat bgr[3];
	split(rgbImage, bgr);
	// ������ 128 �Ӱ�� �� �̹��� ����
	threshold(bgr[0], bgr[0], 127, 255, THRESH_BINARY);
	threshold(bgr[1], bgr[1], 127, 255, THRESH_BINARY);
	threshold(bgr[2], bgr[2], 127, 255, THRESH_BINARY);
	// RGB ��ħ
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


