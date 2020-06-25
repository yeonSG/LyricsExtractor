#include "LineInfoFinder.h"
#include "loger.h"
#include "PeakFinder.h"
#include "UnprintImage.h"

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
	int  maximumStat_colorPixelSum = 0;
	int  maximumStat_frameNum = 0;		// 

	uint expectedWeight = 0;		// 

	int curFrame = 1000;	// movie1.mp4
	//int curFrame = 3810;

	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
	while (videoCapture->read(orgImage))
	{
//		 if (curFrame > 100)
//		 	return linePeaks; //return linePeaks;	// for test
		/* 라인 분석 - get ExpectationLine */
		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		//Mat patternFill = getPatternFillImage(subImage, vecPrintTypes[PrintTypeNum]);
		Mat patternFill = getPatternFillImage_2(subImage, vecPrintTypes[PrintTypeNum]);
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
					maximumStat_WeightSumValue = weightSumAtBinImage;
//					printf(" <Update_WeightSum> ");
					if (maximumStat_colorPixelSum <= colorPixelSumAtBinImage)
					{
						maximumStat_colorPixelSum = colorPixelSumAtBinImage;
						maximumStat_mat = stackBinImages.clone();	// 검은점이 아닌 점 수, 가장오래된점값(offset)
						maximumStat_frameNum = curFrame;
//						printf(" <Update_maxMat> ");
					}
				}
				else if (maximumStat_WeightSumValue / 2 > weightSumAtBinImage)	// 최대값 대비 절반 이하가 되버렸음 => 라인인지 검사
				{
					// 이미지 처리 : max이미지에서 현재 살아있는 점 삭제
					Mat patternFill_flep;
					bitwise_not(patternFill, patternFill_flep);
					Mat maximumStat_mat_noiserm= imageHandler::getMaskedImage_binImage(maximumStat_mat, patternFill_flep);
					
					bool isPeak = false;
					// isPeak = isPeakFrame2(maximumStat_mat);	// patternFill에서 흰색인부분은 삭제
					isPeak = isPeakFrame2(maximumStat_mat_noiserm);	// patternFill에서 흰색인부분은 삭제
					printf("@");
					// 피크 확정 루틴 추가 ()

					if (isPeak == true)	//
					{
						PeakInfo peakInfo;
						peakInfo.frameNum = maximumStat_frameNum;
						peakInfo.maxWeightPixel = imageHandler::getMaximumValue(maximumStat_mat);
						peakInfo.PeakImage = maximumStat_mat.clone();
						stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_8U);

						linePeaks.push_back(peakInfo);
						printf("$(Save.. %d)", maximumStat_frameNum);
						if (linePeaks.size() >= 10)
							return linePeaks; //return linePeaks;	// for test

					}

					{	// 결과에 상관없이 다크모드 진입
						maximumStat_mat = NULL;
						maximumStat_WeightSumValue = 0;
						maximumStat_colorPixelSum = 0;
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
				// 5연속 증가시 다크모드 해제
				if (weightSumAtBinImage >= DarkMode_BeforeWeightSum &&	// 이전 웨이트 합보다 큼
					weightSumAtBinImage >0)// &&							// weight 합이 0 이상
//					colorPixelSumAtBinImage >= DarkMode_BeforePixelSum)	// 이전 픽셀총합보다 큼 -> 노이즈에 약함
				{
					darkMode_increaseCount++;
					if (darkMode_increaseCount >= 5)
					{
						darkMode = false;
						maximumStat_mat = stackBinImages.clone();	// 검은점이 아닌 점 수, 가장오래된점값(offset)
						maximumStat_WeightSumValue = weightSumAtBinImage;
						maximumStat_colorPixelSum = colorPixelSumAtBinImage;
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
		printf("(%d) ", weightSumAtBinImage - expectedWeight);
		printf(" DarkMode= %d	\r\n", darkMode_increaseCount);
		expectedWeight = weightSumAtBinImage + colorPixelSumAtBinImage;
	}

	return linePeaks;
}

/*
	클래스 A를 생성하고 A에게 연속된 이미지만 주면
	내부에서 처리하고 데이터도 갖고 있도록 설계?
	class PeakFinder
	{

		void frame_process(Mat image);	// 이것만 실행

	}
*/

vector<PeakInfo> LineInfoFinder::start2_useContour(int PrintTypeNum)
{
	vector<PeakInfo> linePeaks;

	Mat orgImage;
	Mat subImage;

	Mat stackBinImages;
	
	Mat  maximumStat_mat;			// 
	uint maximumStat_WeightSumValue = 0;		// 
	int  maximumStat_colorPixelSum = 0;
	int  maximumStat_frameNum = 0;		// 

	uint expectedWeight = 0;		// 

	vector<pair< contourLineInfo, int>> expectedLineInfos;	// pair<라인정보, lineCount>		// 라인카운트는 못찾을 수록 증가시킴

#ifndef _DEBUG
	int curFrame = 0;
#else
	int curFrame = 0;	// movie1.mp4
#endif

	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
	while (videoCapture->read(orgImage))
	{
		//		 if (curFrame > 100)
		//		 	return linePeaks; //return linePeaks;	// for test
				/* 라인 분석 - get ExpectationLine */
		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		//Mat patternFill = getPatternFillImage(subImage, vecPrintTypes[PrintTypeNum]);
		Mat patternFill = getPatternFillImage_2(subImage, vecPrintTypes[PrintTypeNum]);
		uint weightSumAtBinImage = 0;
		int colorPixelSumAtBinImage = 0;

		if (stackBinImages.empty())
		{	// get dummy
			//stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_16U);
			stackBinImages = Mat::zeros(patternFill.rows, patternFill.cols, CV_8U);
		}
		else
		{
			vector<contourLineInfo> expectedLineInfos_curFrame;	
			stackBinImages = imageHandler::stackBinImage(stackBinImages, patternFill);
			//stackBinImages = imageHandler::stackBinImage(stackBinImages, patternFill);	// for test
			stackBinImages = imageHandler::getMaxColorContoursImage(stackBinImages, expectedLineInfos_curFrame);
			printf("[found CLine %d]", expectedLineInfos_curFrame.size());
			weightSumAtBinImage = imageHandler::getSumOfBinImageValues(stackBinImages);
			colorPixelSumAtBinImage = imageHandler::getWhitePixelCount(stackBinImages);

			imshow("subImage", subImage);
			imshow("stackBinImages", stackBinImages);
			Mat binImage;
			inRange(stackBinImages, 1, 255, binImage);
			imshow("binImage", binImage);

			for (int i = 0; i < expectedLineInfos.size(); i++)	// 1.
			{
				bool isFind = false;
				for (int j = 0; j < expectedLineInfos_curFrame.size(); j++)
				{
					bool isRelative = imageHandler::isRelation(expectedLineInfos[i].first.coorY_start, expectedLineInfos[i].first.coorY_end, expectedLineInfos_curFrame[j].coorY_start, expectedLineInfos_curFrame[j].coorY_end);
					if (isRelative)	// 이번프레임에도 존재함
					{
						isFind = true;	
						if (expectedLineInfos[i].first.pixelCount / 2 > expectedLineInfos_curFrame[j].pixelCount)
						{	//
							expectedLineInfos[i].second++;	// 존재하지만 픽셀수가 절반이하로 줄어듦
						}
						else
						{
							if(expectedLineInfos[i].second==0)	// 연속으로 0일때만 정보 업데이트
								expectedLineInfos[i].first = expectedLineInfos_curFrame[j];
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

			for (int i = 0; i < expectedLineInfos_curFrame.size(); i++)	// 3. 
			{
				bool isFind = false;
				for (int j = 0; j < expectedLineInfos.size(); j++)
				{
					bool isRelative = imageHandler::isRelation(expectedLineInfos_curFrame[i].coorY_start, expectedLineInfos_curFrame[i].coorY_end, expectedLineInfos[j].first.coorY_start, expectedLineInfos[j].first.coorY_end);
					if (isRelative)	// 
					{
						isFind = true;
						break;
					}
				}
				if (isFind != true)	// 못찾았을 경우 
				{
					expectedLineInfos.push_back(make_pair(expectedLineInfos_curFrame[i], 0));
					printf(" [Line Add] ");
					; // 라인 추가 : addLine()
				}
			}

			vector<pair< contourLineInfo, int>> expectedLineInfos_temp;
			for (int i = 0; i < expectedLineInfos.size(); i++)	// 4. 
			{
				if (expectedLineInfos[i].second >= 5)	// count가 5이상이면 라인으로 처리
				{
					printf("* Line found [coor_y(%d ~ %d)] ", expectedLineInfos[i].first.coorY_start, expectedLineInfos[i].first.coorY_end);		
					int maxValue = 0;
					for (int j = 0; j < expectedLineInfos[i].first.contours.size(); j++)
					{
						int val = expectedLineInfos[i].first.contours[j].getMaxValue();
						if (val > maxValue)
							maxValue = val;
					}
					printf(" [Line start Frame : %d] \r\n", curFrame - (maxValue + expectedLineInfos[i].second));
					
					; // expectedLineInfos에서 삭제					
				}
				else
				{
					expectedLineInfos_temp.push_back(expectedLineInfos[i]);
				}
			}
			expectedLineInfos = expectedLineInfos_temp;
			// expectedLineInfos_withCount  <- 기존에 있던거

			// >> frame 흐름에 있어서 라인으로 판별
			// 1. 받은라인정보가 기존 라인에 있다면 업데이트 and 없어진 라인 count++
			// 2. 업데이트 된 라인 중 중복된 라인이 있다면 삭제  (두개의 라인이 진행되다 하나로 합쳐지기도 함.)
			// 3. 신규 라인 찾아 추가 
			// 4. count가 5 이상인것들만 저장해둠 (이하인 것 라인으로 보지않음)

			//printf(" ( %d : %d ) ", expectedLineInfos_buffer_toLine.size(), expectedLineInfos_buffer_toMaintain.size());
		}

		printf("frame... %d : %d	%d ", curFrame, weightSumAtBinImage, colorPixelSumAtBinImage);
		printf("(%d) \r\n", weightSumAtBinImage - expectedWeight);
		expectedWeight = weightSumAtBinImage + colorPixelSumAtBinImage;

		{	// for test
			int key = waitKey(0);
			if (key == KEY_ESC)
				break;
			else if (key == 'a')	// before frame
				curFrame -= 1;
			else if (key == 'd')	// next frame
				curFrame += 1;
			else if (key == 'w')	// +50th frame
				curFrame += 50;
			else if (key == 's')	// -50th frame
				curFrame -= 50;
			else if (key == 'r')	// +10th frame
				curFrame += 10;
			else if (key == 'f')	// -10th frame
				curFrame -= 10;
			else if (key == 'e')	// +500th frame
				curFrame += 500;
			else if (key == 'q')	// -500th frame
				curFrame -= 500;
			else if (key == '?')
				videoHandler::printVideoSpec();

			videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);
		}
	}

	return linePeaks;
}

vector<LineInfo> LineInfoFinder::start2_useContour2(int PrintTypeNum, Scalar UnprintColor)
{
	m_PrintTypeNumType = PrintTypeNum;
	vector<LineInfo> lineInfos;

	Mat orgImage;
	Mat subImage;

#ifndef _DEBUG
	int curFrame = 0;
#else
	int curFrame = 2500;	// debug	 // YSYSYS
	
#endif

	PeakFinder peakFinder;
	UnprintImage unprintImage;
	unprintImage.m_isFindUnprintColor = true;
	unprintImage.m_unPrintColor = UnprintColor;	
	
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
	while (videoCapture->read(orgImage))
	{
		cout << endl;
		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		if (videoHandler::getVideoFrameCount() <= curFrame)
			break;

		printf("frame... %d ", curFrame);

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		Mat refUnprintImage = unprintImage.unprintImage_process(subImage);

		vector<contourLineInfoSet> line_PeakInfo;
		line_PeakInfo = peakFinder.frameImage_process(subImage, curFrame, vecPrintTypes[m_PrintTypeNumType], refUnprintImage);	// ys-process : 벹어낸 라인의 총 수
		// 이미지 분석하여 라인의 조건에 만족하는 컨투어 셋을 반환함.
		vector<contourLineInfo>	linePeakInfo_max;
		for (int i = 0; i < line_PeakInfo.size(); i++)
			linePeakInfo_max.push_back(line_PeakInfo[i].maximum);
		
		if (linePeakInfo_max.size() > 0)
		{
			vector<contourLineInfo> filltered_line_PeakInfo; 			
			filltered_line_PeakInfo = line_PeakInfoFilter(linePeakInfo_max, lineInfos);	// ys-process : 기본적인 정보로 라인걸러냄

#ifdef _DEBUG
			if (curFrame == 3993)
				filltered_line_PeakInfo = filltered_line_PeakInfo;
#endif			
			if (filltered_line_PeakInfo.size() != 0)
			{
				// merge peakInfo
				vector<contourLineInfo> sep_filltered_line_PeakInfo = separateLineIfTwinline(filltered_line_PeakInfo);	// separateLine if twinLine
				// line_PeakInfoFilter()에 사용되는 요소는 업데이트해줘야함
				filltered_line_PeakInfo = line_PeakInfoFilter(sep_filltered_line_PeakInfo, lineInfos);	// ys-process : 기본적인 정보로 라인걸러냄
			}
			cout << endl;
			for (int i = 0; i < filltered_line_PeakInfo.size(); i++)
			{
				int maxValue = filltered_line_PeakInfo[i].maxValue;
				printf("* Line found [Coor_y(%d ~ %d)] ", filltered_line_PeakInfo[i].coorY_start, filltered_line_PeakInfo[i].coorY_end);
				printf(" [Line start Frame : %d] ", curFrame - (maxValue + PeakFinder::JUDGE_TIMEOUT));

				{
					LineFinder lineFinder(videoCapture);
					LineInfo lineInfo;
					lineInfo.printColor = m_PrintTypeNumType;
					lineInfo = lineFinder.getLine(filltered_line_PeakInfo[i].weightMat, UnprintColor);		// 

					/* 라인 후보 저장 */
					/*	// bool getLine(WeightMat, vc)
						1. weight 이미지(a)의 시작점에서 흰색 필터 이미지(b)를 땀
						2. bitwise_and(a, b) 수행
						3. 결과를 마스크로 사용
						4. 정방향 재생을 통하여 타겟컬러 흰색이 없어지는 프래임 흭득 (endFrame)
						2. 검사
					*/
					if (lineInfo.isValid == true)
					{
						lineInfos.push_back(lineInfo);
						peakFinder.stackBinImageCorrect(filltered_line_PeakInfo[i]);

#ifdef _DEBUG
						//if (lineInfos.size() <= 3)	// ysysys
						//	return lineInfos;
#endif				
					}
					else
					{
						lineInfos.push_back(lineInfo);	// error인 상태로 들어감
					}
				}
			}			
		}

#if(0 && _DEBUG)
		//cout << endl;
		//imshow("stackBinImage", peakFinder.m_stackBinImage);
		//Mat debugImg;
		//inRange(peakFinder.m_stackBinImage, 1, 255, debugImg);
		//imshow("debugImg", debugImg);
		//imshow("subImage", subImage);

		{	// for test
			int key = waitKey(0);
			if (key == KEY_ESC)
				break;
			else if (key == 'a')	// before frame
				curFrame -= 1;
			else if (key == 'd')	// next frame
				curFrame += 1;
			else if (key == 'w')	// +50th frame
				curFrame += 50;
			else if (key == 's')	// -50th frame
				curFrame -= 50;
			else if (key == 'r')	// +10th frame
				curFrame += 10;
			else if (key == 'f')	// -10th frame
				curFrame -= 10;
			else if (key == 'e')	// +500th frame
				curFrame += 500;
			else if (key == 'q')	// -500th frame
				curFrame -= 500;
			else if (key == '?')
				videoHandler::printVideoSpec();

			videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);
		}
#else
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
#endif
	}

	return lineInfos;
}
//
//Scalar LineInfoFinder::findUnprintColor(int PrintTypeNum)
//{
//
//	Mat orgImage;
//	Mat subImage;
//	
//#ifndef _DEBUG
//	int curFrame = 1000;
//	if(videoHandler::getVideoFrameCount() < 1000)
//		curFrame = 0;
//#else
//	int curFrame = 1000;	// movie1.mp4
//#endif
//
//	vector<contourLineInfoSet> line_PeakInfo;
//	PeakFinder peakFinder;
//	UnprintImage unprintImage;
//
//	videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
//	while (videoCapture->read(orgImage))
//	{
//		cout << endl;
//		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
//		printf("frame... %d ", curFrame);
//
//		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
//
//		Mat refUnprintImage = unprintImage.unprintImage_process(subImage);
//
//		line_PeakInfo = peakFinder.frameImage_process(subImage, curFrame, vecPrintTypes[PrintTypeNum], refUnprintImage);
//
//		if (line_PeakInfo.size() > 0)
//		{
//			for (int i = 0; i < line_PeakInfo.size(); i++)
//			{
//				int maxValue = line_PeakInfo[i].maximum.maxValue;
//				printf("* Line found [Coor_y(%d ~ %d)] ", line_PeakInfo[i].maximum.coorY_start, line_PeakInfo[i].maximum.coorY_end);
//				printf(" [Line start Frame : %d] ", curFrame - (maxValue + PeakFinder::JUDGE_TIMEOUT));
//
//				if (line_PeakInfo[i].maximum.maxValue <= 5)	// maxValue(Weight)가 5미만이면 라인으로 안봄
//				{
//					printf(" [IS NOT LINE : maxWeight<5] ");
//					continue;
//				}
//				if (line_PeakInfo[i].maximum.pixelCount <= 500)	// maxValue(Weight)가 5미만이면 라인으로 안봄
//				{
//					printf(" [IS NOT LINE : pixelCount<500] ");
//					continue;
//				}
//				if (line_PeakInfo[i].maximum.coorY_end - line_PeakInfo[i].maximum.coorY_start < 30) // y축 길이가 100이하
//				{
//					printf(" [IS NOT LINE : y_length<10] ");
//					continue;
//				}
//				// 왼쪽에서 오른쪽으로 흐르는 패턴이 아님 ( weight 보기? (모든 픽셀 weight값 정렬 -> 중간값 기준으로 ) )
//
//				cout << endl;
//
//#if(0)
//				Mat dbgImg = line_PeakInfo[i].weightMat.binImage;
//				Mat dbgImg_max = line_PeakInfo[i].weightMat_maximum.binImage;
//				Mat dbgImg_b;
//				Mat dbgImg_max_b;
//				inRange(dbgImg, 1, 255, dbgImg_b);
//				inRange(dbgImg_max, 1, 255, dbgImg_max_b);
//#endif
//				{
//					// Unprint 색 찾는 루틴
//					LineFinder lineFinder(videoCapture);
//					LineInfo lineInfo;
//					lineInfo = lineFinder.getLine(line_PeakInfo[i].maximum.weightMat);
//					if (lineInfo.isValid == true)
//					{
//						unprintImage.m_isFindUnprintColor = true;
//						// 색 찾기
//						// lineInfo.frame_start 의 이미지 가저옴, 마스크 이미지에서 점 수집 -> 가장 많은 점 (255,255,255 또는 0,255,255 일 것)
//						int checkFrame = imageHandler::getWhitePixelAvgValue(line_PeakInfo[i].maximum.weightMat.binImage);// line_PeakInfo[i].maxValue / 2;
//						Mat checkImage;
//						inRange(lineInfo.maskImage_withWeight, 5, checkFrame, checkImage);	// mask
//						cvtColor(checkImage, checkImage, COLOR_GRAY2BGR);
//						//inRange(checkImage, Scalar(1, 1, 1), Scalar(255, 255, 255), checkImage);
//
//						int unPrintFrame = line_PeakInfo[i].maximum.weightMat_Unprint.frameNum - line_PeakInfo[i].maximum.maxValue;
//						videoCapture->set(CAP_PROP_POS_FRAMES, (double)unPrintFrame);
//						videoCapture->read(orgImage);
//						subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
//						Mat fillImage = imageHandler::getPixelContrastImage(subImage);
//						Mat fillImage_erode = imageHandler::getMorphImage(fillImage, MORPH_ERODE);
//
//						bitwise_and(fillImage_erode, checkImage, fillImage_erode);	// mask 처리
//						
//						Scalar unprintColor = unprintImage.getUnprintColor(fillImage_erode);
//						// 
//
//						imwrite(fileManager::getSavePath()+"UnprintImage.jpg", fillImage_erode);
//						unprintImage.m_unPrintColor = unprintColor; // 색 세팅
//						
//						BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Unprint Color : " << (int)unprintImage.m_unPrintColor[0] << " " << (int)unprintImage.m_unPrintColor[1] << " " << (int)unprintImage.m_unPrintColor[2];
//						printf("Unprint Color : %d %d %d", unprintImage.m_unPrintColor[0], unprintImage.m_unPrintColor[1], unprintImage.m_unPrintColor[2]);
//						return unprintColor;
//					}
//				}
//			}
//
//#if(0)
//			cout << endl;
//			imshow("stackBinImage", peakFinder.m_stackBinImage);
//			Mat debugImg;
//			inRange(peakFinder.m_stackBinImage, 1, 255, debugImg);
//			imshow("debugImg", debugImg);
//			imshow("subImage", subImage);
//
//			{	// for test
//				int key = waitKey(0);
//				if (key == KEY_ESC)
//					break;
//				else if (key == 'a')	// before frame
//					curFrame -= 1;
//				else if (key == 'd')	// next frame
//					curFrame += 1;
//				else if (key == 'w')	// +50th frame
//					curFrame += 50;
//				else if (key == 's')	// -50th frame
//					curFrame -= 50;
//				else if (key == 'r')	// +10th frame
//					curFrame += 10;
//				else if (key == 'f')	// -10th frame
//					curFrame -= 10;
//				else if (key == 'e')	// +500th frame
//					curFrame += 500;
//				else if (key == 'q')	// -500th frame
//					curFrame -= 500;
//				else if (key == '?')
//					videoHandler::printVideoSpec();
//
//				videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame - 1);
//			}
//#else
//			videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
//#endif
//		}
//	}
//	return Scalar(255, 255, 255);
//}


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
	
	float clusterValue = items.size() / 10.0;		
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

	Mat forDebugMat_erode;
	forDebugMat_erode = imageHandler::getMorphImage(forDebugMat, MORPH_ERODE);

	Mat masked_expectedFrame;
	masked_expectedFrame = imageHandler::getMaskedImage_binImage(expectedFrame, forDebugMat_erode);

	vector<contourLineInfo> expectedLineInfos_dummy;
	Mat maxImage;	//컨투어에 해당하는 모든 점의 평균으로 색칠된 이미지
	maxImage = imageHandler::getMaxColorContoursImage(masked_expectedFrame, expectedLineInfos_dummy);


	/*for debug*/
	//{
	//	int value = 0;
	//	Mat debugImage;
	//	while (true)
	//	{
	//		cout << "value : " << value << " ~ " << value+1 << endl;
	//		inRange(expectedFrame, value, value, debugImage);
	//		imshow("debugImage", debugImage);

	//		int key = waitKey(0);
	//		if (key == KEY_ESC)
	//			break;
	//		else if (key == 'a')	// before frame
	//			value -= 1;
	//		else if (key == 'd')	// next frame
	//			value += 1;
	//		else if (key == 'q')
	//			break;
	//	}
	//}

	return isPeak;	// isPeakFrame2 End
}


bool LineInfoFinder::isLineFrame(Mat expectedFrame)
{
	bool isLine = false;
	/*	isPeakFrame 역검사
	*/
	uint maxValue = imageHandler::getMaximumValue(expectedFrame);
	if (maxValue < 10)
		return false;

	vector<int>	items = imageHandler::getValueArrWithSort(expectedFrame); // .size() == 총 점의 개수, [i].value == 해당점의 weight

	// 재일 왼쪽 점의 값 이하 프레임 버림
	
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

	inRange(expectedFrame, items[clusterValue * 9] + 1, maxValue, clurster0);	// maxValue/6 *5 ~ *6	
	inRange(expectedFrame, items[clusterValue * 8] + 1, items[clusterValue * 9], clurster1);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, items[clusterValue * 7] + 1, items[clusterValue * 8], clurster2);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, items[clusterValue * 6] + 1, items[clusterValue * 7], clurster3);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, items[clusterValue * 5] + 1, items[clusterValue * 6], clurster4);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, items[clusterValue * 4] + 1, items[clusterValue * 5], clurster5);	// maxValue/6 *4 ~ *5
	inRange(expectedFrame, items[clusterValue * 3] + 1, items[clusterValue * 4], clurster6);	// maxValue/6 *3 ~ *4
	inRange(expectedFrame, items[clusterValue * 2] + 1, items[clusterValue * 3], clurster7);	// maxValue/6 *2 ~ *3
	inRange(expectedFrame, items[clusterValue * 1] + 1, items[clusterValue * 2], clurster8);	// maxValue/6 *1 ~ *2
	inRange(expectedFrame, items[clusterValue * 1] + 1, items[clusterValue * 2], clurster8);	// maxValue/6 *1 ~ *2
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
		- 처리관련 알고리즘 구현할 것..!
	*/
	int clusterChk_cnt = 0;
	int clusterChk_val = 2000;	// Init value
	for (int i = 0; i < clusterAvgPoint.size(); i++)	// 첫 2개 클러스터는 버림
	{
		if (clusterAvgPoint[i] != 0)
		{
			if (clusterChk_val > clusterAvgPoint[i])
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
			isLine = true;
			break;
		}
	}

	Mat forDebugMat;
	inRange(expectedFrame, 1, 255, forDebugMat);	// maxValue/6 *1 ~ *2

	return isLine;	// isLineFrame End
}

/*
	peakFrame(흑백) 을 받아서 Unprint 색을 파악함
	: Max Weight 의 최신값은 버리고 오래된 값만 사용(더 정확성 있다고 판단됨)
*/
bool LineInfoFinder::findUnprintColor(vector<PeakInfo> peaks, Vec3b& findColor)
{
	// while
	vector<Vec3b> unprintColor;
	vector<int> avgContourVolume;
	int avgContourVolumeSum = 0;

	for (int i = 0; i < peaks.size(); i++)
	{
		uint maxValue = peaks[i].maxWeightPixel;

		Mat printedArea;	// use mask
		inRange(peaks[i].PeakImage, maxValue / 2, maxValue, printedArea);

		Mat orgImage, subImage;
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)(peaks[i].frameNum - maxValue));
		videoCapture->read(orgImage);	// 확인바람

		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		//Mat fullyContrastImage_per = imageHandler::getPixelContrastImage_byPercent(subImage);
		Mat fullyContrastImage = imageHandler::getPixelContrastImage(subImage);

		Mat maskedImage = imageHandler::getMaskedImage(fullyContrastImage, printedArea);
		Vec3b mostColor = imageHandler::getMostHaveRGB(maskedImage);

		unprintColor.push_back(mostColor);

		imwrite(fileManager::getSavePath() + "/Captures/peak_" + to_string(i) + ".jpg", maskedImage);
		// 마스크 분석 : 컨투어의 평균 크기를 구함 (x이하는 버림)
		// 마스크 분석결과로 
		// 컨투어 얻음, 부피 파악, 부피로 소팅, 하위 절반 버림
		// 나머지로 평균 얻음

		Mat binMaskedMat;
		cvtColor(maskedImage, binMaskedMat, COLOR_BGR2GRAY);

		avgContourVolume.push_back(imageHandler::getAvgContourVolume(binMaskedMat));
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line AvgContourVoluem[" << i << "] : " << avgContourVolume.back();
	}

	if (avgContourVolume.size() == 0)
		return false;

	int contourVolumeInvalidCount = 0;
	int contourVolumeInvalidLimit = 100;	// 100 이하인 것들은 Invalid로 판별

	for (int i = 0; i < avgContourVolume.size(); i++)
	{
		avgContourVolumeSum += avgContourVolume[i];
		if (avgContourVolume[i] < contourVolumeInvalidLimit)
			contourVolumeInvalidCount++;
	}

	int avg = avgContourVolumeSum / avgContourVolume.size();

	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line AvgContourVoluem_InvalidCount : " << contourVolumeInvalidCount;	// 
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line AvgContourVoluem_All : " << avg;

	findColor = imageHandler::getMostHaveRGB(unprintColor);	

	printf("Unprint Color : { %d %d %d } \r\n", findColor[0], findColor[1], findColor[2]);
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Unprint Color : { " << (int)findColor[0] << " " << (int)findColor[1] << " " << (int)findColor[2] << "}" << endl;

	return true;
}

Mat LineInfoFinder::getUnprintFillteredstackBinImage(Mat weightPaint, Mat weightUnpaint)
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

// peak를 받아서 라인으로 필터링하고 만들어냄
vector<Line> LineInfoFinder::peakToLine(vector<PeakInfo> peaks, Vec3b unprintColor)
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
			//Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
			Mat fullyContrastImage = imageHandler::getPixelContrastImage_byPercent(subImage);
			Mat maskedImage = imageHandler::getMaskedImage(fullyContrastImage, maskImage);	// FC이미지에서 마스크 씌운 것
			//Mat unPrintFiterImage = getFillImage(maskedImage, Scalar(255, 255, 255));// whiteFiterImage
			Mat unPrintFiterImage = getFillImage(maskedImage, unprintColor);// whiteFiterImage
			cvtColor(unPrintFiterImage, unPrintFiterImage, COLOR_BGR2GRAY);

			if (stackBinImages.empty())
			{	// get dummy
				stackBinImages = Mat::zeros(unPrintFiterImage.rows, unPrintFiterImage.cols, CV_8U);
			}
			else
			{
				stackBinImages = imageHandler::stackBinImage(stackBinImages, unPrintFiterImage);
				if (peaks[i].frameNum - peaks[i].maxWeightPixel > curFrame)
				{
					bool isLine = false;
					isLine = isLineFrame(stackBinImages);	// 라인인지 확인 -누적 이미지 전송
					if (isLine == false)
					{
						printf("is Not Line. \r\n");
						BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << peaks[i].frameNum << " is Not LIne." << endl;
						break;
					}
					printf("@");

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

			// 라인 아님에 대한 조건
			if (curFrame < 10)
			{
				break;
			}
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

vector<contourLineInfo> LineInfoFinder::line_PeakInfoFilter(vector<contourLineInfo> lineInfosSet, vector<LineInfo>& errorLineInfos)
{
	vector<contourLineInfo> filltered_line_PeakInfo;

	for (int i = 0; i < lineInfosSet.size(); i++)
	{
		LineInfo errorLineInfo;
		errorLineInfo.frame_end = lineInfosSet[i].weightMat.frameNum;
		errorLineInfo.maskImage = lineInfosSet[i].weightMat;
		errorLineInfo.printColor = m_PrintTypeNumType;

		if (lineInfosSet[i].maxValue <= 5)	// maxValue(Weight)가 5미만이면 라인으로 안봄
		{
			printf(" [IS NOT LINE : maxWeight<5] ");
			errorLineInfo.errorOccured(LINEERROR_PEAKFILLTER_WEIGHT);
			errorLineInfos.push_back(errorLineInfo);
			continue;
		}
		if (lineInfosSet[i].pixelCount <= 300)	// maxValue(Weight)가 5미만이면 라인으로 안봄
		{
			printf(" [IS NOT LINE : pixelCount<300] ");
			errorLineInfo.errorOccured(LINEERROR_PEAKFILLTER_PIXELCOUNT);
			errorLineInfos.push_back(errorLineInfo);
			continue;
		}
		if (lineInfosSet[i].coorY_end - lineInfosSet[i].coorY_start < 10) // y축 길이가 10 이하
		{
			printf(" [IS NOT LINE : y_length<10] ");
			errorLineInfo.errorOccured(LINEERROR_PEAKFILLTER_Y_LENGHTH);
			errorLineInfos.push_back(errorLineInfo);
			continue;
		}
		if (lineInfosSet[i].coorX_end - lineInfosSet[i].coorX_start < 50) // x축 길이가 50 이하
		{
			printf(" [IS NOT LINE : x_length<60] ");
			errorLineInfo.errorOccured(LINEERROR_PEAKFILLTER_X_LENGHTH);
			errorLineInfos.push_back(errorLineInfo);
			continue;
		}
		//int continueCount = getSequentialIncreasedContoursCount(lineInfosSet[i].maximum.contours);
		//if (continueCount < 5)
		//{
		//	printf(" [IS NOT LINE : no have continuity] ");
		//	continue;
		//}

		// print와 unprint 10이상 차이나는 부분 제거
		//Mat unPrintFillteredImage = getUnprintFillteredstackBinImage(lineInfosSet[i].maximum.weightMat.binImage, lineInfosSet[i].maximum.weightMat_Unprint.binImage);
		//Mat unPrintFillteredBinImage;
		//inRange(unPrintFillteredImage, 1, 255, unPrintFillteredBinImage); //  weightMat.binImage 쓰지말도록..
		//bitwise_and(unPrintFillteredBinImage, lineInfosSet[i].maximum.weightMat.binImage, lineInfosSet[i].maximum.weightMat.binImage);
		//bitwise_and(unPrintFillteredBinImage, lineInfosSet[i].weightMat_maximum.binImage, lineInfosSet[i].weightMat_maximum.binImage);	

		// 시작 frame 정보를 이용한 필터  (p이미지의 값이 up이미지의 값보다 크거나 같은 점만 납둠)
//		Mat filteredImage = getWeightCompareImage(lineInfosSet[i].weightMat_Unprint.binImage, lineInfosSet[i].weightMat.binImage);





		filltered_line_PeakInfo.push_back(lineInfosSet[i]);
	}

	return filltered_line_PeakInfo;
}

int LineInfoFinder::getSequentialIncreasedContoursCount(vector<contourInfo> contours)
{
	//for (int i = 0; i < contours.size(); i++)
	//{
	//	printf(" %d ", contours[i].maxValue);
	//}

	int count = 0;
	int bigist_length = 0;

	for(int j = 0; j< contours.size(); j++)
	{
		vector<contourInfo> seqContours;
		for (int i = j; i < contours.size(); i++)
		{
			if (i == j)
				seqContours.push_back(contours[i]);
			else // 
			{
				if (seqContours.back().getMaxValue() > contours[i].getMaxValue())
				{
					seqContours.push_back(contours[i]);
				}
				else
				{
					if (seqContours.size() >= 2
						&& seqContours[seqContours.size() - 1].getMaxValue() > contours[i].getMaxValue())
					{
						seqContours.back() = contours[i];	// 저장된값의 마지막 값보다 크면서 그 이전값보다 작으면 업데이트
					}
				}
			}
		}
		if (bigist_length < seqContours.size())
		{
			bigist_length = seqContours.size();
			//printf("\r\n Length = %d", seqContours.size());
			for (int x = 0; x < seqContours.size(); x++)
			{
				//printf(" %d ", seqContours[x].maxValue);
			}
		}
	}

	return bigist_length;
	/* 연속되는 값 중 값이 더 작아지는 배열 중 가장 많은 수를 갖는 것을 구하는 것.
	ex) arr{81  1  1  81  81  81  80  81  1  1  60  1  80  80  47  78  78  78  2  78  78  1  78  1  78  78  1  78  1  78  1  1  1  78  78  15  18  1  1  1  28  47  78}
	일때 답은 : {81 80 78 15 1} = 5가 되어야 함.
	but 알고리즘 손대야 함..
	*/
}

//// 라인 셋 머지함
//contourLineInfo LineInfoFinder::lineInfomerge(vector<contourLineInfo> lineInfos)
//{
//	if (lineInfos.size() != 0)
//	{
//		contourLineInfo mergedInfo = lineInfos[0];
//
//
//		for (int i = 1; i < lineInfos.size(); i++)
//		{
//		}
//	}
//}

// 라인이 트윈라인인 경우 라인을 나눠줌
vector<contourLineInfo> LineInfoFinder::separateLineIfTwinline(vector<contourLineInfo> lineInfoSet)
{
	vector<contourLineInfo> outLineSet;	// 최종 아웃풋

	vector<contourLineInfo> outLineSetTemp;	// 임시저장소 (모든 lineInfoSet을 separate한 저장소 -> 머지 후 -> 최종 아웃풋에 병합)
	// 가로섬 단위로 이미지 나눔,
	// 겹치는 x좌표의 값 비교하여 차이가 크면 다른 라인으로 판단.
	for (int i = 0; i < lineInfoSet.size(); i++)
	{
		vector<int> projection = imageHandler::getHorizontalProjectionData(lineInfoSet[i].weightMat.binImage);

		vector<pair<int, int>> islands;
		bool blackZone = false;
		int startPoint = 0;
		int endPoint = 0;

		for (int j = 0; j < projection.size(); j++)
		{
			if (projection[j] != 0)	// Not blackZone
			{
				if (blackZone == true)	// blackZone -> Not BlackZone
				{
					startPoint = j;
				}
				blackZone = false;
			}
			else // blackZone
			{
				if (blackZone == false)	// Not blackZone -> blackZone
				{	// save island
					islands.push_back(make_pair(startPoint, j));
				}
				blackZone = true;
			}
		}

		if (islands.size() == 1)	// 섬이 하나임
		{
			outLineSet.push_back(lineInfoSet[i]);
			continue;
		}


		// 섬이 여러개임
		//vector<Mat> separatedMat;	// 섬별로 마스킹 한 Mat 

		for (int j = 0; j < islands.size(); j++)
		{
			Mat mask = Mat::zeros(lineInfoSet[i].weightMat.binImage.rows, lineInfoSet[i].weightMat.binImage.cols, CV_8U);
			printf("j = %d  first = %d  second = %d\r\n", j, islands[j].first, islands[j].second);
			mask = imageHandler::getWhiteMaskImage(mask, 0, islands[j].first, lineInfoSet[i].weightMat.binImage.cols, islands[j].second - islands[j].first);

			bitwise_and(mask, lineInfoSet[i].weightMat.binImage, mask);
			//separatedMat.push_back(mask);

			contourLineInfo infoSet = lineInfoSet[i];
			infoSet.weightMat.binImage = mask;
			outLineSetTemp.push_back(infoSet);
		}
	}	// separate 완료


	// max 값 기준으로 정렬, 
	// (weightMax - frame) ~ frame 이 겹치는 것들 끼리 연산 
	// 1. 겹치는지 확인
	// 2. 겹쳐지는 부분 확인 ( x프레임~x프레임+5 까지 추출 후 x좌표가 겹치는지 확인)
//	{
//		vector<contourLineInfo> mergedSeparatedMat;	// 처리된 Mat
//
//		for (int j = 0; j < outLineSetTemp.size(); j++)
//		{
//			if (j == 0)
//				mergedSeparatedMat.push_back(outLineSetTemp[j]);
//			else
//			{
//				// xframe의 weight 구하는 식 : 현재 프레임=10, 타겟 값 20 
//				bool isfind = false;
//				
//				int weightMax = imageHandler::getMaximumValue(outLineSetTemp[j].weightMat.binImage);
//				int endFrame = outLineSetTemp[j].weightMat.frameNum;
//				int stFrame = endFrame - weightMax;
//				for (int k = 0; k < mergedSeparatedMat.size(); k++)
//				{
//					int weightMax_t = imageHandler::getMaximumValue(mergedSeparatedMat[k].weightMat.binImage);
//					int endFrame_t = mergedSeparatedMat[k].weightMat.frameNum;
//					int stFrame_t = endFrame_t - weightMax_t;
//					Mat temp_t;
//					inRange(mergedSeparatedMat[k].weightMat.binImage, weightMax_t, weightMax_t - 5, temp_t);
//					int tempLeft_t = imageHandler::getLeftistWhitePixel_x(temp_t);
//					int tempRight_t = imageHandler::getRightistWhitePixel_x(temp_t);
//
//					// 관련있는지 확인
//					bool isReration = imageHandler::isRelation(stFrame, endFrame, stFrame_t, endFrame_t);
//					if (isReration)
//					{
//						// 특정부분이 겹침 -> 병합 (프레임 어떻게 맞추는가.. 시작프레임이 큰애 기준)
//						//if (stFrame > stFrame_t)
//						{	// stFrame_t 기준으로 .. stFrame_t ~ stFrame_t+5 프레임 영역 사용 
//
//							int offset = mergedSeparatedMat[k].weightMat.frameNum - outLineSetTemp[k].weightMat.frameNum;
//
//							Mat temp;
//							inRange(outLineSetTemp[j].weightMat.binImage, weightMax_t-offset, weightMax_t-offset - 5, temp);
//
//							int tempLeft = imageHandler::getLeftistWhitePixel_x(temp);
//							int tempRight = imageHandler::getRightistWhitePixel_x(temp);
//
//							if (imageHandler::isRelation(tempLeft, tempRight, tempLeft_t, tempRight_t))
//							{
//								// 병합	-  frameStart가 낮은곳 기준으로 병합
//								isfind = true;
//								break;
//							}
//
//
//						}
//						// 겹치는 부분
//					}
//
//				}
//				if (isfind == false)	// 못찾음
//				{
//					// 새로 추가함  => mergedSeparatedMat
//					mergedSeparatedMat.push_back(outLineSetTemp[j]);
//				}
//
//			}
//
//		}
//	}


	{
		vector<contourLineInfo> mergedSeparatedMat;	// 처리된 Mat
		for (int j = 0; j < outLineSetTemp.size(); j++)
		{
			if (j == 0)
				mergedSeparatedMat.push_back(outLineSetTemp[j]);
			else
			{
				bool isfind = false;

				// 1. separatedMat의 max값 구함
				int maxValue = imageHandler::getMaximumValue(outLineSetTemp[j].weightMat.binImage);
				if (maxValue - 5 <= 0)
					maxValue = 5;	

				// 1. separatedMat의 inrange(max, max-5) 한 것의 시작점-끝점구함
				Mat temp;
				inRange(outLineSetTemp[j].weightMat.binImage, maxValue - 5, maxValue, temp);
				int tempLeft = imageHandler::getLeftistWhitePixel_x(temp);
				int tempRight = imageHandler::getRightistWhitePixel_x(temp);

				for (int k = 0; k < mergedSeparatedMat.size(); k++)
				{ 
					if (outLineSetTemp[j].weightMat.frameNum != mergedSeparatedMat[k].weightMat.frameNum)
						continue;

					Mat mergeTemp;
					// 1. mergedSeparatedMat의 inrange(max, max-5) 한 것의 시작점-끝점구함
					inRange(mergedSeparatedMat[k].weightMat.binImage, maxValue - 5, maxValue, mergeTemp);
					int mergeTempLeft = imageHandler::getLeftistWhitePixel_x(mergeTemp);
					int mergeTempRight = imageHandler::getRightistWhitePixel_x(mergeTemp);
					// 1. isRelation() 에 참이 나옴 -> mergedSeparatedMat에 합병시킴
					//					거짓이 나옴 -> 새로 추가 
					bool isReration = imageHandler::isRelation(tempLeft, tempRight, mergeTempLeft, mergeTempRight);
					if (isReration)
					{
						bitwise_or(mergedSeparatedMat[k].weightMat.binImage, outLineSetTemp[j].weightMat.binImage, mergedSeparatedMat[k].weightMat.binImage);
						// separatedMat[k]
					}
					
					// 조건에 맞음	( separatedMat[j]가 mergedSeparatedMat[k]
					if (isReration)
					{
						isfind = true;
						break;
					}
				}
				if (isfind == false)	// 못찾음
				{
					// 새로 추가함  => mergedSeparatedMat
					mergedSeparatedMat.push_back(outLineSetTemp[j]);
				}

			}
		}

		for (int j = 0; j < mergedSeparatedMat.size(); j++)
		{
			outLineSet.push_back(mergedSeparatedMat[j]);
		}
	}
	
	return outLineSet;
}

vector<LineInfo> LineInfoFinder::mergeAndJudgeLineInfo(vector<LineInfo> lineInfos)
{
	vector<LineInfo> lineInfo_out;

	lineInfo_out = mergeSeparatedByMaximumFrame(lineInfos);	// ys -> 그냥 weightMat 사이즈 올려? char -> short
	lineInfo_out = mergeLineInfo(lineInfo_out);	// 라인 머지함

	for (int i = 0; i < lineInfo_out.size(); i++)
	{
		Mat floodfill = imageHandler::getBorderFloodFilledImage(lineInfo_out[i].maskImage.binImage);
		Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(floodfill);
		Mat clearedImage = imageHandler::removeNotPrimeryLyricLine(erodeImage_Denoise);

		LineInfo tempLineinfo;
		tempLineinfo.maskImage.binImage = clearedImage;
		tempLineinfo = LineFinder::checkValidMask(tempLineinfo);
		if (tempLineinfo.isValid)
		{
			lineInfo_out[i].maskImage.binImage = tempLineinfo.maskImage.binImage.clone();
			lineInfo_out[i].isValid = true;
		}
		else
		{
			lineInfo_out[i].errorOccured(LINEERROR_MERGE_ERROR);
		}
	}

	/*라인들 머지 실행 (isRelation)*/
	// 관련된 라인 합체(start-end, Mat(bitwise_or)-> 결과에서 다시 후처리(네모칸지우기 등등), 결과 픽셀 확인)
	return lineInfo_out;
}

// weight가 255를 넘어가서 라인이 나눠진 경우 머지하는 로직
vector<LineInfo> LineInfoFinder::mergeSeparatedByMaximumFrame(vector<LineInfo> lineInfos)
{
	int relationCount = 0;
	vector<LineInfo> lineInfo_temp;

	sort(lineInfos.begin(), lineInfos.end(), LineInfo::asc);	// 소팅

	for (int i = 0; i < lineInfos.size(); i++)
	{
		if (i == 0)
		{
			lineInfo_temp.push_back(lineInfos[i]);
		}
		else
		{
			int frameLength = lineInfo_temp.back().frame_end - lineInfo_temp.back().frame_start;
			if (frameLength > 250 && frameLength < 260)
			{
				// 조건
				// 1. 맥시멈 값이 255 임,
				// 2. 두 lineInfo 의 weight이 겹치지 않음 (노이즈 고려)		// 첫라인의 가장 오른쪽 x좌표 < 두번째라인의 평균 x좌표
				bool isRelation_frame = imageHandler::isRelation(lineInfo_temp.back().frame_start, lineInfo_temp.back().frame_end, lineInfos[i].frame_start, lineInfos[i].frame_end);
				// y좌표 겹쳐저야함

				bool isOtherLine_byYcoor = false;
				int temp_coorY_avg = imageHandler::getWhitePixelAvgCoordinate(lineInfo_temp.back().maskImage.binImage, false);
				int other_coorY_avg = imageHandler::getWhitePixelAvgCoordinate(lineInfos[i].maskImage.binImage, false);
				if (abs(temp_coorY_avg - other_coorY_avg) > 70)// relation_y값 진행 ->참이면 머지수행, 거짓-> 거리가 40 이상이면(s-e, e-s 값 절대값 중 작은값이 40 이상) 각각의 라인으로 판단
				{
					isOtherLine_byYcoor = true;	// 점들의 평균점의 차이가 70 이상이면서 평균 웨이트의 차이가 크면 다른 라인으로 봄.					
				}

				if (isRelation_frame == false && isOtherLine_byYcoor==false)
				{
					int xCoor = imageHandler::getRightistWhitePixel_x(lineInfo_temp.back().maskImage.binImage);

					if (xCoor < imageHandler::getWhitePixelAvgCoordinate(lineInfos[i].maskImage.binImage, true))
					{
						relationCount++;
						if (lineInfo_temp.back().frame_start > lineInfos[i].frame_start)
							lineInfo_temp.back().frame_start = lineInfos[i].frame_start;
						if (lineInfo_temp.back().frame_end < lineInfos[i].frame_end)
							lineInfo_temp.back().frame_end = lineInfos[i].frame_end;
						Mat orImg;
						bitwise_or(lineInfo_temp.back().maskImage.binImage, lineInfos[i].maskImage.binImage, orImg);
						lineInfo_temp.back().maskImage.binImage = orImg.clone();
						// 라인 머지  to .back()
						continue;
					}
				}
			}

			lineInfo_temp.push_back(lineInfos[i]);
		}

	}

	if (relationCount == 0)
		return lineInfo_temp;
	else
		return mergeSeparatedByMaximumFrame(lineInfo_temp);
}

vector<LineInfo> LineInfoFinder::mergeLineInfo(vector<LineInfo> lineInfos)	// maskImage.frame 을 사용하여 머지할거 머지하자.
{
	int relationCount = 0;
	vector<LineInfo> lineInfo_temp;

	sort(lineInfos.begin(), lineInfos.end(), LineInfo::asc);	// 소팅

	for (int i = 0; i < lineInfos.size(); i++)
	{
		if (i == 0)
		{
			lineInfo_temp.push_back(lineInfos[i]);
		}
		else
		{
			bool isRelation = imageHandler::isRelation(lineInfo_temp.back().frame_start, lineInfo_temp.back().frame_end, lineInfos[i].frame_start, lineInfos[i].frame_end);
			if (isRelation)	// 겹친다면 병합 수행
			{
				bool isOtherLine_byYcoor = false;
				bool isOtherLine_byframe = false;	// 다 만족해야 다른 라인으로 봄

				int temp_coorY_avg = imageHandler::getWhitePixelAvgCoordinate(lineInfo_temp.back().maskImage.binImage, false);
				int other_coorY_avg = imageHandler::getWhitePixelAvgCoordinate(lineInfos[i].maskImage.binImage, false);
				if (abs(temp_coorY_avg - other_coorY_avg) > 70)// relation_y값 진행 ->참이면 머지수행, 거짓-> 거리가 40 이상이면(s-e, e-s 값 절대값 중 작은값이 40 이상) 각각의 라인으로 판단
				{
					isOtherLine_byYcoor = true;	// 점들의 평균점의 차이가 70 이상이면서 평균 웨이트의 차이가 크면 다른 라인으로 봄.					
				}

				// 부분집합인지 확인
				bool isSubset = false;
				if (lineInfo_temp.back().frame_start <= lineInfos[i].frame_start && lineInfo_temp.back().frame_end >= lineInfos[i].frame_end
					|| lineInfo_temp.back().frame_start >= lineInfos[i].frame_start && lineInfo_temp.back().frame_end <= lineInfos[i].frame_end)
				{
					isSubset = true;
				}

				int allLength_start = min(lineInfo_temp.back().frame_start, lineInfos[i].frame_start);
				int allLength_end = max(lineInfo_temp.back().frame_end, lineInfos[i].frame_end);

				int dupFrame_start = max(lineInfo_temp.back().frame_start, lineInfos[i].frame_start);
				int dupFrame_end = min(lineInfo_temp.back().frame_end, lineInfos[i].frame_end);
				int dupFrame = dupFrame_end - dupFrame_start;	//

				if (dupFrame <= 0)	// 프레임 안겹침
				{
					isOtherLine_byframe = true;
				}
				else
				{
					int allFrame = allLength_end - allLength_start;
					if (allFrame == 0)	// error 방지
						;
					else if ((dupFrame / (float)allFrame) < 0.7)	// 70프로도 겹치지 않음
						isOtherLine_byframe = true;
				}

				if (lineInfo_temp.back().printColor != lineInfos[i].printColor)	// 두 라인의 컬러코드가 다르면 듀엣으로 봄
				{	// 듀엣라인검사
					if (isOtherLine_byYcoor && isOtherLine_byframe)
					{
						lineInfo_temp.push_back(lineInfos[i]);;// 점들의 평균점의 차이가 70 이상이면서 평균 웨이트의 차이가 크면 다른 라인으로 봄.
						continue;
					}
					else // 같은 라인 (머지)
					{
						relationCount++;
						if (lineInfo_temp.back().frame_start > lineInfos[i].frame_start)
							lineInfo_temp.back().frame_start = lineInfos[i].frame_start;
						if (lineInfo_temp.back().frame_end < lineInfos[i].frame_end)
							lineInfo_temp.back().frame_end = lineInfos[i].frame_end;
						Mat orImg;
						bitwise_or(lineInfo_temp.back().maskImage.binImage, lineInfos[i].maskImage.binImage, orImg);
						lineInfo_temp.back().maskImage.binImage = orImg.clone();

						if (lineInfo_temp.back().printColor != lineInfos[i].printColor)	// 두 라인의 컬러코드가 다르면 듀엣으로 봄
							lineInfo_temp.back().printColor = 3;	// 컬러코드 (3==듀엣)
					}
				}
				else
				{	// 일반라인검사
					if (isOtherLine_byYcoor && isOtherLine_byframe && !isSubset)	
					{
						lineInfo_temp.push_back(lineInfos[i]);
						continue;
					}
					else // 같은 라인 (머지)
					{
						relationCount++;
						if (lineInfo_temp.back().frame_start > lineInfos[i].frame_start)
							lineInfo_temp.back().frame_start = lineInfos[i].frame_start;
						if (lineInfo_temp.back().frame_end < lineInfos[i].frame_end)
							lineInfo_temp.back().frame_end = lineInfos[i].frame_end;
						Mat orImg;
						bitwise_or(lineInfo_temp.back().maskImage.binImage, lineInfos[i].maskImage.binImage, orImg);
						lineInfo_temp.back().maskImage.binImage = orImg.clone();

					}

				}
			}
			else
			{
				if (lineInfo_temp.back().printColor == lineInfos[i].printColor)	// 두 라인의 컬러코드가 같음
				{
					bool isOtherLine_byXcoor = false;	// X좌표에 의하면 다른 라인임 = X좌표가 집합관계에 있다면 다른 라인으로 봄
					bool isOtherLine_byYcoor = false;	// Y좌표에 의하면 다른 라인임 = Y 평균좌표의 차이가 많이남

					int dupFrame_start = max(lineInfo_temp.back().frame_start, lineInfos[i].frame_start);
					int dupFrame_end = min(lineInfo_temp.back().frame_end, lineInfos[i].frame_end);
					int dupFrame = dupFrame_end - dupFrame_start;	//

					int lineInfo_temp_Xcoor_start = imageHandler::getLeftistWhitePixel_x(lineInfo_temp.back().maskImage.binImage);
					int lineInfos_Xcoor_start = imageHandler::getLeftistWhitePixel_x(lineInfos[i].maskImage.binImage);
					int lineInfo_temp_Xcoor_end = imageHandler::getRightistWhitePixel_x(lineInfo_temp.back().maskImage.binImage);
					int lineInfos_Xcoor_end = imageHandler::getRightistWhitePixel_x(lineInfos[i].maskImage.binImage);

					int dupXcoor_start = max(lineInfo_temp_Xcoor_start, lineInfos_Xcoor_start);
					int dupXcoor_end = min(lineInfo_temp_Xcoor_end, lineInfo_temp_Xcoor_end);
					int dupXcoor = dupXcoor_end - dupXcoor_start;	//

					int temp_coorY_avg = imageHandler::getWhitePixelAvgCoordinate(lineInfo_temp.back().maskImage.binImage, false);
					int other_coorY_avg = imageHandler::getWhitePixelAvgCoordinate(lineInfos[i].maskImage.binImage, false);
					if (abs(temp_coorY_avg - other_coorY_avg) > 70)// relation_y값 진행 ->참이면 머지수행, 거짓-> 거리가 40 이상이면(s-e, e-s 값 절대값 중 작은값이 40 이상) 각각의 라인으로 판단
					{
						isOtherLine_byYcoor = true;	// 점들의 평균점의 차이가 70 이상이면서 평균 웨이트의 차이가 크면 다른 라인으로 봄.					
					}

					if (dupXcoor <= 0						// X좌표 안겹침
						&& (dupFrame<0 && dupFrame>-10))	// 적은 프레임차이(10프레임?)
					{
						isOtherLine_byXcoor = false;		// 머지 진행함
					}
					else
						isOtherLine_byXcoor = true;

					if (isOtherLine_byXcoor || isOtherLine_byYcoor)
					{
						lineInfo_temp.push_back(lineInfos[i]);
						continue;
					}
					else
					{
						relationCount++;
						if (lineInfo_temp.back().frame_start > lineInfos[i].frame_start)
							lineInfo_temp.back().frame_start = lineInfos[i].frame_start;
						if (lineInfo_temp.back().frame_end < lineInfos[i].frame_end)
							lineInfo_temp.back().frame_end = lineInfos[i].frame_end;
						Mat orImg;
						bitwise_or(lineInfo_temp.back().maskImage.binImage, lineInfos[i].maskImage.binImage, orImg);
						lineInfo_temp.back().maskImage.binImage = orImg.clone();
					}
				}
				else
				{
					lineInfo_temp.push_back(lineInfos[i]);
				}
			}
		}
	}

	if (relationCount == 0)
		return lineInfo_temp;
	else
		return mergeLineInfo(lineInfo_temp);
}


// WBW 패턴으로 이미지 구함
Mat LineInfoFinder::getPatternFillImage(Mat rgbImage, Scalar targetColor)	// YS
{
	Mat PatternBin = imageHandler::getPaintedPattern(rgbImage, targetColor, true);	// 내부에서 dilate함
	//Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);
	Mat fullyContrastImage = imageHandler::getPixelContrastImage(rgbImage);		// YS - 이거랑 비교
	Mat fullyContrastImage_per = imageHandler::getPixelContrastImage_byPercent(rgbImage);

	Mat FC_Bin = getFillImage(rgbImage, targetColor);
	Mat PatternFullfill;
	PatternFullfill = imageHandler::getFloodfillImage(FC_Bin, PatternBin);	// FullCont 이미지에 패턴으로 인식한 좌표로 패인트통연산
	PatternFullfill = imageHandler::getBorderFloodFilledImage(PatternFullfill);
	//Mat erodeImage = imageHandler::getMorphImage(PatternFullfill, MORPH_ERODE);	// 침식연산
	//Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(erodeImage);	// 사각박스있는곳 제거
	Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(PatternFullfill);	// 사각박스있는곳 제거

	return erodeImage_Denoise;
}

// 그냥 색으로 이미지 구함
Mat LineInfoFinder::getPatternFillImage_2(Mat rgbImage, Scalar targetColor)	// YS
{
	Mat FC_Bin = getFillImage(rgbImage, targetColor);
	inRange(FC_Bin, Scalar(254, 254, 254), Scalar(255, 255, 255), FC_Bin);	// to 1 demend
	Mat PatternFullfill;
	PatternFullfill = imageHandler::getBorderFloodFilledImage(FC_Bin);
	Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(PatternFullfill);	// 사각박스있는곳 제거
	return erodeImage_Denoise;
	//return FC_Bin;
}

Mat LineInfoFinder::getFillImage(Mat rgbImage, Scalar targetColor)
{
	//Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);
	//Mat fullyContrastImage_pix = imageHandler::getPixelContrastImage(rgbImage);
	Mat fullyContrastImage_per = imageHandler::getPixelContrastImage_byPercent(rgbImage);

	Mat FC_Bin;
	Scalar patternMin = targetColor;
	for (int i = 0; i < 3; i++)
		if (patternMin[i] != 0)
			patternMin[i] = patternMin[i] - 1;

	inRange(fullyContrastImage_per, patternMin, targetColor, FC_Bin);	// 파랑만이미지
	cvtColor(FC_Bin, FC_Bin, COLOR_GRAY2BGR);

	return FC_Bin;
}

// 시작 frame 정보를 이용한 필터  (p이미지의 값이 up이미지의 값보다 크거나 같은 점만 납둠)
// 
Mat LineInfoFinder::getWeightCompareImage(Mat weightUnpaint, Mat weightPaint)
{
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
			if (diff <= 5)	// p가 np보다 크거나 같은값일 때
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

