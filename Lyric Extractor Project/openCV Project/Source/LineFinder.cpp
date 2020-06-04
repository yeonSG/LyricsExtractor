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


	lineInfo = checkValidMask(lineInfo);	
	if (lineInfo.isValid == false)//(isValid)
	{
		return lineInfo;
	}

	return lineInfo;
}

LineInfo LineFinder::getLine(WeightMat weightPrintImage, Scalar unPrintColor)
{
	LineInfo lineInfo;
	lineInfo.isValid = true;

	lineInfo.frame_start = calculateStartTime(weightPrintImage, unPrintColor);				// ���۽ð� ���
	Mat mask = getLineMask(weightPrintImage.binImage, lineInfo.frame_start, unPrintColor);
	lineInfo.maskImage_withWeight = mask.clone();

	Mat mat_bin;	// mask
	inRange(lineInfo.maskImage_withWeight, 1, 255, mat_bin);	// Ȯ�ο�


	lineInfo = checkValidMask(lineInfo);	// ys-process : ����ũ�� ���� ����
	if (lineInfo.isValid == false)//(isValid)
	{
		return lineInfo;
	}


	// endFrame ���ϴ� ��ƾ  int getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor)
	lineInfo.frame_end = getEndFrameNum(lineInfo.frame_start, lineInfo.maskImage_withWeight, unPrintColor);	// ys-process : end frame���� ���� ����
	if (lineInfo.frame_end == 0)	// ã�� ������ maxWeight������ �Ʒ��� ���� �� 
	{
		lineInfo.errorOccured(LINEERROR_ENDFRAME_ENDOVERFLOW);
		return lineInfo;
	}
	if (lineInfo.frame_end - lineInfo.frame_start <= 10)	// ������ ���̰� 10�̸��� ��
	{
		lineInfo.errorOccured(LINEERROR_ENDFRAME_RANGE);
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

int LineFinder::calculateStartTime(WeightMat weightMat, Scalar unPrintColor)
{
	int maxValue = imageHandler::getMaximumValue(weightMat.binImage);
	int curFrame = weightMat.frameNum - maxValue;	
	int calcStartFrame = 0;

	// weightMat.frameNum���� ~ maxValue���� Ž�� -> tempMat(unprintColor�� �����̹���)
	//  weightMat.binImage�� tempMat�� bitwiseAnd�� ���
	//  ���� dot�� ���� ���Ҵ� �������� startFrame ���� ��.
	// curFrame���� ������ ����, ���� ���� �� �ö󰡴ºκ��� �ִٸ� �׺κ��� curFrmae���� ����
	int dotCountMax = 0;
	for (int i = curFrame; i < weightMat.frameNum; i++)
	{
		Mat orgImage;
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
		videoCapture->read(orgImage);
		Mat subImage;
		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		Mat tempMat = imageHandler::getFillImage_unPrint(subImage, unPrintColor);
		//inRange(tempMat, Scalar(254, 254, 254), Scalar(255, 255, 255), tempMat);	// to 1 demend

		int dotCount = imageHandler::getWhitePixelCount(tempMat);
		if (dotCountMax == 0)	// �ʱ�ȭ
		{
			dotCount = dotCountMax;
			calcStartFrame = curFrame;
		}
		else
		{
			if (dotCountMax < dotCount + 100)	// 
				calcStartFrame  = i;
		}
	}


	return calcStartFrame;
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

LineInfo LineFinder::checkValidMask(LineInfo lineInfo)
{
	//bool isVaild = true;

	int pixelCount = imageHandler::getWhitePixelCount(lineInfo.maskImage_withWeight);
	if (pixelCount < 200)
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_PIXELCOUNT);
		return lineInfo;
	}

	int maxWeight = imageHandler::getMaximumValue(lineInfo.maskImage_withWeight);
	int minWeight = imageHandler::getMinimumValue(lineInfo.maskImage_withWeight);	// minValue�� �ּҰ��� ������ ���� �������� or �󸶳� �پ��ϰ� �ִ��� Ȯ�� �� ...

	vector<int>	items = imageHandler::getValueArrWithSort(lineInfo.maskImage_withWeight); // .size() == �� ���� ����, [i].value == �ش����� weight
	items.erase(
		unique(items.begin(), items.end(),
			[](const int& a, const int& b) {
		if (a == b)
			return true;
		else
			return false;
	}	  // �ߺ�����	 
	), items.end());

	if (items.size() < 5)	// ���� : ��ϵ� ������ ������ 5�� �̻� (���ӵ� �� �ִ� ����) [������ ���� ����]
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_FRAMEKINDS);
		return lineInfo;
	}

	if (maxWeight - minWeight < 5) 	// ���� : weight�ִ밪 - weight�ּҰ� = 5 �̻�
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_WEIGHT);
		return lineInfo;
	}

	Mat testMat;
	inRange(lineInfo.maskImage_withWeight, 1, 255, testMat);
	

	//// ���� : ������������ �������� �߰� �������� ���������� ���������麸�� ���� weight�� ����
	int leftistCoorX = imageHandler::getLeftistWhitePixel_x(lineInfo.maskImage_withWeight);
	int rightistCoorX = imageHandler::getRightistWhitePixel_x(lineInfo.maskImage_withWeight);
	//int midCoorX = (leftistCoorX + rightistCoorX) / 2;
	//// ��� ���� ����, ������ ����ŷ�� �̹����� bitwise_and , 
	//Mat areaMask = Mat::zeros(lineInfo.maskImage_withWeight.rows, lineInfo.maskImage_withWeight.cols, CV_8U);	
	//areaMask = imageHandler::getWhiteMaskImage(areaMask, leftistCoorX,0, midCoorX-leftistCoorX, lineInfo.maskImage_withWeight.rows);
	//bitwise_and(areaMask, lineInfo.maskImage_withWeight, areaMask);
	//int leftWeightAvg = imageHandler::getWhitePixelAvgValue(areaMask);	

	//areaMask = Mat::zeros(lineInfo.maskImage_withWeight.rows, lineInfo.maskImage_withWeight.cols, CV_8U);
	//areaMask = imageHandler::getWhiteMaskImage(areaMask, midCoorX, 0, rightistCoorX-midCoorX, lineInfo.maskImage_withWeight.rows);
	//bitwise_and(areaMask, lineInfo.maskImage_withWeight, areaMask);
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
	vector<int> weightAvg;
	vector<int> pixelsum;
	Mat maskImage_rmNoise = lineInfo.maskImage_withWeight.clone();
	inRange(maskImage_rmNoise, 4, 255, maskImage_rmNoise);	// 3�����ӱ��� ����
	bitwise_and(maskImage_rmNoise, lineInfo.maskImage_withWeight, maskImage_rmNoise);

	leftistCoorX = imageHandler::getLeftistWhitePixel_x(maskImage_rmNoise);
	rightistCoorX = imageHandler::getRightistWhitePixel_x(maskImage_rmNoise);

	int totalRange = rightistCoorX- leftistCoorX;
	int sepaRange = totalRange / 3;

	for (int i = 0; i < 3; i++)	// 3����ؼ� Ȯ��
	{
		Mat areaMask = Mat::zeros(lineInfo.maskImage_withWeight.rows, lineInfo.maskImage_withWeight.cols, CV_8U);
		int coorX = leftistCoorX + sepaRange * i;
		areaMask = imageHandler::getWhiteMaskImage(areaMask, coorX, 0, sepaRange, lineInfo.maskImage_withWeight.rows);
		bitwise_and(areaMask, maskImage_rmNoise, areaMask);
		weightAvg.push_back(imageHandler::getWhitePixelAvgValue(areaMask));
		pixelsum.push_back(imageHandler::getWhitePixelCount(areaMask));
		if (weightAvg.back() == 0)
		{
			lineInfo.errorOccured(LINEERROR_MASKCHECK_WEIGHT_AVG);
			return lineInfo;
		}
	}
	// 
	int minVal = 255;
	for (int i = 0; i < weightAvg.size(); i++)
	{
		if (weightAvg[i] < minVal)	// 1�̶� �����ؾ���
		{
			minVal = weightAvg[i];
		}
		else
		{
			lineInfo.errorOccured(LINEERROR_MASKCHECK_WEIGHT_CONTINEUTY);	// ����Ʈ�� ���ӵ��� ����
			return lineInfo;
		}
	}
	//end

	leftistCoorX = imageHandler::getLeftistWhitePixel_x(lineInfo.maskImage_withWeight);
	rightistCoorX = imageHandler::getRightistWhitePixel_x(lineInfo.maskImage_withWeight);
	// ���� : ���� ������ ���� ���� ���� ��(������ ����)���� �Ÿ��� 100pix�̻�
	if (rightistCoorX - leftistCoorX < 100)
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_Y_LENGHTH);	// 
		return lineInfo;
	}

	// ���� : ������ ���� ������ ũ�Ⱑ 100 �̻�
	//int contourVolumeInvalidLimit = 100;	// 100 ������ �͵��� Invalid�� �Ǻ�
	//int avg = imageHandler::getAvgContourVolume(lineInfo.maskImage_withWeight);
	//if (avg < contourVolumeInvalidLimit)
	//{
	//	lineInfo.errorOccured(LINEERROR_MASKCHECK_CONTOUR_VOLUM_AVG);	// 
	//	return lineInfo;
	//}

	/* ����	*/
	// 1. pixel �� ����
	// 2. weightMat_and�� �帣���� (���� ���� �� etc.,..)
	// 3. ���..

	return lineInfo;
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
		if (pixelCount == 0)	// �����ȼ��� 0�� �Ǵ� ���� : ����.
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

void LineInfo::errorOccured(int errorType)
{
	this->errorNumber = errorType;
	this->isValid = false;
}