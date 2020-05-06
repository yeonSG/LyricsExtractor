﻿#include "LineInfoFinder.h"
#include "loger.h"

bool LineInfoFinder::start()
{
	bool isSucess = false;

	Mat orgImage;
	Mat subImage;


	//videoCapture->set(CAP_PROP_POS_FRAMES, (double)0);
	//while (videoCapture->read(orgImage))	
	//{
	//	/* 라인 분석 - get ExpectationLine */
	//	int curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
	//	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

	//	subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

	//	// 모든 라인 분석
	//	for (int i = 0; i < vecPrintTypes.size(); i++)	// 모든 Paint Type에 대한 연산 수행	0~2(Blue, Red, Purple)
	//	{
	//		Mat patternFill = getPatternFillImage(subImage, vecPrintTypes[i]);
	//		int patternPixelCount = imageHandler::getWhitePixelCount(patternFill);
	//		vecPrintTypes_PatternPixelCount[i].push_back(patternPixelCount);
	//	}
	//}

	//vector<vector<int>> peaks;
	//for (int i = 0; i < vecPrintTypes_PatternPixelCount.size(); i++)
	//{
	//	peaks.push_back(getPeak(vecPrintTypes_PatternPixelCount[i]));
	//}

	/* 라인 분석 심화 - ExpectationLine 에서 뒤로 가기 하면서 pixel 줄어드는거 파악*/

	// 1. 3개의 모든 피크를 구해( fade-out도 탐지되도록, )
	// 2. 해당 피크에서 마스크를 구하고 [printTypePixelCount[type_Print] 의 50% 아래가 되는 프레임] 인곳을 분석 => vecPrintTypes색이 왼쪽에 몰려있고, 흰색또는 주황색이 몰린 곳 파악 (파악에 부합하지 않으면 삭제)
	
	// 3. 그걸 라인으로 따서 라인마다 정보 넣기 ()

	/*
	목표는 가사 라인따기
	1. 어떻게 라인을 따는가?
	 - 라인이 있는 곳을 찾아
	 - 라인에 대한 공통점들 :	왼쪽에서 오른쪽으로 색칠됨. (흰, 주황 => 파, 빨, 보라)
								가로로 늘어져있음.

	 - 5f 씩 진행, whiteCount가 증가함, whiteCount의 위치


	 (5f씩 증가하며 pixelCount 측정 and 증가한 곳에 대한 평균 좌표 ) == [frame, pixelCount, avgXcoordinate];

	 frameInfo[frame, pixelCount, avgXcoordinate, avgXCoordinate_added];

	*/
	vector<FrameInfo> frameInfos[3];	// Blue, Red, Purple
	Mat beforeFrameBinImages[3];

	videoCapture->set(CAP_PROP_POS_FRAMES, (double)0);
	while (videoCapture->read(orgImage))
	{
		/* 라인 분석 - get ExpectationLine */
		int curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame +4);

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		// 모든 라인 분석
		for (int i = 0; i < vecPrintTypes.size(); i++)	// 모든 Paint Type에 대한 연산 수행	0~2(Blue, Red, Purple)
		{
			FrameInfo fInfo;
			fInfo.frame = curFrame;
			Mat patternFill = getPatternFillImage(subImage, vecPrintTypes[i]);

			if (beforeFrameBinImages[i].empty())
			{	// get dummy
				fInfo.pixelCount = 0;
				fInfo.avgCoordinate = 0;
				fInfo.added_pixelCount = 0;
				fInfo.added_avgCoordinate = 0;
			}
			else
			{
				fInfo.pixelCount = imageHandler::getWhitePixelCount(patternFill);
				fInfo.avgCoordinate = imageHandler::getWhitePixelAvgCoordinate(patternFill, true);

				Mat BlackToWhiteImage = imageHandler::getBlackToWhiteImage(beforeFrameBinImages[i], patternFill);

				fInfo.added_pixelCount = imageHandler::getWhitePixelCount(BlackToWhiteImage);
				fInfo.added_avgCoordinate = imageHandler::getWhitePixelAvgCoordinate(BlackToWhiteImage, true);
			}
			beforeFrameBinImages[i] = patternFill.clone();

			frameInfos[i].push_back(fInfo);
		}
	}

	for (int i = 0; i < vecPrintTypes.size(); i++)
	{
		printf("	vecPrintTypes : %d\r\n", i);
		printf("	frame	pixCnt	avgCoor	addPixCnt	AddAvgCoor \r\n");
		for (int j = 0; j < frameInfos[i].size(); j++)
		{
			printf("frame : %d	%d	%d	%d	%d\r\n",
				frameInfos[i].at(j).frame,
				frameInfos[i].at(j).pixelCount,
				frameInfos[i].at(j).avgCoordinate,
				frameInfos[i].at(j).added_pixelCount,
				frameInfos[i].at(j).added_avgCoordinate);
		}
	}

	/*	라인 판별 조건
		3(15f)연속 증가(whiteCount 와 addPixCoor 둘 다),
		addAvgCoor 증가,
	
		시작 지점의 Coor는 중간 X좌표 이하,
		끝 지점의 Coor는 중간 X좌표 이상,


	*/

	isSucess = true;
	return isSucess;
}

bool LineInfoFinder::start2()
{
	bool isSucess = false;

	Mat orgImage;
	Mat subImage;

	vector<int> whitePixelCount[3];
	vector<uint> whiteValueSum[3];
	Mat stackBinImages[3];

	Mat  maximumStat_mat[3];			// 
	uint maximumStat_maxValue[3] = { 0, };		// 조건1.
	int  maximumStat_frameNum[3] = { 0, };		// 조건2. max값 대비 절반 떡락시 maxinum값으로 분석하여 맞으면 push_하여 저장

	videoCapture->set(CAP_PROP_POS_FRAMES, (double)0);
	while (videoCapture->read(orgImage))
	{
		/* 라인 분석 - get ExpectationLine */
		int curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		// 모든 라인 분석
		for (int i = 0; i < vecPrintTypes.size(); i++)	// 모든 Paint Type에 대한 연산 수행	0~2(Blue, Red, Purple)
		{
			Mat patternFill = getPatternFillImage(subImage, vecPrintTypes[i]);

			if (stackBinImages[i].empty())
			{	// get dummy
				whitePixelCount[i].push_back(0);
				whiteValueSum[i].push_back(0);
				stackBinImages[i] = Mat::zeros(patternFill.rows, patternFill.cols, CV_16U);
			}
			else
			{
				stackBinImages[i] = imageHandler::stackBinImage(stackBinImages[i], patternFill);
				whiteValueSum[i].push_back(imageHandler::getSumOfBinImageValues(stackBinImages[i]));

				if (maximumStat_maxValue[i] >= whiteValueSum[i].back())	// 마지막 값보다 맥스값이 더 큼 => 업데이트
				{
					maximumStat_mat[i]		= stackBinImages[i];
					maximumStat_maxValue[i]	= imageHandler::getMaximumValue(stackBinImages[i]);
					maximumStat_frameNum[i] = i;
				}
				else if(maximumStat_maxValue[i]/2 > whiteValueSum[i].back())	// 최대값 대비 절반 이하가 되버렸음 => 라인인지 검사
				{

				}
			}
		}
		printf("frame... %d \r\n", curFrame);
	}

	for (int i = 0; i < vecPrintTypes.size(); i++)
	{
		printf("	vecPrintTypes : %d\r\n", i);
		printf("	frame	pixCnt	\r\n");
		for (int j = 0; j < whiteValueSum[i].size(); j++)
		{
			printf("frame : %d	%d \r\n",
				j,
				whiteValueSum[i].at(j) );
		}
	}


	/*	라인 판별 조건
			-  whiteValueSum 의 피크
			- // 피크부분에서 스택이미지 분석
				: (최소값과 최대값의 비율로 약 10군데로 구분지음 예] 0~100, 101~200, 201~300 ... )
			- 

			if(whiteValueSum[i]/5 <
	*/

	isSucess = true;
	return isSucess;
}


vector<PeakInfo> LineInfoFinder::start2_getLinePeak(int PrintTypeNum)
{
	vector<PeakInfo> linePeaks;

	Mat orgImage;
	Mat subImage;

	Mat stackBinImages;

	bool darkMode = true;
	int  darkMode_increaseCount = 0;
	uint DarkMode_BeforeWeightSum = 0;
	int  DarkMode_BeforePixelSum = 0;

	Mat  maximumStat_mat;			// 
	uint maximumStat_WeightSumValue = 0;		// 
	int  maximumStat_frameNum = 0;		// 

	int curFrame = 0;		
	//int curFrame = 3900;
	//curFrame = 0;

	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
	while (videoCapture->read(orgImage))
	{
		 //if (curFrame > 3000)
		 //	return linePeaks; //return linePeaks;	// for test
		/* 라인 분석 - get ExpectationLine */
		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		Mat patternFill = getPatternFillImage(subImage, vecPrintTypes[PrintTypeNum]);
		uint weightSumAtBinImage = 0;
		int colorPixelSumAtBinImage  = 0;

		if (stackBinImages.empty())
		{	// get dummy
			//stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_16U);
			stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_8U);
		}
		else
		{
			stackBinImages = imageHandler::stackBinImage(stackBinImages, patternFill);
			weightSumAtBinImage = imageHandler::getSumOfBinImageValues(stackBinImages);
			colorPixelSumAtBinImage = imageHandler::getWhitePixelCount(stackBinImages);

			if (darkMode == false)
			{
				if (maximumStat_WeightSumValue <= weightSumAtBinImage)			// 마지막 값보다 맥스값이 더 큼 => 업데이트
				{
					maximumStat_mat = stackBinImages.clone();	// 검은점이 아닌 점 수, 가장오래된점값(offset)
					maximumStat_WeightSumValue = weightSumAtBinImage;
					maximumStat_frameNum = curFrame;
				}
				else if (maximumStat_WeightSumValue / 2 > weightSumAtBinImage)	// 최대값 대비 절반 이하가 되버렸음 => 라인인지 검사
				{
					bool isPeak = false;
					isPeak = isPeakFrame2(maximumStat_mat);
					printf("@");
					// 라인인지 검사		linePeaks

					if (isPeak == true)	//
					{
						PeakInfo peakInfo;
						peakInfo.frameNum = maximumStat_frameNum;
						peakInfo.maxWeightPixel = imageHandler::getMaximumValue(maximumStat_mat);
						peakInfo.PeakImage = maximumStat_mat.clone();
						stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_8U);

						linePeaks.push_back(peakInfo);
						printf("$(Save.. %d)", maximumStat_frameNum);
						//if (curFrame > 1140)
						//	return linePeaks; //return linePeaks;	// for test
					}

					{	// 결과에 상관없이 다크모드 진입
						maximumStat_mat = NULL;
						maximumStat_WeightSumValue = 0;
						maximumStat_frameNum = 0;

						darkMode = true;
						darkMode_increaseCount = 0;
						DarkMode_BeforeWeightSum = 0;
						DarkMode_BeforePixelSum = 0;
					}
				}
			}
			else // darkMode
			{
				if (weightSumAtBinImage >= DarkMode_BeforeWeightSum &&
					weightSumAtBinImage >0 &&
					colorPixelSumAtBinImage >= DarkMode_BeforePixelSum)	 // 5연속 증가시 다크모드 해제
				{
					darkMode_increaseCount++;
					if (darkMode_increaseCount >= 5)
					{
						darkMode = false;
						maximumStat_mat = stackBinImages.clone();	// 검은점이 아닌 점 수, 가장오래된점값(offset)
						maximumStat_WeightSumValue = weightSumAtBinImage;
						maximumStat_frameNum = curFrame;
					}
				}
				else
					darkMode_increaseCount = 0;

				DarkMode_BeforeWeightSum = weightSumAtBinImage;
				DarkMode_BeforePixelSum = colorPixelSumAtBinImage;
			}

		}

		printf("frame... %d : %d	%d ", curFrame, weightSumAtBinImage, colorPixelSumAtBinImage);
		printf(" DarkMode= %d	\r\n", darkMode_increaseCount);
	}

	return linePeaks;
}

// 피크인지 검사 
bool LineInfoFinder::isPeakFrame(Mat expectedFrame)
{
	bool isPeak = false;
	/*	1. maximumStat_mat의 최대값을 구함.
		2. 최대값을 기준으로 6개로 클러스터링
		3. 클러스터링 데이터들의 x좌표 평균을 구함 (클러스터링0~5까지, 0일수록 큰값)
		4. 조건 만족시 피크로 판단 ( 클러스터링0 > 클러스터링1 & 클러스터링1 > 클러스터링2 & 클러스터링2 & 클러스터링3  )	// 최대값과 가까운것들만 함
	*/
	uint maxValue = imageHandler::getMaximumValue(expectedFrame);
	if (maxValue < 10)
		return false;

	uint clusterValue = maxValue / 10;
	Mat clurster0;
	Mat clurster1;
	Mat clurster2;
	Mat clurster3;
	Mat clurster4;
	Mat clurster5;
	Mat clurster6;
	Mat clurster7;
	Mat clurster8;
	Mat clurster9;

	inRange(expectedFrame, clusterValue * 9 + 1, maxValue, clurster0);	// maxValue/6 *5 ~ *6	
	inRange(expectedFrame, clusterValue * 8 + 1, clusterValue * 9, clurster1);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, clusterValue * 7 + 1, clusterValue * 8, clurster2);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, clusterValue * 6 + 1, clusterValue * 7, clurster3);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, clusterValue * 5 + 1, clusterValue * 6, clurster4);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, clusterValue * 4 + 1, clusterValue * 5, clurster5);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, clusterValue * 3 + 1, clusterValue * 4, clurster6);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, clusterValue * 2 + 1, clusterValue * 3, clurster7);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, clusterValue * 1 + 1, clusterValue * 2, clurster8);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, 1, clusterValue, clurster9);	// maxValue/6 *0 ~ *1	

	Mat clurster0_DR ;
	Mat clurster1_DR ;
	Mat clurster2_DR ;
	Mat clurster3_DR ;
	Mat clurster4_DR ;
	Mat clurster5_DR ;
	Mat clurster6_DR ;
	Mat clurster7_DR ;
	Mat clurster8_DR ;
	Mat clurster9_DR ;

	int dotSize = 2;
	clurster0_DR = imageHandler::getDustRemovedImage(clurster0);
	clurster1_DR = imageHandler::getDustRemovedImage(clurster1);
	clurster2_DR = imageHandler::getDustRemovedImage(clurster2);
	clurster3_DR = imageHandler::getDustRemovedImage(clurster3);
	clurster4_DR = imageHandler::getDustRemovedImage(clurster4);
	clurster5_DR = imageHandler::getDustRemovedImage(clurster5);
	clurster6_DR = imageHandler::getDustRemovedImage(clurster6);
	clurster7_DR = imageHandler::getDustRemovedImage(clurster7);
	clurster8_DR = imageHandler::getDustRemovedImage(clurster8);
	clurster9_DR = imageHandler::getDustRemovedImage(clurster9);
	
	int clusterAvgPoint_0 = imageHandler::getWhitePixelAvgCoordinate(clurster0_DR, true);
	int clusterAvgPoint_1 = imageHandler::getWhitePixelAvgCoordinate(clurster1_DR, true);
	int clusterAvgPoint_2 = imageHandler::getWhitePixelAvgCoordinate(clurster2_DR, true);
	int clusterAvgPoint_3 = imageHandler::getWhitePixelAvgCoordinate(clurster3_DR, true);
	int clusterAvgPoint_4 = imageHandler::getWhitePixelAvgCoordinate(clurster4_DR, true);
	int clusterAvgPoint_5 = imageHandler::getWhitePixelAvgCoordinate(clurster5_DR, true);
	int clusterAvgPoint_6 = imageHandler::getWhitePixelAvgCoordinate(clurster6_DR, true);
	int clusterAvgPoint_7 = imageHandler::getWhitePixelAvgCoordinate(clurster7_DR, true);
	int clusterAvgPoint_8 = imageHandler::getWhitePixelAvgCoordinate(clurster8_DR, true);
	int clusterAvgPoint_9 = imageHandler::getWhitePixelAvgCoordinate(clurster9_DR, true);
	vector<int> clusterAvgPoint;
	clusterAvgPoint.push_back(clusterAvgPoint_0);
	clusterAvgPoint.push_back(clusterAvgPoint_1);
	clusterAvgPoint.push_back(clusterAvgPoint_2);
	clusterAvgPoint.push_back(clusterAvgPoint_3);
	clusterAvgPoint.push_back(clusterAvgPoint_4);
	clusterAvgPoint.push_back(clusterAvgPoint_5);
	clusterAvgPoint.push_back(clusterAvgPoint_6);
	clusterAvgPoint.push_back(clusterAvgPoint_7);
	clusterAvgPoint.push_back(clusterAvgPoint_8);
	clusterAvgPoint.push_back(clusterAvgPoint_9);

	/*
		- 컨투어 구하고 사이즈 1~2짜리 삭제함
		- 남은점이 있는 클러스터들 끼리 만 진행
		- 마지막 2~3개의 클러스터는 버려야함
	*/
	int clusterChk_cnt = 0;
	int clusterChk_val = 0;
	for (int i = 0; i < clusterAvgPoint.size() - 2; i++)	// 마지막 2개 클러스터는 버림
	{
		if (clusterAvgPoint[i] != 0)
		{
			if (clusterChk_val < clusterAvgPoint[i])
			{
				clusterChk_val = clusterAvgPoint[i];
				clusterChk_cnt++;
			}
			else if (clusterChk_val == clusterAvgPoint[i])
				;
			else
				break;
		}

		if(clusterChk_cnt>=4)
		{
			isPeak = true;
			break;
		 }
	}

	Mat forDebugMat;
	inRange(expectedFrame, 1, 255, forDebugMat);	// maxValue/6 *1 ~ *2
	
	return isPeak;
}

bool LineInfoFinder::isPeakFrame2(Mat expectedFrame)
{
	bool isPeak = false;
	/*	1. maximumStat_mat의 최대값을 구함.
		2. 최대값을 기준으로 6개로 클러스터링
		3. 클러스터링 데이터들의 x좌표 평균을 구함 (클러스터링0~5까지, 0일수록 큰값)
		4. 조건 만족시 피크로 판단 ( 클러스터링0 > 클러스터링1 & 클러스터링1 > 클러스터링2 & 클러스터링2 & 클러스터링3  )	// 최대값과 가까운것들만 함
	*/
	uint maxValue = imageHandler::getMaximumValue(expectedFrame);
	if (maxValue < 10)
		return false;

	/*
		모든 점들의 값을 sort한 후 백분위로 추출
		1. 모든 점들을 수집하고 소팅
		2. 최대값 / 클러스터링값에 해당하는 아이템의 frame 단위로 cluster이미지 추출
	*/
	vector<int>	items = imageHandler::getValueArrWithSort(expectedFrame); // .size() == 총 점의 개수, [i].value == 해당점의 weight
	
	uint clusterValue = items.size() / 10;
	Mat clurster0;
	Mat clurster1;
	Mat clurster2;
	Mat clurster3;
	Mat clurster4;
	Mat clurster5;
	Mat clurster6;
	Mat clurster7;
	Mat clurster8;
	Mat clurster9;
	

	inRange(expectedFrame, items[clusterValue * 9]+1, maxValue, clurster0);	// maxValue/6 *5 ~ *6	
	inRange(expectedFrame, items[clusterValue * 8]+1, items[clusterValue*9], clurster1);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, items[clusterValue * 7]+1, items[clusterValue*8], clurster2);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, items[clusterValue * 6]+1, items[clusterValue*7], clurster3);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, items[clusterValue * 5]+1, items[clusterValue*6], clurster4);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, items[clusterValue * 4]+1, items[clusterValue*5], clurster5);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, items[clusterValue * 3]+1, items[clusterValue*4], clurster6);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, items[clusterValue * 2]+1, items[clusterValue*3], clurster7);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, items[clusterValue * 1]+1, items[clusterValue*2], clurster8);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, 1, items[clusterValue], clurster9);	// maxValue/6 *0 ~ *1	

	Mat clurster0_DR;
	Mat clurster1_DR;
	Mat clurster2_DR;
	Mat clurster3_DR;
	Mat clurster4_DR;
	Mat clurster5_DR;
	Mat clurster6_DR;
	Mat clurster7_DR;
	Mat clurster8_DR;
	Mat clurster9_DR;

	int dotSize = 2;
	clurster0_DR = imageHandler::getDustRemovedImage(clurster0);
	clurster1_DR = imageHandler::getDustRemovedImage(clurster1);
	clurster2_DR = imageHandler::getDustRemovedImage(clurster2);
	clurster3_DR = imageHandler::getDustRemovedImage(clurster3);
	clurster4_DR = imageHandler::getDustRemovedImage(clurster4);
	clurster5_DR = imageHandler::getDustRemovedImage(clurster5);
	clurster6_DR = imageHandler::getDustRemovedImage(clurster6);
	clurster7_DR = imageHandler::getDustRemovedImage(clurster7);
	clurster8_DR = imageHandler::getDustRemovedImage(clurster8);
	clurster9_DR = imageHandler::getDustRemovedImage(clurster9);

	int clusterAvgPoint_0 = imageHandler::getWhitePixelAvgCoordinate(clurster0_DR, true);
	int clusterAvgPoint_1 = imageHandler::getWhitePixelAvgCoordinate(clurster1_DR, true);
	int clusterAvgPoint_2 = imageHandler::getWhitePixelAvgCoordinate(clurster2_DR, true);
	int clusterAvgPoint_3 = imageHandler::getWhitePixelAvgCoordinate(clurster3_DR, true);
	int clusterAvgPoint_4 = imageHandler::getWhitePixelAvgCoordinate(clurster4_DR, true);
	int clusterAvgPoint_5 = imageHandler::getWhitePixelAvgCoordinate(clurster5_DR, true);
	int clusterAvgPoint_6 = imageHandler::getWhitePixelAvgCoordinate(clurster6_DR, true);
	int clusterAvgPoint_7 = imageHandler::getWhitePixelAvgCoordinate(clurster7_DR, true);
	int clusterAvgPoint_8 = imageHandler::getWhitePixelAvgCoordinate(clurster8_DR, true);
	int clusterAvgPoint_9 = imageHandler::getWhitePixelAvgCoordinate(clurster9_DR, true);
	vector<int> clusterAvgPoint;
	clusterAvgPoint.push_back(clusterAvgPoint_0);
	clusterAvgPoint.push_back(clusterAvgPoint_1);
	clusterAvgPoint.push_back(clusterAvgPoint_2);
	clusterAvgPoint.push_back(clusterAvgPoint_3);
	clusterAvgPoint.push_back(clusterAvgPoint_4);
	clusterAvgPoint.push_back(clusterAvgPoint_5);
	clusterAvgPoint.push_back(clusterAvgPoint_6);
	clusterAvgPoint.push_back(clusterAvgPoint_7);
	clusterAvgPoint.push_back(clusterAvgPoint_8);
	clusterAvgPoint.push_back(clusterAvgPoint_9);

	/*
		- 컨투어 구하고 사이즈 1~2짜리 삭제함
		- 남은점이 있는 클러스터들 끼리 만 진행
		- 마지막 2~3개의 클러스터는 버려야함
	*/
	int clusterChk_cnt = 0;
	int clusterChk_val = 0;
	for (int i = 0; i < clusterAvgPoint.size() - 2; i++)	// 마지막 2개 클러스터는 버림
	{
		if (clusterAvgPoint[i] != 0)
		{
			if (clusterChk_val < clusterAvgPoint[i])
			{
				clusterChk_val = clusterAvgPoint[i];
				clusterChk_cnt++;
			}
			else if (clusterChk_val == clusterAvgPoint[i])
				;
			else
				break;
		}

		if (clusterChk_cnt >= 4)
		{
			isPeak = true;
			break;
		}
	}

	Mat forDebugMat;
	inRange(expectedFrame, 1, 255, forDebugMat);	// maxValue/6 *1 ~ *2

	return isPeak;
}


bool LineInfoFinder::isLineFrame(Mat expectedFrame)
{
	bool isPeak = false;
	/*	isPeakFrame 역검사
	*/
	uint maxValue = imageHandler::getMaximumValue(expectedFrame);
	if (maxValue < 10)
		return false;

	uint clusterValue = maxValue / 10;
	Mat clurster0;
	Mat clurster1;
	Mat clurster2;
	Mat clurster3;
	Mat clurster4;
	Mat clurster5;
	Mat clurster6;
	Mat clurster7;
	Mat clurster8;
	Mat clurster9;

	inRange(expectedFrame, clusterValue * 9 + 1, maxValue, clurster0);	// maxValue/6 *5 ~ *6	
	inRange(expectedFrame, clusterValue * 8 + 1, clusterValue * 9, clurster1);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, clusterValue * 7 + 1, clusterValue * 8, clurster2);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, clusterValue * 6 + 1, clusterValue * 7, clurster3);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, clusterValue * 5 + 1, clusterValue * 6, clurster4);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, clusterValue * 4 + 1, clusterValue * 5, clurster5);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, clusterValue * 3 + 1, clusterValue * 4, clurster6);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, clusterValue * 2 + 1, clusterValue * 3, clurster7);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, clusterValue * 1 + 1, clusterValue * 2, clurster8);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, clusterValue * 1 + 1, clusterValue * 2, clurster8);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, 1, clusterValue, clurster9);	// maxValue/6 *0 ~ *1	

	Mat clurster0_DR;
	Mat clurster1_DR;
	Mat clurster2_DR;
	Mat clurster3_DR;
	Mat clurster4_DR;
	Mat clurster5_DR;
	Mat clurster6_DR;
	Mat clurster7_DR;
	Mat clurster8_DR;
	Mat clurster9_DR;

	int dotSize = 2;
	clurster0_DR = imageHandler::getDustRemovedImage(clurster0);
	clurster1_DR = imageHandler::getDustRemovedImage(clurster1);
	clurster2_DR = imageHandler::getDustRemovedImage(clurster2);
	clurster3_DR = imageHandler::getDustRemovedImage(clurster3);
	clurster4_DR = imageHandler::getDustRemovedImage(clurster4);
	clurster5_DR = imageHandler::getDustRemovedImage(clurster5);
	clurster6_DR = imageHandler::getDustRemovedImage(clurster6);
	clurster7_DR = imageHandler::getDustRemovedImage(clurster7);
	clurster8_DR = imageHandler::getDustRemovedImage(clurster8);
	clurster9_DR = imageHandler::getDustRemovedImage(clurster9);

	int clusterAvgPoint_0 = imageHandler::getWhitePixelAvgCoordinate(clurster0_DR, true);
	int clusterAvgPoint_1 = imageHandler::getWhitePixelAvgCoordinate(clurster1_DR, true);
	int clusterAvgPoint_2 = imageHandler::getWhitePixelAvgCoordinate(clurster2_DR, true);
	int clusterAvgPoint_3 = imageHandler::getWhitePixelAvgCoordinate(clurster3_DR, true);
	int clusterAvgPoint_4 = imageHandler::getWhitePixelAvgCoordinate(clurster4_DR, true);
	int clusterAvgPoint_5 = imageHandler::getWhitePixelAvgCoordinate(clurster5_DR, true);
	int clusterAvgPoint_6 = imageHandler::getWhitePixelAvgCoordinate(clurster6_DR, true);
	int clusterAvgPoint_7 = imageHandler::getWhitePixelAvgCoordinate(clurster7_DR, true);
	int clusterAvgPoint_8 = imageHandler::getWhitePixelAvgCoordinate(clurster8_DR, true);
	int clusterAvgPoint_9 = imageHandler::getWhitePixelAvgCoordinate(clurster9_DR, true);
	vector<int> clusterAvgPoint;
	clusterAvgPoint.push_back(clusterAvgPoint_0);
	clusterAvgPoint.push_back(clusterAvgPoint_1);
	clusterAvgPoint.push_back(clusterAvgPoint_2);
	clusterAvgPoint.push_back(clusterAvgPoint_3);
	clusterAvgPoint.push_back(clusterAvgPoint_4);
	clusterAvgPoint.push_back(clusterAvgPoint_5);
	clusterAvgPoint.push_back(clusterAvgPoint_6);
	clusterAvgPoint.push_back(clusterAvgPoint_7);
	clusterAvgPoint.push_back(clusterAvgPoint_8);
	clusterAvgPoint.push_back(clusterAvgPoint_9);

	/*
		- 처리관련 알고리즘 구현할 것..!
	*/
	/*int clusterChk_cnt = 0;
	int clusterChk_val = 0;
	for (int i = 0; i < clusterAvgPoint.size() - 2; i++)	// 마지막 2개 클러스터는 버림
	{
		if (clusterAvgPoint[i] != 0)
		{
			if (clusterChk_val < clusterAvgPoint[i])
			{
				clusterChk_val = clusterAvgPoint[i];
				clusterChk_cnt++;
			}
			else if (clusterChk_val == clusterAvgPoint[i])
				;
			else
				break;
		}

		if (clusterChk_cnt >= 4)
		{
			isPeak = true;
			break;
		}
	}

	Mat forDebugMat;
	inRange(expectedFrame, 1, 255, forDebugMat);	// maxValue/6 *1 ~ *2

	*/
	return isPeak;
}


// peak를 받아서 라인으로 필터링하고 만들어냄
vector<Line> LineInfoFinder::peakToLine(vector<PeakInfo> peaks)
{
	/*
		1. 피크별 반복연산
		2. 피크의 값부터 - (max + a)값 까지 역으로 이미지 연산
		3. 흰색누적.
	*/
	vector<Line> lines;


	for (int i = 0; i < peaks.size(); i++)
	{
		// 마스크 얻음
		// 이미지 마스킹하고 FC이미지 변환 후 
		// 흰색 누적 
		Mat orgImage;
		Mat subImage;
		Mat maskImage;
		Mat stackBinImages;
		inRange(peaks[i].PeakImage, 1, 255, maskImage);

		Mat  maximumStat_mat;			// 
		uint maximumStat_WeightSumValue = 0;		// 
		int  maximumStat_frameNum = 0;		// 

		int curFrame = peaks[i].frameNum;
		

		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
		while (videoCapture->read(orgImage))
		{
			subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
			Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
			Mat maskedImage = imageHandler::getMaskedImage(fullyContrastImage, maskImage);	// FC이미지에서 마스크 씌운 것
			Mat whiteFiterImage = getFillImage(maskedImage, Scalar(255, 255, 255));// whiteFiterImage
			cvtColor(whiteFiterImage, whiteFiterImage, COLOR_BGR2GRAY);

			if (stackBinImages.empty())
			{	// get dummy
				stackBinImages = Mat::zeros(whiteFiterImage.rows, whiteFiterImage.cols, CV_8U);
			}
			else
			{
				stackBinImages = imageHandler::stackBinImage(stackBinImages, whiteFiterImage);
				if (peaks[i].frameNum - peaks[i].maxWeightPixel > curFrame)
				{
					int maxValue = imageHandler::getMaximumValue(stackBinImages);

					int startFrame = peaks[i].frameNum - imageHandler::getMaximumValue(peaks[i].PeakImage);
					int endFrame = startFrame + maxValue;	// 750 + (115)maxValue = 865
					Line line;
					line.startFrame= startFrame;	// 잘 맞음
					line.endFrame = endFrame;		// @잘 안맞음					
					line.maskImage = maskedImage.clone();	// @이미지 클리어링 진행할 것
					lines.push_back(line);
					printf("%d	-	%d	\r\n", startFrame, endFrame);			
					// whiteFiterImage bin으로 저장
					break;
				}
			}

			curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
			videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame-1-1);	// 이전프레임 탐색
		}

		/* // 확인용  무언가 필요하면 작성할것 (예] 가짜 라인 제거 등등)
		//bool isCheck = false;
		//isCheck = isLineFrame(stackBinImages);	// 확인용 

		// 만약 라인이 맞다면 stackBinImage를 
		if (isCheck)
		{
			LineInfo lineInfo;
			inRange(stackBinImages, 1, 255, lineInfo.binaryImage);
			lineInfo.startFrame = peaks[i].frameNum - imageHandler::getMaximumValue(lineInfo.binaryImage);	// 임시 : 지만 대충 맞을 것.. (fade-out 빼고,)
																											// Fade-out 은 어떻게? : 보류
			lineInfo.endFrame= peaks[i].frameNum;															// 임시 : how to impliment => 흰점 개수가 무너지는순간?
			lines.push_back(lineInfo);
		}
		else
		{
			printf("isNot Line : %d \r\n", peaks[i].frameNum);
		}
		*/
	}

	return lines;
}


MVInformation LineInfoFinder::getMVInformation()
{
	return this->m_mvInformation;
}

LineInfoFinder::LineInfoFinder(VideoCapture* videoCapture)
{
	this->videoCapture = videoCapture;

	vecPrintTypes.push_back(Scalar(255, 0, 0));	// Blue
	vecPrintTypes.push_back(Scalar(0, 0, 255));	// Red
	vecPrintTypes.push_back(Scalar(255, 0, 255));	// Purple

	vecUnPrintTypes.push_back(Scalar(255, 255, 255));	// white
	vecUnPrintTypes.push_back(Scalar(0, 255, 255));		// yellow(Orange)

	//vecPrintTypes_PatternPixels.push_back(vector<int>);
	for (int i = 0; i < vecPrintTypes.size(); i++)
	{
		vector<int> dummy;
		vecPrintTypes_PatternPixelCount.push_back(dummy);
	}

}

void LineInfoFinder::WriteLineInfo_toLog(vector<Line> lines)
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Lines";
	for (int i = 0; i < lines.size(); i++)
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line : " << lines[i].startFrame << " - " << lines[i].endFrame;
	}
}

Mat LineInfoFinder::getPatternFillImage(Mat rgbImage, Scalar targetColor)
{
	Mat PatternBin = imageHandler::getPaintedPattern(rgbImage, targetColor, true);	// 내부에서 dilate함
	Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);

	Mat FC_Bin = getFillImage(rgbImage, targetColor);
	Mat PatternFullfill;
	PatternFullfill = imageHandler::getFloodfillImage(FC_Bin, PatternBin);	// FullCont 이미지에 패턴으로 인식한 좌표로 패인트통연산
	PatternFullfill = imageHandler::getBorderFloodFilledImage(PatternFullfill);
	//Mat erodeImage = imageHandler::getMorphImage(PatternFullfill, MORPH_ERODE);	// 침식연산
	//Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(erodeImage);	// 사각박스있는곳 제거
	Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(PatternFullfill);	// 사각박스있는곳 제거

	return erodeImage_Denoise;
}

Mat LineInfoFinder::getFillImage(Mat rgbImage, Scalar targetColor)
{
	Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);

	Mat FC_Bin;
	Scalar patternMin = targetColor;
	for (int i = 0; i < 3; i++)
		if (patternMin[i] != 0)
			patternMin[i] = patternMin[i] - 1;

	inRange(fullyContrastImage, patternMin, targetColor, FC_Bin);	// 파랑만이미지
	cvtColor(FC_Bin, FC_Bin, COLOR_GRAY2BGR);

	return FC_Bin;
}


/// <summary>
/// 각각 프래임별 흰 점의 갯수를 이용하여 피크를 구함
/// 피크는 흰 점의 갯수가 높은 곳을 말하며 가사의 패인팅이 다 된 곳을 가르킴
/// </summary>
/// <param name="vecWhitePixelCounts">프래임 별 흰 점의 테이터.</param>
/// <returns>피크들</returns>
vector<int> LineInfoFinder::getPeak(vector<int> vecPixelCounts)
{
	vector<int> peakValues;

	bool isFadeOut = false;
	int fadeOutCount = 0;
	int maxValueFrame = 0;
	bool zeroZone = true;
	int noneZeroZonCount = 0;
	for (int frameNum = 1; frameNum < vecPixelCounts.size(); frameNum++)
	{
		bool isZeroZone;
		if (vecPixelCounts[frameNum] < 100)		// 100 이하면 ZeroZone 으로
		{
			isZeroZone = true;
		}
		else if (vecPixelCounts[frameNum - 1] > 1000)	// 100 이상이면서, 이전값이 1000이상임 
		{
			if ((vecPixelCounts[frameNum - 1] / 10) * 5 > vecPixelCounts[frameNum])	// 50%이상 급락시 Zero Zone으로 판단
				isZeroZone = true;
			else
				isZeroZone = false;
		}
		else
			isZeroZone = false;


		if (zeroZone == true)
		{
			if (isZeroZone == false)	// zero -> noneZero 
			{
				noneZeroZonCount = 0;
				zeroZone = false;
				maxValueFrame = 0;
			}
		}
		else // noneZeroZone
		{
			noneZeroZonCount++;

			if (vecPixelCounts[maxValueFrame] < vecPixelCounts[frameNum])
				maxValueFrame = frameNum;

			if (vecPixelCounts[frameNum] < vecPixelCounts[frameNum - 1])	// 이전값이 큰경우가 연속일 경우...
				fadeOutCount++;
			else
				fadeOutCount = 0;

			if (isZeroZone == true)	// noneZero -> Zero,	
			{
				zeroZone = true;

				if (noneZeroZonCount < 5)
					continue;

				peakValues.push_back(maxValueFrame);
				if (vecPixelCounts[maxValueFrame] / 2 > vecPixelCounts[frameNum - 1])	// 전프레임의 흰점수 < 최대값/2 인경우 맥스값 넣음
					peakValues.push_back(maxValueFrame);
				else
				{
					peakValues.push_back(frameNum - 1);
				}
			}
		}
	}
	
	return peakValues;
}

Mat LineInfoFinder::stackBinImage(Mat addImage, Mat stackImage)
{
	return Mat();
}

