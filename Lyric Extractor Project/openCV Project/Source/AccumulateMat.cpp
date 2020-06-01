#include "AccumulateMat.h"

Mat AccumulateMat::accumulateProcess(Mat patternImage)
{	

	if (m_AccImage.empty())
	{	// get dummy
		m_AccImage = Mat::ones(patternImage.rows, patternImage.cols, CV_8U)*255;
	}
	else
	{
		updateAccImage(patternImage);
	}

	return m_AccImage;
}

void AccumulateMat::tick(Mat binImage)
{
	int height = m_AccImage.rows;
	int width = m_AccImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_acc = m_AccImage.ptr<uchar>(y);
		uchar* yPtr_pattern = binImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr_acc[x] != 255)	// 255아닌곳 ++;
			{
				if(yPtr_pattern[x]!=0)
					yPtr_acc[x]++;
				else 
					yPtr_acc[x] = 0;
			}
		}
	}
}

void AccumulateMat::updateAccImage(Mat binImage)
{
	int height = m_AccImage.rows;
	int width = m_AccImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_acc = m_AccImage.ptr<uchar>(y);
		uchar* yPtr_pattern = binImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr_pattern[x] == 0)	// 패턴이 0임 -> stack 초기화
			{
				yPtr_acc[x] = 0;
			}
			else // 패텬이 0이 아님
			{
				if(yPtr_acc[x]!=255)
					yPtr_acc[x]++;		
			}
		}

		// 0이 되는 조건 : binImage이 0임
		// +되는 조건 : binImage도 양수임

		/*for (int x = 0; x < width; x++)
		{
			if (yPtr_acc[x] != 0)
			{
				if(yPtr_pattern[x]==0)
			}
		}*/

	}

}
