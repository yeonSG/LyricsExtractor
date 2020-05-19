#include "LineFinder.h"
#include "loger.h"

LineFinder::LineFinder(VideoCapture* vc)
{
	videoCapture = vc;
}

LineInfo LineFinder::getLine(WeightMat weightPrintImage, Scalar unPrintColor)
{
	LineInfo lineInfo;
	lineInfo.isValid = true;

	lineInfo.frame_start = calculateStartTime(weightPrintImage);
	Mat mask = getLineMask(weightPrintImage.binImage, lineInfo.frame_start, unPrintColor);
	lineInfo.maskImage_withWeight = mask.clone();

	Mat mat_bin;	// mask
	inRange(lineInfo.maskImage_withWeight, 1, 255, mat_bin);	// 확인용


	bool isValidMask = checkValidMask(lineInfo.maskImage_withWeight);
	if (isValidMask == false)//(isValid)
	{
		lineInfo.isValid = false;
		return lineInfo;
	}


	// endFrame 구하는 루틴  int getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor)
	lineInfo.frame_end = getEndFrameNum(lineInfo.frame_start, lineInfo.maskImage_withWeight, unPrintColor);
	if (lineInfo.frame_end == 0)	// 찾은 끝점이 maxWeight값보다 아래에 있을 때 
	{
		lineInfo.isValid = false;
		return lineInfo;
	}
	if (lineInfo.frame_end - lineInfo.frame_start <= 5)	// 라인의 길이가 5미만일 때
	{
		lineInfo.isValid = false;
		return lineInfo;
	}
/*	
	1. weight 이미지(a)의 시작점에서 흰색 필터 이미지(b)를 땀
	2. bitwise_and(a, b) 수행
	3. 결과를 마스크로 사용
	4. 정방향 재생을 통하여 타겟컬러 흰색이 없어지는 프래임 흭득 (endFrame)
	2. 검사
*/

	return lineInfo;
}

int LineFinder::calculateStartTime(WeightMat weightMat)
{
	int maxValue = imageHandler::getMaximumValue(weightMat.binImage);
	int curFrame = weightMat.frameNum - maxValue - 1;	

	return curFrame;	// 프래임번호 - (maxWeightValue - 1)
}

Mat LineFinder::getLineMask(Mat weightPrintImage, int startFrame, Scalar unPrintColor)
{
	Mat orgImage;
	Mat subImage;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)startFrame);
	videoCapture->read(orgImage);
	subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

	Mat unPrintPatternFill = imageHandler::getFillImage(subImage, unPrintColor);
	inRange(unPrintPatternFill, Scalar(254, 254, 254), Scalar(255, 255, 255), unPrintPatternFill);

	Mat weightMat_and;	// mask
	bitwise_and(weightPrintImage, unPrintPatternFill, weightMat_and);
	
	return weightMat_and;
}

bool LineFinder::checkValidMask(Mat maskImage)
{
	//bool isVaild = true;

	int pixelCount = imageHandler::getWhitePixelCount(maskImage);
	if (pixelCount < 300)
	{
		return false;
	}

	int maxWeight = imageHandler::getMaximumValue(maskImage);
	int minWeight = imageHandler::getMinimumValue(maskImage);	// minValue를 최소값을 제외한 값을 가저오자 or 얼마나 다양하게 있느지 확인 후 ...

	vector<int>	items = imageHandler::getValueArrWithSort(maskImage); // .size() == 총 점의 개수, [i].value == 해당점의 weight
	items.erase(
		unique(items.begin(), items.end(),
			[](const int& a, const int& b) {
		if (a == b)
			return true;
		else
			return false;
	}	  // 중복제거	 
	), items.end());

	if (items.size() < 5)	// 조건 : 기록된 프레임 개수가 5개 이상
	{
		return false;
	}

	if (maxWeight - minWeight < 5) 	// 조건 : weight최대값 - weight최소값 = 5 이상
	{
		return false;
	}

	Mat testMat;
	inRange(maskImage, 1, 255, testMat);

	// 조건 : 흰점시작점과 흰점끝점 중간 기준으로 왼쪽점들이 오른쪽점들보다 높은 weight를 가짐
	int leftistCoorX = imageHandler::getLeftistWhitePixel_x(maskImage);
	int rightistCoorX = imageHandler::getRightistWhitePixel_x(maskImage);
	int midCoorX = (leftistCoorX + rightistCoorX) / 2;
	// 가운데 기준 왼쪽, 오른쪽 마스킹한 이미지와 bitwise_and , 
	Mat areaMask = Mat::zeros(maskImage.rows, maskImage.cols, CV_8U);	
	areaMask = imageHandler::getWhiteMaskImage(areaMask, leftistCoorX,0, midCoorX-leftistCoorX, maskImage.rows);
	bitwise_and(areaMask, maskImage, areaMask);
	int leftWeightAvg = imageHandler::getWhitePixelAvgValue(areaMask);	

	areaMask = Mat::zeros(maskImage.rows, maskImage.cols, CV_8U);
	areaMask = imageHandler::getWhiteMaskImage(areaMask, midCoorX, 0, rightistCoorX-midCoorX, maskImage.rows);
	bitwise_and(areaMask, maskImage, areaMask);
	int rightWeightAvg = imageHandler::getWhitePixelAvgValue(areaMask);

	if (leftWeightAvg == 0 || rightWeightAvg == 0)
	{
		return false;
	}
	if (leftWeightAvg <= rightWeightAvg)
	{
		return false;
	}
	
	// 조건 : 가장 오른쪽 점과 가장 왼쪽 점(라인의 길이)까지 거리가 100pix이상
	if (rightistCoorX - leftistCoorX < 100)
	{
		return false;
	}

	/* 필터	*/
	// 1. pixel 총 갯수
	// 2. weightMat_and가 흐르는지 (좌측 우측 비교 etc.,..)
	// 3. 등등..

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

		Mat unPrintPatternFill = imageHandler::getFillImage(subImage, unPrintColor);
		inRange(unPrintPatternFill, Scalar(254, 254, 254), Scalar(255, 255, 255), unPrintPatternFill);
		bitwise_and(mask_bin, unPrintPatternFill, tempMat);

		int pixelCount = imageHandler::getWhitePixelCount(tempMat);
		if (pixelCount == 0)	// 끝점 찾음
		{
			return curFrame;
		}

		if (startFrame + maxValue + 5 < curFrame)	// 검사프래임동안(startFrame ~ +pixelMaxWeight) 끝이 나지 않는다면.. 
		{
			return 0;	// false
		}

		curFrame++;
	}

	return 0;
}
