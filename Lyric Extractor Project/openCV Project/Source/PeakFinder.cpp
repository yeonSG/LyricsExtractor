#include "PeakFinder.h"
#include "loger.h"

const int PeakFinder::JUDGE_TIMEOUT = 5;

vector<contourLineInfoSet> PeakFinder::frameImage_process(Mat frameImage, int frameNumber, Scalar targetColor, Mat refUnprintImage)
{	
	Mat patternFill = imageHandler::getPatternFillImage_2(frameImage, targetColor);
	this->m_frameNumber = frameNumber;
	this->m_refUnprintColorWeight = refUnprintImage;

	//bitwise_and(patternFill, refUnprintImage, patternFill);
	vector<contourLineInfoSet> foundExpectedLines;

	if (m_stackBinImage.empty())
	{	// get dummy
		//stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_16U);
		m_stackBinImage = Mat::zeros(patternFill.rows, patternFill.cols, CV_8U);
	}
	else
	{
		Mat bin_refUnprintImage;
		inRange(refUnprintImage, 0, 2, bin_refUnprintImage);// 최근 3프래임중 흰색이었던곳

		m_stackBinImage = stackBinImage(m_stackBinImage, patternFill, bin_refUnprintImage);

		m_weightSumAtBinImage = imageHandler::getSumOfBinImageValues(m_stackBinImage);
		m_colorPixelSumAtBinImage = imageHandler::getWhitePixelCount(m_stackBinImage);

		makeContourMaxBinImageAndContourInfos();
		makeExpectedLineInfos();
		printf("[found CLine %d]", m_contourMaxBinImage_expectedLineInfo.size());

		foundExpectedLines = getJudgeLineByFrameFlow();	// 
	}

	return foundExpectedLines;
}

// x라인이 확정되면 확정된 이미지보다 여전히 큰 값을 가지는 점을 보정해주어 
// 라인이 확정되면 해당하는 y축의 binImage를 0으로 만듦.
// 이 후 라인 판별될 때 앞에 라인까지 잡아먹는 현상을 제거하기 위함
void PeakFinder::stackBinImageCorrect(Mat validImage)
{
	Mat corrImage = m_stackBinImage.clone();
	int height = corrImage.rows;
	int width = corrImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_corr = corrImage.ptr<uchar>(y);
		uchar* yPtr_valid = validImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr_corr[x] > yPtr_valid[x] && yPtr_valid[x]!=0)	// stackBin[x]의 값이 vaild[x]보다 크고 vaild[x]가 0이 아닌 곳 조정함
			{
				int temp = yPtr_corr[x] - (yPtr_valid[x] + JUDGE_TIMEOUT);
				if (temp < 0)	// 연산 값이 - 이면 연산하지않음
					;//yPtr_corr[x] = 0;
				else 
					yPtr_corr[x] = temp;
				//yPtr_corr[x] = yPtr_corr[x] - (yPtr_valid[x]+JUDGE_TIMEOUT);
				
			}
		}
	}

	m_stackBinImage = corrImage.clone();
}

Mat PeakFinder::stackBinImage(Mat stackBinImage, Mat patternImage, Mat refUnprintImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_stack = stackBinImage.ptr<uchar>(y);
		uchar* yPtr_pattern = patternImage.ptr<uchar>(y);
		uchar* yPtr_refUnprint = refUnprintImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			// 누적법
			// 패턴이 검정 -> 0처리
			// 패턴이 흰색 and Unprint가 흰색이었던곳 -> 1처리 (시작)
			// 스택이미지가 이미 칠해져 있으면서 패턴인곳

			if (yPtr_pattern[x] == 0)	// 0인곳  On 조건
			{
				yPtr_stack[x] = 0;
			}
			else if (yPtr_pattern[x] != 0 && yPtr_stack[x] != 0)	// 패턴이면서,
			{
				yPtr_stack[x] += 1;
			}
			else if (yPtr_pattern[x] != 0 && yPtr_refUnprint[x] != 0)	// 패턴이면서 흰색이었던곳 (시작조건)
			{
				yPtr_stack[x] = 1;
			}

		}
	}
	return stackBinImage;
}

// 컨투어 맥스 이미지 생성과 컨투어 정보 생성
void PeakFinder::makeContourMaxBinImageAndContourInfos()
{
	Mat contourMaxImage;
	m_contourMaxBinImage_contourInfos.clear();
	// m_contourMaxBinImage_contourInfos = contourInfo::getContourInfosFromBinImage(m_stackBinImage, contourMaxImage);
	// m_contourMaxBinImage = contourMaxImage.clone();
	
	Mat outImage = Mat::zeros(m_stackBinImage.rows, m_stackBinImage.cols, CV_8U);
	m_contourMaxBinImage_contourInfos.clear();

	//Mat unPrintFillteredImage = getUnprintFillteredstackBinImage(m_stackBinImage, m_refUnprintColorWeight);
	
	Mat binTempImage;
	inRange(m_stackBinImage, 1, 255, binTempImage);
	//inRange(unPrintFillteredImage, 1, 255, binTempImage);
	binTempImage = imageHandler::getDustRemovedImage(binTempImage);	// 부피가 3보다 작은것 삭제함

	vector<vector<Point>> contours;
	findContours(binTempImage, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(m_stackBinImage.rows, m_stackBinImage.cols, CV_8U);;
		// 컨투어별 마스크
		vector<vector<Point>> contours_picked;
		contours_picked.push_back(contours[i]);
		fillPoly(contourMask, contours_picked, 255);
		vector<Point> indices;		// 내부의 점들을 얻음
		bitwise_and(contourMask, binTempImage, contourMask);
		findNonZero(contourMask, indices);
		
		//int sum = 0;
		int max = 0;
		for (int idx = 0; idx < indices.size(); idx++)
		{
			uchar* yPtr = m_stackBinImage.ptr<uchar>(indices[idx].y);	//in
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
		m_contourMaxBinImage_contourInfos.push_back(conInfo);
	}
	sort(m_contourMaxBinImage_contourInfos.begin(), m_contourMaxBinImage_contourInfos.end(), imageHandler::asc_contourInfo);
	m_contourMaxBinImage = outImage.clone();
	

}

Mat PeakFinder::getUnprintFillteredstackBinImage(Mat weightPaint, Mat weightUnpaint)
{
	// m_refUnprintColorWeight;
	// m_stackBinImage;
	Mat outImage = Mat::zeros(weightPaint.rows, weightPaint.cols, CV_8U);

	int height = weightPaint.rows;
	int width = weightPaint.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_p = weightPaint.ptr<uchar>(y);
		uchar* yPtr_unp = weightUnpaint.ptr<uchar>(y);
		uchar* yPtr_out = outImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			int diff = yPtr_p[x] - yPtr_unp[x];
			if (diff <= 10)	// p가 np보다 크거나 같은값일 때
			{
				yPtr_out[x] = yPtr_p[x];
			}
		}
	}

	Mat test_outImage;
	Mat test_printImage;
	inRange(weightPaint, 1, 255, test_printImage);
	inRange(outImage, 1, 255, test_outImage);
	return outImage;
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
	Mat maskImage = Mat::zeros(m_contourMaxBinImage.rows, m_contourMaxBinImage.cols, CV_8U);
	maskImage = imageHandler::getWhiteMaskImage(maskImage, LineInfo.coorX_start, LineInfo.coorY_start, LineInfo.coorX_end - LineInfo.coorX_start, LineInfo.coorY_end - LineInfo.coorY_start);

	Mat maskedImage;
	bitwise_and(maskImage, m_contourMaxBinImage, maskedImage);

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

	conLineInfo = getLineInfoFromContourInfos(m_contourMaxBinImage_contourInfos);		// 컨투어를 라인으로 묶기만 함

	if (conLineInfo.size() != 0)
	{
		m_contourMaxBinImage_expectedLineInfo = expectedLineInfoAfterProcess(conLineInfo);	// 라인 후처리(필터)
	}
}

vector<contourLineInfoSet> PeakFinder::expectedLineInfoAfterProcess(vector<contourLineInfo> conLineInfos)
{
	vector<contourLineInfoSet> outExpectedLine;

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
		conLineInfos[i].maxValue = getMaxValue(conLineInfos[i]);
		conLineInfos[i].weightMat					= WeightMat(getLineInfoAreaCuttedImage(conLineInfos[i]), m_frameNumber);
		conLineInfos[i].weightMat_Unprint			= WeightMat(m_refUnprintColorWeight.clone(), m_frameNumber);
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

	for (int i = 0; i < conLineInfos_buffer.size(); i++)
		outExpectedLine.push_back(contourLineInfoSet(conLineInfos_buffer[i]));

	conLineInfos_buffer.clear();

	return outExpectedLine;

	// >> 현제 프래임에서 예상라인을 찾아냄
	// 1. contourLineInfo의 coorY_start와 coorY_end를 모든 컨투어에 대한 최대-최소값으로 바꿈
	// 2. vecContours.Size()가 5 이상인 것만 살림 & 픽셀카운트가 100 이상인것만 살림 & 최대 weight가 5 이상인것만 살림
	// 3. contourLineInfo끼리 Y값이 겹치는 부분이 있으면 합치고(merge) vecContours 다시 소팅 -> 클러스터링 된 것만 남음
	// 4. contours중복 제거
	// 5. 부피 계산 & 관련 이미지 생성 & 최대값 입력
	// 2. (한번더)vecContours.Size()가 5 이상인 것만 살림 & 픽셀카운트가 100 이상인것만 살림 & 최대 weight가 5 이상인것만 살림
	// 6. 해당 값 리턴 ( 필요하다면 조건 추가 및 변경할것!)

}

vector<contourLineInfoSet> PeakFinder::getJudgeLineByFrameFlow()
{
	vector<contourLineInfoSet> judgedLine;

	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 1.
	{
		bool isFind = false;
		for (int j = 0; j < m_contourMaxBinImage_expectedLineInfo.size(); j++)	// 이번에 프레임에 찾아낸 라인
		{
			bool isRelative = imageHandler::isRelation(m_expectedLineInfos[i].first.progress.coorY_start, m_expectedLineInfos[i].first.progress.coorY_end, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_start, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_end);
			if (isRelative)	// 이번프레임에도 좌표에 존재함
			{
				isFind = true;
				if (m_expectedLineInfos[i].first.progress.pixelCount / 2 > m_contourMaxBinImage_expectedLineInfo[j].progress.pixelCount)
				{	//
					m_expectedLineInfos[i].second++;	// 존재하지만 픽셀수가 절반이하로 줄어듦
				}
				else if (m_expectedLineInfos[i].first.progress.maxValue > m_contourMaxBinImage_expectedLineInfo[j].progress.maxValue)	// maxvalue가 이전보다 낮음
				{
					m_expectedLineInfos[i].second++;
				}
				else 
				{
					if (m_expectedLineInfos[i].second == 0)	// 연속으로 0일때만 정보 업데이트
					{
						int pixelCount = m_expectedLineInfos[i].first.maximum.pixelCount; //weightMat_maximum.binImage);	// 픽셀수가 더 많다면 이미지 유지
						if (pixelCount > m_contourMaxBinImage_expectedLineInfo[j].progress.pixelCount)	// 맥시멈이 progress보다 큼 -> 맥시멈은 유지
						{
							m_expectedLineInfos[i].first.progress = m_expectedLineInfos[i].first.progress;
						}
						else // 맥시멈, 프로그래스 둘다 업데이트
						{
							m_expectedLineInfos[i].first = m_contourMaxBinImage_expectedLineInfo[j];	
						}
						//
					}
					m_expectedLineInfos[i].second = 0;	// 카운트 초기화
				}
				;
			}
		}
		if (isFind != true)	// 이번프레임에 없음
		{
			m_expectedLineInfos[i].second++;
			;	// 카운트 증가 : Count++;
		}
	}

	// 2. 
	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 
	{
		m_expectedLineInfos[i].first.progress.coorY_start;
		m_expectedLineInfos[i].first.progress.coorY_end;
		m_expectedLineInfos[i].first.progress.pixelCount;

		m_expectedLineInfos.erase(
			unique(m_expectedLineInfos.begin(), m_expectedLineInfos.end(),
				[](const pair<contourLineInfoSet, int>& a, const pair<contourLineInfoSet, int>& b) {
			if (a.first.progress.coorY_start == b.first.progress.coorY_start &&
				a.first.progress.coorY_end == b.first.progress.coorY_end &&
				a.first.progress.pixelCount == b.first.progress.pixelCount)
				return true;
			else
				return false;
		}
		), m_expectedLineInfos.end());
	}

	for (int i = 0; i < m_contourMaxBinImage_expectedLineInfo.size(); i++)	// 3. 
	{
		bool isFind = false;
		for (int j = 0; j < m_expectedLineInfos.size(); j++)
		{
			bool isRelative = imageHandler::isRelation(m_contourMaxBinImage_expectedLineInfo[i].progress.coorY_start, m_contourMaxBinImage_expectedLineInfo[i].progress.coorY_end, m_expectedLineInfos[j].first.progress.coorY_start, m_expectedLineInfos[j].first.progress.coorY_end);
			if (isRelative)	// 
			{
				isFind = true;
				break;
			}
		}
		if (isFind != true)	// 못찾았을 경우 = 새로운 라인
		{
			m_expectedLineInfos.push_back(make_pair(m_contourMaxBinImage_expectedLineInfo[i], 0));
			printf(" [Line Add] ");
			; // 라인 추가 : addLine()
		}
	}

	vector<pair< contourLineInfoSet, int>> expectedLineInfos_temp;
	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 4. 
	{
		if (m_expectedLineInfos[i].second >= PeakFinder::JUDGE_TIMEOUT)	// timeOut이 5이상이면 라인으로 처리 
		{
			judgedLine.push_back(m_expectedLineInfos[i].first);
			; // expectedLineInfos 확정				
		}
		else
		{
			expectedLineInfos_temp.push_back(m_expectedLineInfos[i]);
		}
	}
	m_expectedLineInfos = expectedLineInfos_temp;

	// 5. 중복 제거
	sort(judgedLine.begin(), judgedLine.end(), imageHandler::asc_contourLineInfo);
	for (int i = 0; i < judgedLine.size(); i++)
	{
		judgedLine.erase(
			unique(judgedLine.begin(), judgedLine.end(),
				[](const contourLineInfoSet& a, const contourLineInfoSet& b) {
			if (a.progress.coorX_start == b.progress.coorX_start &&
				a.progress.coorX_end == b.progress.coorX_end &&
				a.progress.coorY_start == b.progress.coorY_start &&
				a.progress.coorY_end == b.progress.coorY_end &&
				a.progress.pixelCount == b.progress.pixelCount)
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


