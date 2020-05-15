#include "PeakFinder.h"
#include "loger.h"

const int PeakFinder::JUDGE_TIMEOUT = 5;

vector<contourLineInfo> PeakFinder::frameImage_process(Mat frameImage, int frameNumber, Scalar targetColor)
{	
	Mat patternFill = imageHandler::getPatternFillImage_2(frameImage, targetColor);
	this->frameNumber = frameNumber;
	uint weightSumAtBinImage = 0;
	int colorPixelSumAtBinImage = 0;

	vector<contourLineInfo> foundExpectedLines;

	if (stackBinImage.empty())
	{	// get dummy
		//stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_16U);
		stackBinImage = Mat::zeros(patternFill.rows, patternFill.cols, CV_8U);
	}
	else
	{
		//vector<contourLineInfo> expectedLineInfos_curFrame;
		stackBinImage = imageHandler::stackBinImage(stackBinImage, patternFill);
		//stackBinImages = imageHandler::stackBinImage(stackBinImages, patternFill);	// for test
		//stackBinImage = imageHandler::getMaxColorContoursImage(stackBinImage, expectedLineInfos_curFrame);
		weightSumAtBinImage = imageHandler::getSumOfBinImageValues(stackBinImage);
		colorPixelSumAtBinImage = imageHandler::getWhitePixelCount(stackBinImage);

		makeContourMaxBinImageAndContourInfos();
		makeExpectedLineInfos();
		printf("[found CLine %d]", contourMaxBinImage_expectedLineInfo.size());

		foundExpectedLines = getJudgeLineByFrameFlow();	// 
	}

	return foundExpectedLines;
}

// 컨투어 맥스 이미지 생성과 컨투어 정보 생성
void PeakFinder::makeContourMaxBinImageAndContourInfos()
{
	Mat outImage = Mat::zeros(stackBinImage.rows, stackBinImage.cols, CV_8U);
	contourMaxBinImage_contourInfos.clear();

	vector<vector<Point>> contours;
	findContours(stackBinImage, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	Mat binImage;
	inRange(stackBinImage, 1, 255, binImage);
	binImage = imageHandler::getDustRemovedImage(binImage);	// 부피가 3보다 작은것 삭제함

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(stackBinImage.rows, stackBinImage.cols, CV_8U);;
		// 컨투어별 마스크
		vector<vector<Point>> contours_picked;
		contours_picked.push_back(contours[i]);
		fillPoly(contourMask, contours_picked, 255);
		vector<Point> indices;		// 내부의 점들을 얻음
		bitwise_and(contourMask, binImage, contourMask);
		findNonZero(contourMask, indices);
		
		//int sum = 0;
		int max = 0;
		for (int idx = 0; idx < indices.size(); idx++)
		{
			uchar* yPtr = stackBinImage.ptr<uchar>(indices[idx].y);	//in
			//sum += yPtr[indices[idx].x];
			if (max < yPtr[indices[idx].x])
				max = yPtr[indices[idx].x];
		}
		//int avgContourColor = sum / indices.size();

		for (int idx = 0; idx < indices.size(); idx++)
		{
			uchar* yPtr = outImage.ptr<uchar>(indices[idx].y);	//in
			//yPtr[indices[idx].x] = avgContourColor;
			yPtr[indices[idx].x] = max;
		}

		contourInfo conInfo = getContourInfoFromPixels(indices);
		conInfo.maxValue = max;
		contourMaxBinImage_contourInfos.push_back(conInfo);
	}
	sort(contourMaxBinImage_contourInfos.begin(), contourMaxBinImage_contourInfos.end(), imageHandler::asc_contourInfo);
	contourMaxBinImage = outImage.clone();
}

vector<contourLineInfo> PeakFinder::getLineInfoFromContourInfos(vector<contourInfo> contourInfos)
{
	vector<contourLineInfo> conLineInfos;	// Expect contour Line
	
		;	// contourLineInfo 생성 루틴
			// 2.1 전채순회하여 모든 연결 구함 (모든 컨투어 진행)
			// 2.2 
		for (int i = 0; i < contourInfos.size(); i++)
		{
			if (contourInfos[i].isRefed == false)
			{
				contourInfos[i].isRefed = true;
				contourLineInfo contourLineInfo;
				contourLineInfo.contours.push_back(contourInfos[i]);
				contourLineInfo.coorY_start = contourInfos[i].coorY_start;
				contourLineInfo.coorY_end = contourInfos[i].coorY_end;
				contourLineInfo.coorX_start = contourInfos[i].coorX_start;
				contourLineInfo.coorX_end = contourInfos[i].coorX_end;

				for (int j = i + 1; j < contourInfos.size(); j++)	// 남은 컨투어들 확인
				{
					// Y start~end 안에 포함된다면 추가
					if (imageHandler::isRelation(contourLineInfo.coorY_start, contourLineInfo.coorY_end, contourInfos[j].coorY_start, contourInfos[j].coorY_end))
					{
						contourLineInfo.contours.push_back(contourInfos[j]);
						contourInfos[j].isRefed = true;
					}
					contourLineInfo.pixelCount = imageHandler::getContourLineInfoVolume(contourLineInfo);
				}

				conLineInfos.push_back(contourLineInfo);
			}
		}	
		return conLineInfos;
}

Mat PeakFinder::getLineInfoAreaCuttedImage(contourLineInfo LineInfo)
{
	Mat maskImage = Mat::zeros(contourMaxBinImage.rows, contourMaxBinImage.cols, CV_8U);
	maskImage = imageHandler::getWhiteMaskImage(maskImage, LineInfo.coorX_start, LineInfo.coorY_start, LineInfo.coorX_end - LineInfo.coorX_start, LineInfo.coorY_end - LineInfo.coorY_start);

	Mat maskedImage;
	bitwise_and(maskImage, contourMaxBinImage, maskedImage);

	return maskedImage;
}

contourInfo PeakFinder::getContourInfoFromPixels(vector<Point> pixels)
{
	contourInfo conInfo;
	conInfo.maxValue = 0;
	for (int idx = 0; idx < pixels.size(); idx++)
	{
		if (idx == 0)	// 초기화
		{
			conInfo.coorX_start = pixels[idx].x;
			conInfo.coorX_end = pixels[idx].x;
			conInfo.coorY_start = pixels[idx].y;
			conInfo.coorY_end = pixels[idx].y;
			conInfo.pixelCount = pixels.size();
		}

		if (pixels[idx].x < conInfo.coorX_start)
			conInfo.coorX_start = pixels[idx].x;
		if (pixels[idx].x > conInfo.coorX_end)
			conInfo.coorX_end = pixels[idx].x;
		if (pixels[idx].y < conInfo.coorY_start)
			conInfo.coorY_start = pixels[idx].y;
		if (pixels[idx].y > conInfo.coorY_end)
			conInfo.coorY_end = pixels[idx].y;
	}
	return conInfo;
}

void PeakFinder::makeExpectedLineInfos()
{
	vector<contourLineInfo> conLineInfo;

	conLineInfo = getLineInfoFromContourInfos(contourMaxBinImage_contourInfos);		// 컨투어를 라인으로 묶기만 함

	contourMaxBinImage_expectedLineInfo = expectedLineInfoAfterProcess(conLineInfo);	// 라인 후처리(필터)
}

vector<contourLineInfo> PeakFinder::expectedLineInfoAfterProcess(vector<contourLineInfo> conLineInfos)
{

	for (int i = 0; i < conLineInfos.size(); i++)	// 1.
	{
		for (int j = 0; j < conLineInfos[i].contours.size(); j++)
		{
			if (conLineInfos[i].coorY_start > conLineInfos[i].contours[j].coorY_start)
				conLineInfos[i].coorY_start = conLineInfos[i].contours[j].coorY_start;	//최소값

			if (conLineInfos[i].coorY_end < conLineInfos[i].contours[j].coorY_end)
				conLineInfos[i].coorY_end = conLineInfos[i].contours[j].coorY_end;	// 최대값

			if (conLineInfos[i].coorX_start > conLineInfos[i].contours[j].coorX_start)
				conLineInfos[i].coorX_start = conLineInfos[i].contours[j].coorX_start;	//최소값

			if (conLineInfos[i].coorX_end < conLineInfos[i].contours[j].coorX_end)
				conLineInfos[i].coorX_end = conLineInfos[i].contours[j].coorX_end;	// 최대값
		}
	}

	vector<contourLineInfo> conLineInfos_buffer;
	for (int i = 0; i < conLineInfos.size(); i++)	// 2.
	{
		bool isPassOnSize = false;
		bool isPassOnVolume = false;
		bool isPassOnMaxWeight = false;
		if (conLineInfos[i].contours.size() >= 5)
			isPassOnSize = true;
		if (conLineInfos[i].pixelCount >= 100)
			isPassOnVolume = true;
		if(getMaxValue(conLineInfos[i]) >= 5)
			isPassOnVolume = true;

		if (isPassOnSize && isPassOnVolume && isPassOnVolume)
			conLineInfos_buffer.push_back(conLineInfos[i]);
	}
	conLineInfos = conLineInfos_buffer;
	conLineInfos_buffer.clear();

	for (int i = 0; i < conLineInfos.size(); i++)	// 3. 
	{
		bool isMerged = false;
		for (int j = 0; j < conLineInfos_buffer.size(); j++)
		{
			if (imageHandler::isRelation(conLineInfos_buffer[j].coorY_start, conLineInfos_buffer[j].coorY_end, conLineInfos[i].coorY_start, conLineInfos[i].coorY_end))
			{
				if (conLineInfos_buffer[j].coorY_start > conLineInfos[i].coorY_start)
					conLineInfos_buffer[j].coorY_start = conLineInfos[i].coorY_start;

				if (conLineInfos_buffer[j].coorY_end < conLineInfos[i].coorY_end)
					conLineInfos_buffer[j].coorY_end = conLineInfos[i].coorY_end;

				for (int idx = 0; idx < conLineInfos[i].contours.size(); idx++)
				{
					conLineInfos_buffer[j].contours.push_back(conLineInfos[i].contours[idx]);
				}
				sort(conLineInfos_buffer[j].contours.begin(), conLineInfos_buffer[j].contours.end(), imageHandler::asc_contourInfo);

				isMerged = true;
				break;
			}
		}
		if (isMerged != true)
		{
			conLineInfos_buffer.push_back(conLineInfos[i]);
		}
	}
	conLineInfos = conLineInfos_buffer;
	conLineInfos_buffer.clear();

	for (int i = 0; i < conLineInfos.size(); i++)	// 4.
	{
		conLineInfos[i].contours.erase(
			unique(conLineInfos[i].contours.begin(), conLineInfos[i].contours.end(),
				[](const contourInfo& a, const contourInfo& b) {
			if (a.maxValue == b.maxValue &&
				a.coorX_start == b.coorX_start &&
				a.coorX_end == b.coorX_end &&
				a.coorY_start == b.coorY_start &&
				a.coorY_end == b.coorY_end)
				return true;
			else
				return false;
		}	  // 중복제거	 
		), conLineInfos[i].contours.end());
	}

	for (int i = 0; i < conLineInfos.size(); i++)	// 5.
	{
		conLineInfos[i].pixelCount = imageHandler::getContourLineInfoVolume(conLineInfos[i]);
		conLineInfos[i].weightMat = WeightMat(getLineInfoAreaCuttedImage(conLineInfos[i]), frameNumber);
		conLineInfos[i].weightMat_maximum = WeightMat(getLineInfoAreaCuttedImage(conLineInfos[i]), frameNumber);
		conLineInfos[i].maxValue = getMaxValue(conLineInfos[i]);
	}

	for (int i = 0; i < conLineInfos.size(); i++)	// 2.
	{
		bool isPassOnSize = false;
		bool isPassOnVolume = false;
		bool isPassOnMaxWeight = false;
		if (conLineInfos[i].contours.size() >= 5)
			isPassOnSize = true;
		if (conLineInfos[i].pixelCount >= 100)
			isPassOnVolume = true;
		if (getMaxValue(conLineInfos[i]) >= 5)
			isPassOnVolume = true;

		if (isPassOnSize && isPassOnVolume && isPassOnVolume)
			conLineInfos_buffer.push_back(conLineInfos[i]);
	}
	conLineInfos = conLineInfos_buffer;
	conLineInfos_buffer.clear();

	return conLineInfos;	

	// >> 현제 프래임에서 예상라인을 찾아냄
	// 1. contourLineInfo의 coorY_start와 coorY_end를 모든 컨투어에 대한 최대-최소값으로 바꿈
	// 2. vecContours.Size()가 5 이상인 것만 살림 & 픽셀카운트가 100 이상인것만 살림 & 최대 weight가 5 이상인것만 살림
	// 3. contourLineInfo끼리 Y값이 겹치는 부분이 있으면 합치고(merge) vecContours 다시 소팅 -> 클러스터링 된 것만 남음
	// 4. contours중복 제거
	// 5. 부피 계산 & 관련 이미지 생성 & 최대값 입력
	// 2. (한번더)vecContours.Size()가 5 이상인 것만 살림 & 픽셀카운트가 100 이상인것만 살림 & 최대 weight가 5 이상인것만 살림
	// 6. 해당 값 리턴 ( 필요하다면 조건 추가 및 변경할것!)

}

vector<contourLineInfo> PeakFinder::getJudgeLineByFrameFlow()
{
	vector<contourLineInfo> judgedLine;

	for (int i = 0; i < expectedLineInfos.size(); i++)	// 1.
	{
		bool isFind = false;
		for (int j = 0; j < contourMaxBinImage_expectedLineInfo.size(); j++)	// 이번에 프레임에 찾아낸 라인
		{
			bool isRelative = imageHandler::isRelation(expectedLineInfos[i].first.coorY_start, expectedLineInfos[i].first.coorY_end, contourMaxBinImage_expectedLineInfo[j].coorY_start, contourMaxBinImage_expectedLineInfo[j].coorY_end);
			if (isRelative)	// 이번프레임에도 좌표에 존재함
			{
				isFind = true;
				if (expectedLineInfos[i].first.pixelCount / 2 > contourMaxBinImage_expectedLineInfo[j].pixelCount)
				{	//
					expectedLineInfos[i].second++;	// 존재하지만 픽셀수가 절반이하로 줄어듦
				}
				else if (expectedLineInfos[i].first.maxValue > contourMaxBinImage_expectedLineInfo[j].maxValue)	// maxvalue가 이전보다 낮음
				{
					expectedLineInfos[i].second++;
				}
				else 
				{
					if (expectedLineInfos[i].second == 0)	// 연속으로 0일때만 정보 업데이트
					{
						int pixelCount = imageHandler::getWhitePixelCount(expectedLineInfos[i].first.weightMat_maximum.binImage);	// 픽셀수가 더 많다면 이미지 유지
						if (pixelCount > contourMaxBinImage_expectedLineInfo[j].pixelCount)
						{
							WeightMat temp = expectedLineInfos[i].first.weightMat_maximum;
							expectedLineInfos[i].first = contourMaxBinImage_expectedLineInfo[j];
							expectedLineInfos[i].first.weightMat_maximum = temp;
						}
						else
							expectedLineInfos[i].first = contourMaxBinImage_expectedLineInfo[j];
						//
					}
					expectedLineInfos[i].second = 0;
				}
				;// 카운트 초기화 : Count = 0;
			}
		}
		if (isFind != true)	// 이번프레임에 없음
		{
			expectedLineInfos[i].second++;
			;	// 카운트 증가 : Count++;
		}
	}

	// 2. 
	for (int i = 0; i < expectedLineInfos.size(); i++)	// 
	{
		expectedLineInfos[i].first.coorY_start;
		expectedLineInfos[i].first.coorY_end;
		expectedLineInfos[i].first.pixelCount;
		expectedLineInfos.erase(
			unique(expectedLineInfos.begin(), expectedLineInfos.end(),
				[](const pair<contourLineInfo, int>& a, const pair<contourLineInfo, int>& b) {
			if (a.first.coorY_start == b.first.coorY_start &&
				a.first.coorY_end == b.first.coorY_end &&
				a.first.pixelCount == b.first.pixelCount)
				return true;
			else
				return false;
		}
		), expectedLineInfos.end());
	}

	for (int i = 0; i < contourMaxBinImage_expectedLineInfo.size(); i++)	// 3. 
	{
		bool isFind = false;
		for (int j = 0; j < expectedLineInfos.size(); j++)
		{
			bool isRelative = imageHandler::isRelation(contourMaxBinImage_expectedLineInfo[i].coorY_start, contourMaxBinImage_expectedLineInfo[i].coorY_end, expectedLineInfos[j].first.coorY_start, expectedLineInfos[j].first.coorY_end);
			if (isRelative)	// 
			{
				isFind = true;
				break;
			}
		}
		if (isFind != true)	// 못찾았을 경우  
		{
			expectedLineInfos.push_back(make_pair(contourMaxBinImage_expectedLineInfo[i], 0));
			printf(" [Line Add] ");
			; // 라인 추가 : addLine()
		}
	}

	vector<pair< contourLineInfo, int>> expectedLineInfos_temp;
	for (int i = 0; i < expectedLineInfos.size(); i++)	// 4. 
	{
		if (expectedLineInfos[i].second >= PeakFinder::JUDGE_TIMEOUT)	// timeOut이 5이상이면 라인으로 처리 
		{
			judgedLine.push_back(expectedLineInfos[i].first);
			; // expectedLineInfos 확정				
		}
		else
		{
			expectedLineInfos_temp.push_back(expectedLineInfos[i]);
		}
	}
	expectedLineInfos = expectedLineInfos_temp;

	// 5. 중복 제거
	sort(judgedLine.begin(), judgedLine.end(), imageHandler::asc_contourLineInfo);
	for (int i = 0; i < judgedLine.size(); i++)
	{
		judgedLine.erase(
			unique(judgedLine.begin(), judgedLine.end(),
				[](const contourLineInfo& a, const contourLineInfo& b) {
			if (a.coorX_start == b.coorX_start &&
				a.coorX_end == b.coorX_end &&
				a.coorY_start == b.coorY_start &&
				a.coorY_end == b.coorY_end &&
				a.pixelCount == b.pixelCount)
				return true;
			else
				return false;
		}
		), judgedLine.end());
	}

	return judgedLine;

	// >> frame 흐름에 있어서 라인으로 판별
	// 1. 받은라인정보가 기존 라인에 있다면 업데이트 and 없어진 라인 count++
	// 2. 업데이트 된 라인 중 중복된 라인이 있다면 삭제  (두개의 라인이 진행되다 하나로 합쳐지기도 함.)
	// 3. 신규 라인 찾아 추가 
	// 4. timeOut count가 5 이상인것들 라인으로 최종판결 (이하인 것 라인으로 보지않음) + 조건(MaxValue, etc..)
	// 5. 중복 제거 

	//printf(" ( %d : %d ) ", expectedLineInfos_buffer_toLine.size(), expectedLineInfos_buffer_toMaintain.size());
}

int PeakFinder::getMaxValue(contourLineInfo lineInfo)
{
	int maxValue = 0;
	for (int i = 0; i < lineInfo.contours.size(); i++)
	{
		if (lineInfo.contours[i].maxValue > maxValue)
			maxValue = lineInfo.contours[i].maxValue;
	}
	return maxValue;
}


