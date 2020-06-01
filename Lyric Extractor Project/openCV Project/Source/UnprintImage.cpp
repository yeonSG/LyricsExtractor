#include "UnprintImage.h"

// unprintImage는 최근 x프래임 안에 Unprint 색이었던 픽셀에 대한 정보를 갖고있음.
Mat UnprintImage::unprintImage_process(Mat frameImage)
{	
	if (m_isFindUnprintColor != true)	// 아직 못찾음
	{
		Mat whiteImage = m_UnprintImage = Mat::ones(frameImage.rows, frameImage.cols, CV_8U) * 1;
		return whiteImage;
	}
	Mat targetColorImage = imageHandler::getFillImage_unPrint(frameImage, m_unPrintColor);
	inRange(targetColorImage, Scalar(254, 254, 254), Scalar(255, 255, 255), targetColorImage);	// to 1 demend


	vector<contourLineInfo> foundExpectedLines;

	if (m_UnprintImage.empty())
	{	// get dummy
		m_UnprintImage = Mat::ones(targetColorImage.rows, targetColorImage.cols, CV_8U)*253;
	}
	else
	{
		tick();
		updateUnprintImage(targetColorImage);
		// 모든 좌표점 ++ 수행
		// Unprint 색이 있다면 0으로 초기화
	}
	
	//Mat outImage;
	//inRange(m_UnprintImage, 0, 2, outImage);	// 최근 3프래임중 흰색이었던곳

	return m_UnprintImage;
}

Scalar UnprintImage::getUnprintColor(Mat fImage)
{
	Vec3b color_black = { 0,0,0 };
	Vec3b color_white = { 255, 255, 255 };
	Vec3b color_yellow = { 0, 255, 255 };
	int colorCount_white  = 0;	// 흰색 Unprint
	int colorCount_yellow = 0;	// 주황색 Unprint
	int colorCount_other = 0;

	vector< pair<Scalar, int>> pixels;

	int height = fImage.rows;
	int width = fImage.cols;

	for (int y = 0; y < height; y++)
	{
		Vec3b* yPtr = fImage.ptr<Vec3b>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr[x] != color_black)
			{
				if (yPtr[x] == color_white)
				{
					colorCount_white++;
				}
				else if (yPtr[x] == color_yellow)
				{
					colorCount_yellow++;
				}
				else
				{
					colorCount_other++;
				}
			}
		}
	}
	Scalar unPrintColor = color_white;
	int maxValue = colorCount_white;
	if (maxValue < colorCount_yellow)
	{
		unPrintColor = color_yellow;
		maxValue = colorCount_yellow;
	}
	if (maxValue < colorCount_other)
	{
		unPrintColor = color_white;
			maxValue = colorCount_other;
	}

	return unPrintColor;
}

void UnprintImage::tick()
{
	int height = m_UnprintImage.rows;
	int width = m_UnprintImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = m_UnprintImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr[x] != 255)	// 255아닌곳 ++;
			{
				yPtr[x]++;
			}
		}
	}
}

void UnprintImage::updateUnprintImage(Mat binImage)
{
	//int height = m_UnprintImage.rows;
	//int width = m_UnprintImage.cols;

	//for (int y = 0; y < height; y++)
	//{
	//	uchar* yPtr = m_UnprintImage.ptr<uchar>(y);
	//	uchar* yPtr_pattern = binImage.ptr<uchar>(y);

	//	for (int x = 0; x < width; x++)
	//	{
	//		if (yPtr_pattern[x] != 0)	// 
	//		{
	//			yPtr[x] = 0;	// 패턴에서 0이 아닌곳 0으로 초기화
	//		}
	//	}
	//}

	bitwise_not(binImage, binImage);
	bitwise_and(binImage, m_UnprintImage, m_UnprintImage);
}
