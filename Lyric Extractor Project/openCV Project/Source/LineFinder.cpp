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
	inRange(lineInfo.maskImage_withWeight, 1, 255, mat_bin);	// 확인용


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

	lineInfo.frame_start = calculateStartTime(weightPrintImage, unPrintColor);				// 시작시간 계산
	Mat mask = getLineMask(weightPrintImage.binImage, lineInfo.frame_start, unPrintColor);
	lineInfo.maskImage_withWeight = mask.clone();

	Mat mat_bin;	// mask
	inRange(lineInfo.maskImage_withWeight, 1, 255, mat_bin);	// 확인용


	lineInfo = checkValidMask(lineInfo);	// ys-process : 마스크로 에러 검출
	if (lineInfo.isValid == false)//(isValid)
	{
		return lineInfo;
	}


	// endFrame 구하는 루틴  int getEndFrameNum(int startFrame, Mat mask, Scalar unPrintColor)
	lineInfo.frame_end = getEndFrameNum(lineInfo.frame_start, lineInfo.maskImage_withWeight, unPrintColor);	// ys-process : end frame으로 에러 검출
	if (lineInfo.frame_end == 0)	// 찾은 끝점이 maxWeight값보다 아래에 있을 때 
	{
		lineInfo.errorOccured(LINEERROR_ENDFRAME_ENDOVERFLOW);
		return lineInfo;
	}
	if (lineInfo.frame_end - lineInfo.frame_start <= 10)	// 라인의 길이가 10미만일 때
	{
		lineInfo.errorOccured(LINEERROR_ENDFRAME_RANGE);
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

int LineFinder::calculateStartTime(WeightMat weightMat, Scalar unPrintColor)
{
	int maxValue = imageHandler::getMaximumValue(weightMat.binImage);
	int curFrame = weightMat.frameNum - maxValue;	
	int calcStartFrame = 0;

	// weightMat.frameNum부터 ~ maxValue까지 탐색 -> tempMat(unprintColor로 얻은이미지)
	//  weightMat.binImage와 tempMat의 bitwiseAnd의 결과
	//  가장 dot의 수가 많았던 프레임을 startFrame 으로 함.
	// curFrame부터 정방향 진행, 점의 수가 훅 올라가는부분이 있다면 그부분을 curFrmae으로 조정
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
		if (dotCountMax == 0)	// 초기화
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
	int minWeight = imageHandler::getMinimumValue(lineInfo.maskImage_withWeight);	// minValue를 최소값을 제외한 값을 가저오자 or 얼마나 다양하게 있느지 확인 후 ...

	vector<int>	items = imageHandler::getValueArrWithSort(lineInfo.maskImage_withWeight); // .size() == 총 점의 개수, [i].value == 해당점의 weight
	items.erase(
		unique(items.begin(), items.end(),
			[](const int& a, const int& b) {
		if (a == b)
			return true;
		else
			return false;
	}	  // 중복제거	 
	), items.end());

	if (items.size() < 5)	// 조건 : 기록된 프레임 개수가 5개 이상 (연속될 수 있는 라인) [프레임 종류 딸림]
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_FRAMEKINDS);
		return lineInfo;
	}

	if (maxWeight - minWeight < 5) 	// 조건 : weight최대값 - weight최소값 = 5 이상
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_WEIGHT);
		return lineInfo;
	}

	Mat testMat;
	inRange(lineInfo.maskImage_withWeight, 1, 255, testMat);
	

	//// 조건 : 흰점시작점과 흰점끝점 중간 기준으로 왼쪽점들이 오른쪽점들보다 높은 weight를 가짐
	int leftistCoorX = imageHandler::getLeftistWhitePixel_x(lineInfo.maskImage_withWeight);
	int rightistCoorX = imageHandler::getRightistWhitePixel_x(lineInfo.maskImage_withWeight);
	//int midCoorX = (leftistCoorX + rightistCoorX) / 2;
	//// 가운데 기준 왼쪽, 오른쪽 마스킹한 이미지와 bitwise_and , 
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
	inRange(maskImage_rmNoise, 4, 255, maskImage_rmNoise);	// 3프레임까지 버림
	bitwise_and(maskImage_rmNoise, lineInfo.maskImage_withWeight, maskImage_rmNoise);

	leftistCoorX = imageHandler::getLeftistWhitePixel_x(maskImage_rmNoise);
	rightistCoorX = imageHandler::getRightistWhitePixel_x(maskImage_rmNoise);

	int totalRange = rightistCoorX- leftistCoorX;
	int sepaRange = totalRange / 3;

	for (int i = 0; i < 3; i++)	// 3등분해서 확인
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
		if (weightAvg[i] < minVal)	// 1이라도 감소해야함
		{
			minVal = weightAvg[i];
		}
		else
		{
			lineInfo.errorOccured(LINEERROR_MASKCHECK_WEIGHT_CONTINEUTY);	// 웨이트가 연속되지 않음
			return lineInfo;
		}
	}
	//end

	leftistCoorX = imageHandler::getLeftistWhitePixel_x(lineInfo.maskImage_withWeight);
	rightistCoorX = imageHandler::getRightistWhitePixel_x(lineInfo.maskImage_withWeight);
	// 조건 : 가장 오른쪽 점과 가장 왼쪽 점(라인의 길이)까지 거리가 100pix이상
	if (rightistCoorX - leftistCoorX < 100)
	{
		lineInfo.errorOccured(LINEERROR_MASKCHECK_Y_LENGHTH);	// 
		return lineInfo;
	}

	// 조건 : 컨투어 상위 절반의 크기가 100 이상
	//int contourVolumeInvalidLimit = 100;	// 100 이하인 것들은 Invalid로 판별
	//int avg = imageHandler::getAvgContourVolume(lineInfo.maskImage_withWeight);
	//if (avg < contourVolumeInvalidLimit)
	//{
	//	lineInfo.errorOccured(LINEERROR_MASKCHECK_CONTOUR_VOLUM_AVG);	// 
	//	return lineInfo;
	//}

	/* 필터	*/
	// 1. pixel 총 갯수
	// 2. weightMat_and가 흐르는지 (좌측 우측 비교 etc.,..)
	// 3. 등등..

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
		if (pixelCount == 0)	// 남은픽셀이 0이 되는 시점 : 끝점.
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