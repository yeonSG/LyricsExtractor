#include "UnprintImage.h"

// unprintImage는 최근 x프래임 안에 Unprint 색이었던 픽셀에 대한 정보를 갖고있음.
Mat UnprintImage::unprintImage_process(Mat frameImage, Scalar targetColor)
{	
	Mat patternFill = imageHandler::getFillImage(frameImage, targetColor);
	inRange(patternFill, Scalar(254, 254, 254), Scalar(255, 255, 255), patternFill);
	//Mat PatternFullfill = imageHandler::getBorderFloodFilledImage(patternFill);
	//Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(PatternFullfill);
	//patternFill = erodeImage_Denoise;

	vector<contourLineInfo> foundExpectedLines;

	if (m_UnprintImage.empty())
	{	// get dummy
		m_UnprintImage = Mat::ones(patternFill.rows, patternFill.cols, CV_8U)*255;
	}
	else
	{
			tick();
			updateUnprintImage(patternFill);
		// 모든 좌표점 ++ 수행
		// Unprint 색이 있다면 0으로 초기화
	}

	
	//Mat outImage;
	//inRange(m_UnprintImage, 0, 2, outImage);	// 최근 3프래임중 흰색이었던곳

	return m_UnprintImage;
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
