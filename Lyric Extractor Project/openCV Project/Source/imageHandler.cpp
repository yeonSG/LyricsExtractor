#include "imageHandler.h"


/// <summary>
/// �̹����� ���� �κи� �߶� ��ȯ��.
/// �ڸ� �κ��� ���� ���̱� ������ �����ؾ��� (���� �ڸ� �κ��� �������� ���Ѵٸ� �ڸ� �κ� ã�� �˰��� ���� �ʿ�)
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <returns>���� �κи� �ڸ� �̹���</returns>
Mat imageHandler::getSubtitleImage(Mat sourceImage)
{
	Rect subRect(0, SUBTITLEAREA_Y, sourceImage.cols, SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
	Mat subImage = sourceImage(subRect);
	return subImage;
}

/// <summary>
/// 
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <returns></returns>
Mat imageHandler::getBinaryImage(Mat sourceImage)
{
	// 0. cvtColor - ���ȭ
	Mat image_gray;
	cvtColor(sourceImage, image_gray, COLOR_BGR2GRAY);
	Mat image_gray_bin;
	threshold(image_gray, image_gray_bin, 146, 255, THRESH_BINARY);	// 146�� �׽�Ʈ�� ���� ������
	return image_gray_bin;
}

Mat imageHandler::getBlueColorFilteredBinaryImage(Mat sourceImage)
{
	// 3-1. Blue(HSV)�� ����ȭ
	Mat hsvImage;
	cvtColor(sourceImage, hsvImage, COLOR_BGR2HSV);
	// 3. ����ȭ
	Mat image_getBlue;	// Scalar (H=����(180'), S=ä��(255), V=��(255))
	inRange(hsvImage, Scalar(115, 100, 100), Scalar(140, 255, 255), image_getBlue);	// HSV���� �׽�Ʈ�� ���� ����.

	return image_getBlue;
}

Mat imageHandler::getMorphImage(Mat sourceImage)
{
	Mat element(5, 5, CV_8U, Scalar(1));
	element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));

	//Mat image_close;
	//morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);
	Mat image_dilateion;
	dilate(sourceImage, image_dilateion, element);	// ��â����
	//erode()	// ħ�Ŀ���

	return image_dilateion;
}


Mat imageHandler::getCannyImageWithBinaryImage(Mat binImage)
{
	Mat image_canny;
	Canny(binImage, image_canny, 250, 255);
	return image_canny;
}

/// <summary>
/// ���̳ʸ� �̹����� �µθ��� floodFill(�׸����� ����Ʈ��) ������ �� �̹��� ��ȯ(Noise ���ſ�)
/// </summary>
/// <param name="binaryMat">����ȭ �̹���</param>
/// <param name="toBlack">true�� �µθ��� �������� ��</param>
/// <returns>�׵θ��� floodFill ������ ��� �̹���</returns>
Mat imageHandler::getFloodProcessedImage(Mat binaryMat, bool toBlack)
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
Mat imageHandler::getCompositeBinaryImages(Mat subImage)
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
Mat imageHandler::getCompositeBinaryImagesRed(Mat subImage)
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

	Mat image_binIR_HSV_R_Filterd = getFloodProcessedImage(image_binIR_HSV_R.clone());
	Mat image_binIR_RGB_R_Filterd = getFloodProcessedImage(image_binIR_RGB_R.clone());

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
Mat imageHandler::getCompositeBinaryImagesBlue(Mat subImage)
{
	Mat subImage_hsv;	// Scalar (H=����(180'), S=ä��(255), V=��(255))
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

	Mat image_binIR_RGB_B;
	inRange(subImage, Scalar(140, 0, 0), Scalar(255, 40, 50), image_binIR_RGB_B);	// binarize by rgb

	Mat image_binIR_HSV_B;
	inRange(subImage_hsv, Scalar(118, 240, 100), Scalar(140, 255, 255), image_binIR_HSV_B);	// binarize by hsv

	Mat image_binIR_HSV_B_Filterd = getFloodProcessedImage(image_binIR_HSV_B.clone());
	Mat image_binIR_RGB_B_Filterd = getFloodProcessedImage(image_binIR_RGB_B.clone());
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
Mat imageHandler::getDifferenceImage(Mat binImageA, Mat binImageB)
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