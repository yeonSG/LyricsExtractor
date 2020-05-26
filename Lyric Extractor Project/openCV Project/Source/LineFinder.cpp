#include "LineFinder.h"
#include "loger.h"

LineFinder::LineFinder(VideoCapture* vc)
{
	videoCapture = vc;
}

LineInfo LineFinder::getLine(WeightMat weightPrintImage)
{
	LineInfo lineInfo;
	lineInfo.isValid = true;

	lineInfo.maskImage_withWeight = weightPrintImage.binImage.clone();

	Mat mat_bin;	// mask
	inRange(lineInfo.maskImage_withWeight, 1, 255, mat_bin);	// Ȯ�ο�


	bool isValidMask = checkValidMask(lineInfo.maskImage_withWeight);
	if (isValidMask == false)//(isValid)
	{
		lineInfo.isValid = false;
		return lineInfo;
	}

	return lineInfo;
}

LineInfo LineFinder::getLine(WeightMat weightPrintImage, Scalar unPrintColor)
{
	LineInfo lineInfo;
	lineInfo.isValid = true;

	lineInfo.frame_start = calculateStartTime(weightPrintImage);
	Mat mask = getLineMask(weightPrintImage.binImage, lineInfo.frame_start, unPrintColor);
	lineInfo.maskImage_withWeight = mask.clone();

	Mat mat_bin;	// mask
	inRange(lineInfo.maskImage_withWeight, 1, 255, mat_bin);	// Ȯ�ο�


	bool isValidMask = checkValidMask(lineInfo.maskImage_withWeight);
	if (isValidMask == false)//(isValid)
	{
		lineInfo.isValid = false;
		return lineInfo;
	}


	// endFrame ���ϴ� ��ƾ  int getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor)
	lineInfo.frame_end = getEndFrameNum(lineInfo.frame_start, lineInfo.maskImage_withWeight, unPrintColor);
	if (lineInfo.frame_end == 0)	// ã�� ������ maxWeight������ �Ʒ��� ���� �� 
	{
		lineInfo.isValid = false;
		return lineInfo;
	}
	if (lineInfo.frame_end - lineInfo.frame_start <= 10)	// ������ ���̰� 10�̸��� ��
	{
		lineInfo.isValid = false;
		return lineInfo;
	}
/*	
	1. weight �̹���(a)�� ���������� ��� ���� �̹���(b)�� ��
	2. bitwise_and(a, b) ����
	3. ����� ����ũ�� ���
	4. ������ ����� ���Ͽ� Ÿ���÷� ����� �������� ������ ŉ�� (endFrame)
	2. �˻�
*/

	return lineInfo;
}

int LineFinder::calculateStartTime(WeightMat weightMat)
{
	int maxValue = imageHandler::getMaximumValue(weightMat.binImage);
	int curFrame = weightMat.frameNum - maxValue - 1;	

	return curFrame;	// �����ӹ�ȣ - (maxWeightValue - 1)
}

Mat LineFinder::getLineMask(Mat weightPrintImage, int startFrame, Scalar unPrintColor)
{
	Mat orgImage;
	Mat subImage;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)startFrame);
	videoCapture->read(orgImage);
	subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

	Mat unPrintPatternFill = imageHandler::getFillImage_unPrint(subImage, unPrintColor);
	inRange(unPrintPatternFill, Scalar(254, 254, 254), Scalar(255, 255, 255), unPrintPatternFill);

	Mat weightMat_and;	// mask
	bitwise_and(weightPrintImage, unPrintPatternFill, weightMat_and);
	
	return weightMat_and;
}

bool LineFinder::checkValidMask(Mat maskImage)
{
	//bool isVaild = true;

	int pixelCount = imageHandler::getWhitePixelCount(maskImage);
	if (pixelCount < 200)
	{
		return false;
	}

	int maxWeight = imageHandler::getMaximumValue(maskImage);
	int minWeight = imageHandler::getMinimumValue(maskImage);	// minValue�� �ּҰ��� ������ ���� �������� or �󸶳� �پ��ϰ� �ִ��� Ȯ�� �� ...

	vector<int>	items = imageHandler::getValueArrWithSort(maskImage); // .size() == �� ���� ����, [i].value == �ش����� weight
	items.erase(
		unique(items.begin(), items.end(),
			[](const int& a, const int& b) {
		if (a == b)
			return true;
		else
			return false;
	}	  // �ߺ�����	 
	), items.end());

	if (items.size() < 5)	// ���� : ��ϵ� ������ ������ 5�� �̻� (���ӵ� �� �ִ� ����)
	{
		return false;
	}

	if (maxWeight - minWeight < 5) 	// ���� : weight�ִ밪 - weight�ּҰ� = 5 �̻�
	{
		return false;
	}

	Mat testMat;
	inRange(maskImage, 1, 255, testMat);
	

	//// ���� : ������������ �������� �߰� �������� ���������� ���������麸�� ���� weight�� ����
	int leftistCoorX = imageHandler::getLeftistWhitePixel_x(maskImage);
	int rightistCoorX = imageHandler::getRightistWhitePixel_x(maskImage);
	//int midCoorX = (leftistCoorX + rightistCoorX) / 2;
	//// ��� ���� ����, ������ ����ŷ�� �̹����� bitwise_and , 
	//Mat areaMask = Mat::zeros(maskImage.rows, maskImage.cols, CV_8U);	
	//areaMask = imageHandler::getWhiteMaskImage(areaMask, leftistCoorX,0, midCoorX-leftistCoorX, maskImage.rows);
	//bitwise_and(areaMask, maskImage, areaMask);
	//int leftWeightAvg = imageHandler::getWhitePixelAvgValue(areaMask);	

	//areaMask = Mat::zeros(maskImage.rows, maskImage.cols, CV_8U);
	//areaMask = imageHandler::getWhiteMaskImage(areaMask, midCoorX, 0, rightistCoorX-midCoorX, maskImage.rows);
	//bitwise_and(areaMask, maskImage, areaMask);
	//int rightWeightAvg = imageHandler::getWhitePixelAvgValue(areaMask);

	//if (leftWeightAvg == 0 || rightWeightAvg == 0)
	//{
	//	return false;
	//}
	//if (leftWeightAvg <= rightWeightAvg)
	//{
	//	return false;
	//}


	//start	 -> 
	int totalRange = rightistCoorX- leftistCoorX;
	int sepaRange = totalRange / 3;
	vector<int> weightAvg;
	Mat maskImage_rmNoise = maskImage.clone();
	inRange(maskImage_rmNoise, 4, 255, maskImage_rmNoise);	// 3�����ӱ��� ����
	bitwise_and(maskImage_rmNoise, maskImage, maskImage_rmNoise);

	for (int i = 0; i < 3; i++)	// 3����ؼ� Ȯ��
	{
		Mat areaMask = Mat::zeros(maskImage.rows, maskImage.cols, CV_8U);
		int coorX = leftistCoorX + sepaRange * i;
		areaMask = imageHandler::getWhiteMaskImage(areaMask, coorX, 0, sepaRange, maskImage.rows);
		bitwise_and(areaMask, maskImage_rmNoise, areaMask);
		weightAvg.push_back(imageHandler::getWhitePixelAvgValue(areaMask));
		if (weightAvg.back() == 0)
		{
			return false;
		}
	}
	// 
	int minVal = 255;
	for (int i = 0; i < weightAvg.size(); i++)
	{
		if (weightAvg[i] <= minVal)
		{
			minVal = weightAvg[i];
		}
		else
		{
			return false;
		}
	}
	

	//end
	
	// ���� : ���� ������ ���� ���� ���� ��(������ ����)���� �Ÿ��� 100pix�̻�
	if (rightistCoorX - leftistCoorX < 100)
	{
		return false;
	}


	/* ����	*/
	// 1. pixel �� ����
	// 2. weightMat_and�� �帣���� (���� ���� �� etc.,..)
	// 3. ���..

	return true;
}

int LineFinder::getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor)
{
	Mat orgImage;
	Mat subImage;
	Mat mask_bin;
	inRange(mask, 1, 255, mask_bin);

	int maxValue = imageHandler::getMaximumValue(mask);
	int curFrame = startFrame;

	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
	while (videoCapture->read(orgImage))
	{
		Mat tempMat;
		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		Mat unPrintPatternFill = imageHandler::getFillImage_unPrint(subImage, unPrintColor);
		inRange(unPrintPatternFill, Scalar(254, 254, 254), Scalar(255, 255, 255), unPrintPatternFill);
		bitwise_and(mask_bin, unPrintPatternFill, tempMat);

		int pixelCount = imageHandler::getWhitePixelCount(tempMat);
		if (pixelCount == 0)	// ���� ã��
		{
			return curFrame;
		}

		if (startFrame + maxValue + 5 < curFrame)	// �˻������ӵ���(startFrame ~ +pixelMaxWeight) ���� ���� �ʴ´ٸ�.. 
		{
			return 0;	// false
		}

		curFrame++;
	}

	return 0;
}

bool LineInfo::desc(LineInfo a, LineInfo b)
{
	return a.frame_start > b.frame_start;
}

bool LineInfo::asc(LineInfo a, LineInfo b)
{
	return a.frame_start < b.frame_start;
}
