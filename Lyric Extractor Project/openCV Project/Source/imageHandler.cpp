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
	Rect subRect(0, SUBTITLEAREA_Y, sourceImage.cols, sourceImage.rows - SUBTITLEAREA_Y);//SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
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
Mat imageHandler::getMorphImage(Mat& sourceImage, cv::MorphTypes type)
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

Mat imageHandler::getFloodProcessedImageWhiteToBlack(Mat& colorMat)
{
	int nRows = colorMat.rows;
	int nCols = colorMat.cols;

	Scalar color_white = Scalar(255, 255, 255);
	Scalar color_black = Scalar(0, 0, 0);


	// ���� 
	for (int i = 0; i < nCols; i++)
	{
		Scalar matColor = colorMat.at<Vec3b>(2, i);
		if (matColor == color_white)
			floodFill(colorMat, Point(i, 2), color_black);;
	}

	// ����
	for (int i = 0; i < nRows; i++)
	{
		Scalar matColor = colorMat.at<Vec3b>(i, 30);
		if (matColor == color_white)
			floodFill(colorMat, Point(30, i), color_black);
	}

	// ����
	for (int i = 0; i < nRows; i++)
	{
		Scalar matColor = colorMat.at<Vec3b>(i, nCols - 30);
		if (matColor == color_white)
			floodFill(colorMat, Point(nCols - 30, i), color_black);
	}

	// �Ʒ���
	for (int i = 0; i < nCols; i++)
	{
		Scalar matColor = colorMat.at<Vec3b>(nRows - 1, i);
		if (matColor == color_white)
			floodFill(colorMat, Point(i, nRows - 1), color_black);
	}

	return colorMat;
}

/// <summary>
/// ����� MaskImage ��� ���� ��, 
///  - ���η� ��� �����ִ� Object�� ����
///  - �ش��ϴ� ���� row�� �ִ� �ּ� x���� ���� �߾ӱ������� ������ ����ϴٸ� ������ �ƴѰɷ�
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
/// �⿵ ȸ�� ������ ��� Ŀư�̹��� ����
/// </summary>
/// <param name="cols">The cols.</param>
/// <param name="rows">The rows.</param>
/// <param name="maskLength">Ŀư�� ����</param>
/// <param name="targetColum">Ÿ�� ��ǥ.</param>
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
	inRange(subImage_hsv, Scalar(118, 230, 100), Scalar(140, 255, 255), image_binIR_HSV_B);	// binarize by hsv

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

/*
	�� ��(��) �� ������ ���� ã�Ƴ�
*/
Mat imageHandler::getPaintedBinImage_inner(Mat& rgbImage)
{
	Mat fullyContrastImage = getFullyContrastImage(rgbImage);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage_row;
	Mat outImage_col;
	cvtColor(fullyContrastImage, outImage_row, COLOR_BGR2GRAY);
	cvtColor(fullyContrastImage, outImage_col, COLOR_BGR2GRAY);

	// �࿬��
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage_col.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			bool isRight = false;	// ���Ǹ���?

			if(isBlue(yPtr_FCImage[x]) || isRed(yPtr_FCImage[x]))
			//if (isWhite(yPtr_FCImage[x]))
			{
				bool targetColorisBlue;
				if (isBlue(yPtr_FCImage[x]))
					targetColorisBlue = true;
				else
					targetColorisBlue = false;

				int count = 1;
				int color_m = -1; // white=1, blue=2, red=3, other= -1    /// black==2, Blue,Red==1, other==-1
				int color_p = -1;
				while (x - count > 0)	// �Ʒ��� �� Ȯ��
				{
					Vec3b v3p = yPtr_FCImage[x - count];

					if( (isBlue(v3p) && targetColorisBlue) || (isRed(v3p) && !targetColorisBlue) )
					{
						count++;//x_m--;
						if (count > 100)	// ��(��)�� ����� ����ϴ°�
							break;
					}
					else if (isWhite(v3p))
					{
						if (x - (count + 1) > 0)	// ���ӵǴ��� Ȯ��
						{
							Vec3b v3p_ = yPtr_FCImage[x - (count + 1)]; 
							if (isWhite(v3p_))
							{
								color_m = 1;	// white
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

					if ((isBlue(v3p) && targetColorisBlue) || (isRed(v3p) && !targetColorisBlue))
					{
						count++;//x_m--;
						if (count > 100)	// ��(��)�� ����� ����ϴ°�
							break;
					}
					else if (isWhite(v3p))
					{
						if (x + (count + 1) < width)	// ���ӵǴ��� Ȯ��
						{
							Vec3b v3p_ = yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isWhite(v3p_))
							{
								color_p = 1;	
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
				
				if((color_p == 1 && color_m == 1) || (color_p == 1 && color_m == 1))	// ������ ���, �ݴ��ʵ� ���
					isRight = true;
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
		uchar* yPtr = outImage_row.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //

		for (int x = 0; x < width; x++)
		{
			if (isWhite(yPtr[x]))	// �̹� ����ΰ�
				continue;

			bool isRight = false;	// ���Ǹ���?
			if (isBlue(yPtr_FCImage[x]) || isRed(yPtr_FCImage[x]))
			{
				bool targetColorisBlue;
				if (isBlue(yPtr_FCImage[x]))
					targetColorisBlue = true;
				else
					targetColorisBlue = false;

				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;
				while (y - count > 0)	// �Ʒ��� �� Ȯ��
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(y - count)[x];

					if ( (isBlue(v3p) && targetColorisBlue) || (isRed(v3p) && !targetColorisBlue) )
					{
						count++;//x_m--;
						if (count > 100)
							break;
					}
					else if (isWhite(v3p))
					{
						if (y - (count + 1) > 0)	// ���ӵǴ��� Ȯ��
						{
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y - (count + 1))[x];//yPtr_FCImage[x - (count + 1)]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isWhite(v3p_))
							{
								color_m = 1;		// white
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
					if ((isBlue(v3p) && targetColorisBlue) || (isRed(v3p) && !targetColorisBlue))
					{
						count++;//x_p++;
						if (count > 100)
							break;
					}
					else if (isWhite(v3p))
					{
						if (y + (count + 1) < height)	// ���ӵǴ��� Ȯ��
						{
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isWhite(v3p_))
							{
								color_p = 1;	
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

				if ((color_p == 1 && color_m == 1) || (color_p == 1 && color_m == 1))	// ������ ���, �ݴ��ʵ� ���
					isRight = true;
			}

			if (isRight)	// ���ǿ� ������
				yPtr[x] = 255;
			//else
			//	yPtr[x] = 0;
		}
	}
	Mat outimage;
	bitwise_and(outImage_row, outImage_col, outimage);
	threshold(outimage, outimage, 200, 255, THRESH_BINARY);

	return outimage;
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

Mat imageHandler::getSharpenAndContrastImage(Mat rgbImage)
{			// �����Ųٱ� ���� �ι� ��
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

// BinImage�� �ּ�, �ִ밪 ���� �� �ش� �� +-10 Vertical���� �ڸ�
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

// ���� �̹������� ���� �κ��� ����� ���� �Լ�
Mat imageHandler::removeSubLyricLine(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);

	vector<pair<int, int>> islands;
	vector<int> islandsVolum;

	// ���� ���� ����, 
	// ���� 2�� �̻��̸鼭, ���� �Ʒ����� ������ 2�� �̸�
	// ���� lyric���� �Ǻ��Ͽ� 2�� ����
	/*	or ���� ū volume�� Lyric�� ���� */
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

		if (islandsVolum_sort[1] == islandsVolum[islandsVolum.size() - 1])	// ���� �Ʒ����� ������ 2��
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

	// ���� ���� ����, 
	// ���� 2�� �̻��̸鼭, ���� �Ʒ����� ������ 2�� �̸�
	// ���� lyric���� �Ǻ��Ͽ� 2�� ����
	/*	or ���� ū volume�� Lyric�� ���� */
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
/// ���� �̹����� Vertical projection �����ͷ� ��ȯ 
/// </summary>
/// <param name="binImage">��ȯ�� ���� �̹���.</param>
/// <returns>Vertical projection ������</returns>
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
/// ���� �̹����� Horizontal projection �����ͷ� ��ȯ 
/// </summary>
/// <param name="binImage">��ȯ�� ���� �̹���.</param>
/// <returns>Horizontal projection ������</returns>
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
/// Mat�� ��� ���� ������ ��ȯ��
/// </summary>
/// <param name="binImage">The binary mat.</param>
/// <returns>��� ���� ����</returns>
int imageHandler::getWihtePixelCount(Mat binImage)
{
	int height = binImage.rows;
	int width = binImage.cols;
	int whiteCount = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// ����� �ƴѰ��
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
/// <param name="targetStartX">����� �ɽ�������.</param>
/// <param name="range">��� x ���� +range���� �˻�.</param>
/// <param name="threshold"> ����� �� col�� ���� �ּ� ����.</param>
/// <returns></returns>
int imageHandler::getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold)
{
	// get Vertical Projection
	// ��� x ���� x+range ���� �˻��� �� �� thresold���� ū ��,
	// ���� �ִ� x�� ������ �� ���ʱ��� �� Ȯ��?
	vector<int> vecVProjection = imageHandler::getVerticalProjectionData(binImage);

	// �ڿ������� �˻�
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

	return targetStartX;	// ��ã���� �� ���� 
}


