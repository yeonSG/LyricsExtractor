#include "analyzer.h"
#include "testClass.h"

int analyzer::getContourCount(Mat cannyImage)
{
	vector<vector<Point>> contours;
	findContours(cannyImage, contours, RETR_TREE, CHAIN_APPROX_SIMPLE); // Image에서 Contour를 찾음

	return contours.size();
}

/*
	파란색으로 Line의 시작과 끝을 알아내려 했던 알고리즘이지만, 동영상에 파란색이 아닌 주황색인 MV를 발견하여 쓰지않을생각.
*/
bool analyzer::videoContoursAnalyzation(string videoPath)
{
	vector<int> vecContours;
	vector<int> vecWhiteCount;

	VideoCapture vc(videoPath);
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return false;
	}
	videoHandler::printVideoSpec(vc);

	Mat orgImage;
	while (vc.read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(vc);

		Mat subImage = imageHandler::getSubtitleImage(orgImage);
		Mat binImage = imageHandler::getBlueColorFilteredBinaryImage(subImage);
		//Mat binImage = imageHandler::getBinaryImage(subImage);
		Mat morphImage = imageHandler::getMorphImage(binImage);
		Mat cannyImage = imageHandler::getCannyImageWithBinaryImage(morphImage);

		int Contours = getContourCount(cannyImage);
		int whiteCount = getWihtePixelCount(binImage);

		vecContours.push_back(Contours);
		vecWhiteCount.push_back(whiteCount);
		
		int key = waitKey(0);
	}
	vector<int> vecAverageWhiteCount = vectorToAverageVector(vecWhiteCount, 2);
	getJudgedLine(vecAverageWhiteCount);

	string fileName = "ContoursCount.txt";
	fileManager::writeVector(fileName, vecContours);

	fileName = "WhiteCount.txt";
	fileManager::writeVector(fileName, vecWhiteCount);


	return true;
}

/*
	0번 알고리즘의 한계 때문에 생각해낸 알고리즘임.
	순서 :
		1. 이미지 흰색으로 이진화
		2. x(cols)축으로 흰색 픽셀의 개수를 카운트함.
		3. 이전 frame과 x축의 흰색 개수를 비교하여 일정 수(미정) 이상 변경된 부분만 표시한 데이터를 얻음
		4. 변경된 부분만 표시한 데이터에서 x데이터 low->high로 올라가는 패턴을 가진 부분 추출 (Expected Line)
		
		5. 
*/
bool analyzer::videoContoursAnalyzation1(string videoPath)
{
	VideoCapture vc(videoPath);
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return false;
	}
	videoHandler::printVideoSpec(vc);

	videoHandler::printCurrentFrameSpec(vc);
	
	Mat orgImage;
	vector<vector<int>> whiteProjectionCounts;

	while (vc.read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(vc);

		Mat subImage = imageHandler::getSubtitleImage(orgImage);
		Mat binImage = imageHandler::getBinaryImage(subImage);
		Mat morphImage = imageHandler::getMorphImage(binImage);
		
		vector<int> whiteProjectionCount;
		whiteProjectionCount = getVerticalProjectionData(morphImage);	// 현 프레임의 x축 프로젝션
		whiteProjectionCounts.push_back(whiteProjectionCount);

		//int key = waitKey(0);
	}
	Mat changeHistogram;
	changeHistogram = getChangeHistorgramMat(whiteProjectionCounts, 10);//= Mat::ones(orgImage.cols, 0, CV_64F);	// col, row


	imshow("graph", changeHistogram);
	return false;
}

/*
	0번 알고리즘의 한계 때문에 생각해낸 알고리즘임.
	 - 원인: 기존 알고리즘은 파랑색(HVS)값으로 판별했는데 movie1.mp4 파랑이 너무많음
	순서 :
*/
bool analyzer::videoContoursAnalyzation2(string videoPath)
{
	VideoCapture vc(videoPath);
	if (!vc.isOpened())
	{
		cout << "fail to open the video" << endl;
		return false;
	}
	videoHandler::printVideoSpec(vc);

	videoHandler::printCurrentFrameSpec(vc);

	Mat orgImage;
	Mat beforeBinImage;
	vector<int> whitePixelChangedCounts;		// 픽셀 변화값
	vector<vector<int>> whiteProjectionCounts;	// 픽셀 변화값의 이미지화

	while (vc.read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(vc);

		Mat subImage = imageHandler::getSubtitleImage(orgImage);
		Mat binImage;	// = getBinaryImage(subImage);
		{
			Mat image_binAT, image_binIR;
			Mat image_gray;
			cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
			adaptiveThreshold(image_gray, image_binAT, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
			
			Mat element5(7, 7, CV_8U, Scalar(1));
			element5 = getStructuringElement(MORPH_ELLIPSE, Point(1, 7));	// @이 값 조정해서 지난 텍스트가 사라지지 않도록 조정해보자
			erode(image_binAT, image_binAT, element5);

			Mat image_floodFilled_AT = imageHandler::getFloodProcessedImage(image_binAT);

			inRange(subImage, Scalar(185, 185, 185), Scalar(255, 255, 255), image_binIR);
			element5 = getStructuringElement(MORPH_ELLIPSE, Point(3, 5));
			Mat image_floodFilled_IR = imageHandler::getFloodProcessedImage(image_binIR);

			// 3-1. Blue(HSV)의 이진화
			Mat subImage_hsv;
			cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
			// 3. 이진화
			//Mat image_binIR_HSV;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
			//inRange(subImage_hsv, Scalar(0, 0, 65), Scalar(180, 15, 255), image_binIR_HSV);
			//Mat image_floodFilled_IR_HSV = imageHandler::getFloodProcessedImage(image_binIR_HSV.clone());

			//bitwise_and(image_floodFilled_IR, image_floodFilled_AT, binImage);
			bitwise_and(image_floodFilled_IR, image_floodFilled_AT, binImage);
		}


		if (beforeBinImage.empty() == true)
		{
			whitePixelChangedCounts.push_back(0);

			whiteProjectionCounts.push_back(getVerticalProjectionData(binImage));	// temporary Image;
		}
		else
		{
			Mat ChangedToWhiteImage = imageHandler::getDifferenceImage(binImage, beforeBinImage);
			whitePixelChangedCounts.push_back(getWihtePixelCount(ChangedToWhiteImage));

			vector<int> whiteProjectionCount;
			whiteProjectionCount = getVerticalProjectionData(ChangedToWhiteImage);	// 현 프레임의 x축 프로젝션
			whiteProjectionCounts.push_back(whiteProjectionCount);
		}

		beforeBinImage = binImage;
		////int key = waitKey(0);
		//int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);
		//vc.set(CAP_PROP_POS_FRAMES, (double)curFrame + 2 - 1);
	}
	Mat changeHistogram; // 체인지 히스토그램
	changeHistogram = getChangeHistorgramMat(whiteProjectionCounts, 10);//= Mat::ones(orgImage.cols, 0, CV_64F);	// col, row
	imwrite(videoPath + "_Histogram.jpg", changeHistogram);

	Mat changeHistogramWeight; // 체인지 히스토그램의 평균점
	changeHistogramWeight = getVerticalHistogramAverageMat(changeHistogram);
	imwrite(videoPath + "_HistogramWeight.jpg", changeHistogramWeight);

	string fileName = "whitePixelChangedCounts.txt";
	fileManager::writeVector(fileName, whitePixelChangedCounts);

	vector<pair<int, int>> lines = getJudgedLine2(whitePixelChangedCounts);
	// Save pictures

	captureLines(lines, videoPath);
	capturedLinesToText(lines.size(), videoPath);
	makeLyrics(lines, videoPath);
	
	vc.release();
	return false;
}

bool analyzer::videoContoursAnalyzation3(string videoPath)
{
	if (!setVideo(videoPath))
	{
		return false;
	}
	fileManager::videoName = videoPath; // 비디오 이름만 넣어줘야함
	string savePath = fileManager::getSavePath();

	Mat orgImage;
	Mat subImage;
	Mat binImage;
	Mat beforeBinImage;	// 이전 바이너리 이미지
	vector<int> vecWhitePixelCounts;			// 프래임 별 흰색 개수
	vector<int> vecWhitePixelChangedCounts;		// 이전 프래임 대비 흰색 변화량
	vector<vector<int>> verticalProjectionDatasOfDifferenceImage;	// 디프런트 이미지의 흰색 vertical Projection데이터의 모임

	/* 이미지 분석 */
	while (videoCapture->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*videoCapture);

		subImage = imageHandler::getSubtitleImage(orgImage);
		binImage = imageHandler::getCompositeBinaryImages(subImage);

		//White dot Count
		vecWhitePixelCounts.push_back(getWihtePixelCount(binImage));

		if (beforeBinImage.empty() == true)
		{
			verticalProjectionDatasOfDifferenceImage.push_back(getVerticalProjectionData(binImage));	// insert zero row

			vecWhitePixelChangedCounts.push_back(0);
		}
		else
		{
			Mat DifferenceImage = imageHandler::getDifferenceImage(binImage, beforeBinImage);
			vector<int> verticalProjectionData;
			verticalProjectionData = getVerticalProjectionData(DifferenceImage);	// 현 프레임의 vertical 프로젝션
			verticalProjectionDatasOfDifferenceImage.push_back(verticalProjectionData);

			vecWhitePixelChangedCounts.push_back(getWihtePixelCount(DifferenceImage));
		}

		beforeBinImage = binImage;
	}

	fileManager::initDirectory(videoPath);

	string fileName = "WhitePixelCount.txt";
	fileManager::writeVector(fileName, vecWhitePixelCounts);
	fileName = "WhitePixelChangedCount.txt";
	fileManager::writeVector(fileName, vecWhitePixelChangedCounts);


	Mat changeHistogramMat; // 체인지 히스토그램	//changeHistogram = getChangeHistorgramMat(verticalProjectionDatasOfDifferenceImage, 10);//= Mat::ones(orgImage.cols, 0, CV_64F);	// col, row
	vector<vector<bool>> changeHistorgramData = getChangeHistorgramData(verticalProjectionDatasOfDifferenceImage, 10);
	changeHistogramMat = vectorToBinaryMat(changeHistorgramData);
	imwrite(savePath + "ChangeHistogram.jpg", changeHistogramMat);

	vector<int> verticalHistogramAverage = getVerticalHistogramAverageData(changeHistorgramData);	// getChangeStatusAverage -> getChangeAverageHistogramData
	fileName = "WhitePixelChangedCountAverage.txt";
	fileManager::writeVector(fileName, verticalHistogramAverage);

	Mat changeHistogramAverageMat = averageVectorToBinaryMat(verticalHistogramAverage, changeHistogramMat.cols); // 체인지 히스토그램의 평균점 이미지
	imwrite(savePath + "changeHistogramAverage.jpg", changeHistogramAverageMat);
	
	/* 라인 확정 */
	vector<pair<int, int>> lines = getJudgedLine4(vecWhitePixelCounts, verticalHistogramAverage);

	// Save pictures
	captureLines(lines, savePath);
	capturedLinesToText(lines.size(), savePath);
	makeLyrics(lines, savePath);

	closeVideo();
	return false;
}

/// <summary>
/// Mat의 흰색 점의 개수를 반환함
/// </summary>
/// <param name="binMat">The bin mat.</param>
/// <returns>흰색 점의 개수</returns>
int analyzer::getWihtePixelCount(Mat binMat)
{
	int height = binMat.rows;
	int width = binMat.cols;
	int whiteCount = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binMat.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// 흑색이 아닌경우
				whiteCount++;
	}
	//printf("whiteCount:%d\r\n", whiteCount);
	return whiteCount;
}

/// <summary>
/// 입력 vector의 평균화 한 vector를 반환
/// ex) effectiveArea가 2라면 x번째의 값은 (x-2 .. + x + .. x+2) 가 됨.
/// </summary>
/// <param name="vec">The vector.</param>
/// <param name="effectiveArea">The effective area.</param>
/// <returns>평균화 한 vector</returns>
vector<int> analyzer::vectorToAverageVector(vector<int> vec, int effectiveRange)
{
	vector<int> vecAverage;

	for (int i = 0; i < vec.size(); i++)
	{
		int sum = (int)getAverageOnVectorTarget(vec, i, effectiveRange);
		vecAverage.push_back(sum);
	}

	return vecAverage;
}

vector<pair<int, int>> analyzer::getJudgedLine(vector<int> vec)
{
	vector<pair<int, int>> judgedLines;

	int increaseJudgeValue = 40;
	int increaseFrameJudgeCount = 25;
	int continuouslyIncreaseFrameCount = 0;

	int capturedStartPoint;
	int expectedCapturedStartPoint = 0;

	// 40pix씩 증가가 x 프래임동안 증가 시 시작점, 증가하던 pix이 
	for (int i = 1; i < vec.size(); i++)
	{
		if ((vec[i] - (vec[i-1])) > increaseJudgeValue)	// 현재값 - 이전값 > 40 이면 증가로 판단
		{
			continuouslyIncreaseFrameCount++;
			if (continuouslyIncreaseFrameCount == 25)	// 작점으로 판단
			{
				expectedCapturedStartPoint = i;
				printf("expectedCapturedStartPoint = i;\r\n");
			}
		}
		else // + 추세가 끊김 (count가 25frame 이상 지속되었다면)
		{
			if (expectedCapturedStartPoint != 0)
			{
				judgedLines.push_back(make_pair(expectedCapturedStartPoint -25, i));
				printf("found line : Start: %4d, End: %4d\r\n", expectedCapturedStartPoint - 25, i);
				expectedCapturedStartPoint = 0;
			}
			continuouslyIncreaseFrameCount = 0;
		}

		//if (continuouslyIncreaseFrameCount == 1)	// 예측 시작점 저장
		//	expectedCapturedStartPoint = i;


	}

	for (int i = 0; i < vec.size(); i++)
	{
		// 증가counter 증가
		// 감소counter 증가
		// 데이터(시작시간, 시작 데이터, 끝시간, 최대 값)	// 시작시간이
		// 
	}


		
	return judgedLines;
}

/*
// 입력 : 
// 출력 : Pairs of Line

	1. 특정 프레임 기준 좌우를 살핌,
		[Fade-In, Fade-Out의 경우]
			- 좌측에 변화량이 없고 우측에 변화량이 있다면 시작점
			- 우측에 변화량이 없고 좌측에 변화량이 있다면 끝점
		[Line이 변경되는 경우]
			- 특점 지점에 좌, 우측과 대조되는 높은 Peak가 생성 (시작점이자, 끝점)
			  (5000 이상이면서 주변(1초=25Frame)에 자기보다 큰 값이 없는 것)
*/
vector<pair<int, int>> analyzer::getJudgedLine2(vector<int> vec)
{
	vector<pair<int, int>> lines;
	vector<int> peakValues;
	
	// Peak 추출
	for (int frameNum = 0; frameNum < vec.size(); frameNum++)
	{
		if (vec[frameNum] > 5000)
		{
			bool isPeak = true;
			for (int checkRange = (frameNum - DEFAULT_FPS/2); checkRange < frameNum + DEFAULT_FPS/2; checkRange++)
			{
				if (frameNum == checkRange)
					continue;

				if (checkRange<0 || checkRange>vec.size())
				{
					isPeak = false;
					break;	// 시작점, 끝점은 안봄
				}

				//  조건1. 자신 주변에 자신의 70% 이상의 값을 가진것이 없슴.
				if (vec[checkRange] > ((vec[frameNum] / 100) * 70))	
				{
					isPeak = false;
					break;
				}

				//  조건2. 자신 주변에 0이 없음
				if (getAverageOnVectorTarget(vec, frameNum-DEFAULT_FPS, 3) < 1 || getAverageOnVectorTarget(vec, frameNum + DEFAULT_FPS, 3) < 1)
				{
					isPeak = false;
					break;
				}
			}
			if (isPeak)
				peakValues.push_back(frameNum);
		}
	}
	
	for (int i = 0; i < peakValues.size(); i++)
		printf("Peak %2d : %d\r\n",i, peakValues[i]);
	
	/*
		Peak
		Frame 보정값
			fade-in		: +15 
			peak end	: -2
			peak start	: +2
			fade-out	: -15
	*/
	bool isStartedZero = true;	// 처음 시작일경우, zero로 피크가 끝난경우 true로 쌧
	for (int i = 0; i < peakValues.size(); i++)
	{
		int nextPeak;
		if (i == peakValues.size() - 1)
			nextPeak = vec.size();
		else
			nextPeak = peakValues[i + 1];

		if (isStartedZero)	// 이전Frame으로 탐색
		{
			for (int frame = peakValues[i]; frame > 0; frame--)
			{
				if (getAverageOnVectorTarget(vec, frame, 3) == 0)
				{
					lines.push_back(make_pair(frame +15, peakValues[i]-2));	// (0시작, 피크로끝) fade-in
					isStartedZero = false;
					break;
				}
			}
		}
		
		for (int frame = peakValues[i]; frame < vec.size(); frame++)	// zero 또는 peak가 나올때까지 탐색
		{
			if (getAverageOnVectorTarget(vec, frame, 3) == 0)
			{
				lines.push_back(make_pair(peakValues[i]+2, frame-15));	// (피크시작, 0으로끝) fade-out
				isStartedZero = true;
				break;
			}
			else if (frame == nextPeak)
			{
				lines.push_back(make_pair(peakValues[i]+2, nextPeak-2)); // (피크시작, 피크로끝)
				isStartedZero = false;
				break;
			}
		}
	}

	for (int i = 0; i < lines.size(); i++)
		printf("lines %2d : %d - %d\r\n", i, lines[i].first, lines[i].second);

	return lines;
}

/*
// 입력 : Frame의 white dot 수의 배열
// 출력 : Pairs of Line

	처음부터 끝 frame까지 확인
	Line의 조건
	1. 최대값이 1000 이상
	2. 최소값이 200 이하

	0. 값이 0이면
*/
vector<pair<int, int>> analyzer::getJudgedLine3(vector<int> vec)
{
	bool lineCatched = false;
	int startLine, endLine = 0;

	vector<int> ends, starts;

	//for (int frameNum = 0; frameNum < vec.size() - 15; frameNum++)	// 1000 하항조정 (노이즈 제거)
	//{
	//	if(vec[frameNum]>=1000)
	//		vec[frameNum] -= 1000;
	//}

	for (int frameNum = 0; frameNum < vec.size()-15; frameNum++)
	{
		if (vec[frameNum] <= vec[frameNum + 15])	// 상승추세
			continue;
		
		if (vec[frameNum] > vec[frameNum + 15])	// 상승추세가 끝나고 하락.
		{
			int max = frameNum;
			for (int i = 0; i <= 15; i++)	// x+ 0~15 값 탐색
				if (vec[max] < vec[frameNum + i])
					max = frameNum + i;
			ends.push_back(max);
			frameNum += 15;	// 다음 end 탐색
			printf("endPoint: %d\r\n", max);
		}
	}

	for (int endNum = 0; endNum < ends.size(); endNum++)
	{	// 뒤로 탐색하면서 하강추세가 끝나는 최소지점
		int frameNum = ends[endNum];
		for (; frameNum >= 0+15; frameNum--)
		{
			if (vec[frameNum] > vec[frameNum - 15])	// 하강추세
				continue;

			if (vec[frameNum] < vec[frameNum - 15])	// 상승추세
			{
				int min = frameNum;

				for (int i = 0; i >= 15; i--)	// x+ 0~15 값 탐색
				{
					if (vec[min] > vec[frameNum - i])
						min = frameNum - i;
				}
				starts.push_back(min);
				printf("startPoint: %d\r\n", min);
				break;
			}
		}
	}

	printf("ends: %d \r\n starts: %d \r\n", (int)ends.size(), (int)starts.size());

	vector<pair<int, int>> lines;
	if (ends.size() == starts.size())
	{
		for(int i=0; ends.size(); i++)
			lines.push_back(make_pair(starts[i], ends[i])); // (피크시작, 피크로끝)
	}
	   
	return lines;
}

/*
input:
	- vecWhitePixcelCount	: 해당 프레임에 존재하는 흰 점 개수
	- vecChangeAverage	: 
*/
/// <summary>
/// 분석한 데이터로 가사 Line들의 시작점-끝점을 판단함
/// </summary>
/// <param name="vecWhitePixelCounts"> 프레임에 존재하는 흰 점의 합 </param>
/// <param name="changeStatusAverage"> 변한 흰색의 평균 위치</param>
/// <returns>가사 Line들의 시작점-끝점 Pair.</returns>
vector<pair<int, int>> analyzer::getJudgedLine4(vector<int> vecWhitePixelCounts, const vector<int> verticalHistogramAverage)
{
	vector<pair<int, int>> lines;
	vector<int> peakValues;

	// Peak 추출
	peakValues = getPeakFromWhitePixelCounts(vecWhitePixelCounts);

	for (int i = 0; i < peakValues.size(); i++)
		printf("Peak %2d : %d\r\n", i, peakValues[i]);

	string fileName = "peak.txt";
	fileManager::writeVector(fileName, peakValues);

	lines = getLinesFromPeak(peakValues, vecWhitePixelCounts);

	lineRejudgeByVerticalHistogramAverage(lines, verticalHistogramAverage);

	vector<string> lines_string;
	for (int i = 0; i < lines.size(); i++)
	{
		string line_string = to_string(lines[i].first) + "\t" + to_string(lines[i].second);
		lines_string.push_back(line_string);// ("lines %2d : %d - %d\r\n", i, lines[i].first, lines[i].second);
	}

	fileName = "lineStart_End.txt";
	fileManager::writeVector(fileName, lines_string);

	return lines;
}

/// <summary>
/// 각각 프래임별 흰 점의 갯수를 이용하여 피크를 구함
/// 피크는 흰 점의 갯수가 높은 곳을 말하며 가사의 패인팅이 다 된 곳을 가르킴
/// </summary>
/// <param name="vecWhitePixelCounts">프래임 별 흰 점의 테이터.</param>
/// <returns>피크들</returns>
vector<int> analyzer::getPeakFromWhitePixelCounts(vector<int> vecWhitePixelCounts)
{
	vector<int> peakValues;

	// Peak 추출
	for (int frameNum = 0; frameNum < vecWhitePixelCounts.size(); frameNum++)
	{
		if (vecWhitePixelCounts[frameNum] > 1000)	// 노이즈 레벨
		{
			bool isPeak = true;
			bool isHaveDrop = false;
			for (int checkRange = (frameNum - DEFAULT_FPS / 2); checkRange < frameNum + DEFAULT_FPS / 2; checkRange++)
			{
				if (frameNum == checkRange)
					continue;

				if (checkRange<0 || checkRange>vecWhitePixelCounts.size())
				{
					isPeak = false;
					break;	// 시작점, 끝점은 안봄
				}

				//  조건1. 자신 주변에 자신 이상의 값을 가진것이 없슴.
				if (vecWhitePixelCounts[checkRange] > vecWhitePixelCounts[frameNum])
				{
					isPeak = false;
					break;
				}
			}
			for (int checkRange = frameNum; checkRange < frameNum + DEFAULT_FPS * 2; checkRange++)	// 2Sec (fade-out 까지 포함하는 초)
			{
				// 자신보다 뒤쪽에 자신의 70% 하락한 값보다 작은 값이 있어야 함 - 이거..
				if (checkRange > frameNum)
				{
					if (vecWhitePixelCounts[checkRange] < (vecWhitePixelCounts[frameNum] / 10) * 7)
						isHaveDrop = true;
				}
			}

			if (isPeak && isHaveDrop)
			{
				if (!peakValues.empty())
				{

					if (vecWhitePixelCounts[peakValues.back()] == vecWhitePixelCounts[frameNum])
						peakValues.back() = frameNum;
					else
						peakValues.push_back(frameNum);
				}
				else
					peakValues.push_back(frameNum);
			}
		}
	}

	return peakValues;
}

/// <summary>
/// 피크와 흰 점의 개수들을 이용하여 가사 Line의 시작점-끝점을 반환
/// </summary>
/// <param name="vecWhitePixelCounts">">프래임 별 흰 점의 테이터</param>
/// <param name="peaks">피크</param>
/// <returns>가사 Line들</returns>
vector<pair<int, int>> analyzer::getLinesFromPeak(vector<int> peaks, vector<int> vecWhitePixelCounts)
{
	vector<pair<int, int>> lines;

	for (int i = 0; i < peaks.size(); i++)	// find start point
	{
		// 이전peak값 index보다 큼
		int minRange;
		if (i == 0)
			minRange = 0;
		else
			minRange = peaks[i - 1];

		int minIndex = minRange;
		for (int index = minRange; index < peaks[i]; index++)
		{
			if (vecWhitePixelCounts[minIndex] >= vecWhitePixelCounts[index])
			{
				minIndex = index;
			}
		}

		lines.push_back(make_pair(minIndex, peaks[i] + 1));
	}

	return lines;
}

/// <summary>
/// 가사 라인들을 히스토그램 데이터를 통해 다시한번 걸러냄
/// </summary>
/// <param name="judgedLines">가사 Line들</param>
/// <param name="verticalHistogramAverage">히스토그램 평균 데이터</param>
void analyzer::lineRejudgeByVerticalHistogramAverage(vector<pair<int, int>>& judgedLines, const vector<int> verticalHistogramAverage)
{
	int lineCount = 0;
	for (vector<pair<int, int>>::iterator it = judgedLines.begin(); it != judgedLines.end(); /*it++*/)
	{
		// 라인 중간 프래임 기준으로 좌 우의 평균 차이가 어느정도 있어야 함
		int avgLeft = 0;
		int avgLeftCount = 0;
		int avgRight = 0;
		int avgRightCount = 0;
		int middlePoint = (it->first + it->second) / 2;

		for (int frame = it->first; frame <= it->second; frame++)
		{
			if (verticalHistogramAverage[frame] == 0)
				continue;

			if (frame < middlePoint)
			{
				avgLeft += verticalHistogramAverage[frame];
				avgLeftCount++;
			}
			else
			{
				avgRight += verticalHistogramAverage[frame];
				avgRightCount++;
			}
		}
		if (avgLeft == 0 || avgRight == 0)
		{
			printf("exceptionLine(div by zero : %d\r\n", lineCount);
			it = judgedLines.erase(it);
			lineCount++;
			continue;
		}

		avgLeft = avgLeft / avgLeftCount;
		avgRight = avgRight / avgRightCount;
		float ratio = avgLeft / (float)avgRight;
		printf("lines %2d : Left:%d  Right:%d \t", lineCount, avgLeft, avgRight);
		printf("(ratio : %3.2f)\r\n", ratio);

		if (ratio > 0.90)
		{
			printf("exceptionLine : %d\r\n", lineCount);
			it = judgedLines.erase(it);
		}
		else
			++it;

		lineCount++;
	}
}

/// <summary>
/// 이진 이미지의 Vertical projection 데이터로 변환 
/// </summary>
/// <param name="binImage">변환할 이진 이미지.</param>
/// <returns>Vertical projection 데이터</returns>
vector<int> analyzer::getVerticalProjectionData(Mat binImage)
{
	int nRows = binImage.rows;
	int nCols = binImage.cols;

	vector<int> counts;
	for (int i = 0; i < binImage.cols; i++)	// inital
		counts.push_back(0);

	for (int j = 0; j < nRows; j++) {
		uchar* rowPtr = binImage.ptr<uchar>(j);
		for (int i = 0; i < nCols; i++) {
			if (rowPtr[i] != 0)
			//if (binImage.at<uchar>(j, i) != 0)
				counts[i]++;
		}
	}
	return counts;
}


/// <summary>
/// 이진 이미지의 Horizontal projection 데이터로 변환 
/// </summary>
/// <param name="binImage">변환할 이진 이미지.</param>
/// <returns>Horizontal projection 데이터</returns>
vector<int> analyzer::getHorizontalProjectionData(Mat binImage)
{
	int nRows = binImage.rows;
	int nCols = binImage.cols;

	vector<int> counts;
	for (int i = 0; i < binImage.rows; i++)	// inital
		counts.push_back(0);

	for (int j = 0; j < nCols; j++) {
		for (int i = 0; i < nRows; i++) {
			if (binImage.at<uchar>(i, j) != 0)
				counts[i]++;
		}
	}
	return counts;
}

/// <summary>
/// HistogramData를 이미지 파일로 변경
/// </summary>
/// <param name="histogramData">히스토그램 데이터.</param>
/// <param name="threshold">히스토그램 데이터에서 의미있다고 판단할 문턱값.</param>
/// <returns>히스토그램 이미지</returns>
Mat analyzer::getChangeHistorgramMat(vector<vector<int>> histogramData, int threshold)
{
	vector<vector<bool>> data = getChangeHistorgramData(histogramData, threshold);
	Mat outImage = vectorToBinaryMat(data);

	return outImage;
}

/// <summary>
/// Vertical Projection 데이터들을 histogram data로 변경
/// </summary>
/// <param name="histogramData">프로젝션 데이터들.</param>
/// <param name="threshold">The히스토그램 데이터에서 의미있다고 판단할 문턱값.</param>
/// <returns>이진화 된 히스토그램 데이터</returns>
vector<vector<bool>> analyzer::getChangeHistorgramData(vector<vector<int>> histogramData, int threshold)
{
	vector<vector<bool>> changeStatus;

	int imageWidth = histogramData.at(0).size();
	vector<bool> defaultRow(imageWidth, false);
	changeStatus.push_back(defaultRow);

	for (int i = 1; i < histogramData.size(); i++)
	{
		vector<bool> row = defaultRow;
		for (int cols = 0; cols < imageWidth; cols++)
		{
			int changeValue = histogramData[i][cols] - histogramData[i - 1][cols];
			changeValue = abs(changeValue);
			if (changeValue >= threshold)
				row.at(cols) = true;
			else
				row.at(cols) = false;
		}
		
		changeStatus.push_back(row);
	}
	return 	changeStatus;
}

/// <summary>
/// 이진 히스토그램 데이터 분포의 평균의 배열을 반환.
/// </summary>
/// <param name="histogramData">이진 히스토그램 데이터.</param>
/// <returns>이진 히스토그램 데이터 분포의 평균 배열</returns>
vector<int> analyzer::getVerticalHistogramAverageData(vector<vector<bool>> histogramData)
{
	vector<int> vecAverage;
	int framwWidth = (int)histogramData.at(0).size();
	int frameCounts = (int)histogramData.size();

	// input 한 row에서 White인 cols들의 평균 지점.
	for (int i = 0; i < frameCounts; i++)
	{
		int sumOfCol = 0;
		int sumCount = 0;
		for (int cols = 0; cols < framwWidth; cols++)
		{
			if(histogramData[i][cols]== true)
			{
				sumOfCol += cols;
				sumCount++;
			}
		}
		if (sumCount == 0)
			vecAverage.push_back(0);
		else
			vecAverage.push_back(sumOfCol / sumCount);
	}

	return vecAverage;
}

/// <summary>
/// ChangeHistogram 이미지의 평균에 대한 이미지 반환.
/// </summary>
/// <param name="changeHistogramMat">The change histogram mat.</param>
/// <returns></returns>
Mat analyzer::getVerticalHistogramAverageMat(Mat changeHistogramMat)
{
	Mat weightMat;
	int imageWidth = changeHistogramMat.cols;
	vector<int> weightOfCols;
	Mat outImage = Mat::ones(1, imageWidth, CV_8U);	// row, col

	weightOfCols.push_back(0);
	Mat row = Mat::zeros(1, imageWidth, CV_8U);	// to add 
	outImage.push_back(row);
	
	// input 이미지의 한 row에서 White인 cols들의 평균에 점찍음.
	for (int i = 1; i < changeHistogramMat.rows; i++)
	{
		int sumOfCol = 0;
		int sumCount = 0;
		for (int cols = 0; cols < imageWidth; cols++)
		{
			if (changeHistogramMat.at<uchar>(i, cols) == 255)
			{
				sumOfCol += cols;
				sumCount++;
			}
		}
		if(sumCount==0)
			weightOfCols.push_back(0);
		else
			weightOfCols.push_back(sumOfCol/sumCount);
	}

	for (int i = 1; i < weightOfCols.size(); i++)
	{
		row = Mat::zeros(1, imageWidth, CV_8U);
		row.at<uchar>(0, weightOfCols[i]) = 255;
		row.at<uchar>(0, imageWidth/2) = 255;	// 가운데 줄긋기
		outImage.push_back(row);
	}

	string fileName = "test.txt";
	fileManager::writeVector(fileName, weightOfCols);

	return outImage;
}

/// <summary>
/// bool 타입의 2차원 배열을 Mat으로 바꿈
/// </summary>
/// <param name="vectorData">2차원 vector 데이터.</param>
/// <returns></returns>
Mat analyzer::vectorToBinaryMat(vector<vector<bool>> vectorData)
{
	int imageWidth = (int)vectorData.at(0).size();
	Mat outImage = Mat::ones(1, imageWidth, CV_8U);	// row, col

	Mat row = Mat::zeros(1, imageWidth, CV_8U);	// to add 
	outImage.push_back(row);

	for (int i = 1; i < vectorData.size(); i++)
	{
		row = Mat::zeros(1, imageWidth, CV_8U);	// to add 
		for (int cols = 0; cols < imageWidth; cols++)
		{
			if (vectorData[i][cols] == true)
				row.at<uchar>(0, cols) = 255;
			else
				row.at<uchar>(0, cols) = 0;
		}

		outImage.push_back(row);
	}

	return outImage;
}

/// <summary>
/// 평균점 백터를 히스토그램 이미지로 변환
/// </summary>
/// <param name="vectorData">평균점 데이터.</param>
/// <param name="imageWidth">만들 이미지의 넓이.</param>
/// <returns>평균점 히스토그램 이미지</returns>
Mat analyzer::averageVectorToBinaryMat(vector<int> vectorData, int imageWidth)
{
	Mat outImage = Mat::zeros(vectorData.size(), imageWidth, (int)CV_8U);	// row, col

	for (int i = 1; i < vectorData.size(); i++)
	{
		if (vectorData[i] > imageWidth)
			vectorData[i] = 0;
		outImage.at<uchar>(i, vectorData[i]) = 255;
		outImage.at<uchar>(i, imageWidth / 2) = 255;	// 가운데 줄긋기
	}

	return outImage;
}


/// <summary>
/// 백터에서 타겟점 주변의 effectiveRange만큼의 값들의 평균 값 반환
/// </summary>
/// <param name="vec">The vec.</param>
/// <param name="target">The target.</param>
/// <param name="effectiveRange">The effective range.</param>
/// <param name="includeZero">if set to <c>true</c> [include zero].</param>
/// <returns></returns>
float analyzer::getAverageOnVectorTarget(vector<int> vec, int target, int effectiveRange, bool includeZero)
{
	int calCount = 0;
	int sum = 0;
	for (int i = target - effectiveRange; i <= target + effectiveRange; i++)
	{
		if (i < 0 || i>=vec.size())
			continue;
		else if (vec[i] == 0 && includeZero == false)
			continue;
		else
		{
			sum += vec[i];
			calCount++;
		}
	}

	if (calCount == 0)
		return 0;
		
	return sum/(float)calCount;
}

/// <summary>
/// Line들의 시작점, 끝점의 이미지 + 끝점의 binary 이미지를 저장함.
/// </summary>
/// <param name="lines">라인들(시작점 프레임, 끝점 프레임).</param>
/// <param name="videoPath">비디오 경로.</param>
void analyzer::captureLines(vector<pair<int, int>> lines, string videoPath)
{
	for (int i = 0; i < lines.size(); i++)
	{
		Mat startImage, endImage;
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)lines[i].first);
		videoCapture->read(startImage);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)lines[i].second);
		videoCapture->read(endImage);
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_Start.jpg", startImage);
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_End.jpg", endImage);

		// make sub bin Image 
		Mat subBinImage = ImageToSubBinImage(endImage);
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_Bin.jpg", subBinImage);

		//Mat subImage = getSubtitleImage(endImage);
		//Mat binImage = getCompositeBinaryImages(subImage);
		//imwrite(videoPath + "/Captures/Line" + to_string(i) + "_Bin.jpg", binImage);

	}
}

/// <summary>
/// TargetImage에서 subtitle부분을 자르고 흑백연산 + 노이즈 제거 의 결과 이미지 반환.
/// </summary>
/// <param name="targetImage">The target image.</param>
/// <returns></returns>
Mat analyzer::ImageToSubBinImage(Mat targetImage)
{
	Mat subImage = imageHandler::getSubtitleImage(targetImage);
	Mat binCompositeImage = imageHandler::getCompositeBinaryImages(subImage);

	Mat image_gray;
	cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
	Mat binATImage;
	adaptiveThreshold(image_gray, binATImage, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);
	
	Mat image_floodFilled_AT = imageHandler::getFloodProcessedImage(binATImage.clone(), true);
	Mat image_floodFilled_AT2 = imageHandler::getFloodProcessedImage(image_floodFilled_AT.clone(), false);
	Mat image_floodFilled_AT2_Not;
	bitwise_not(image_floodFilled_AT2, image_floodFilled_AT2_Not);

	Mat image_out;
	image_out = getBinImageByFloodfillAlgorism(image_floodFilled_AT2_Not, binCompositeImage);
	
	return image_out;
}

/// <summary>
/// ATImage에 compositeImage의 흰색점인 좌표에 floodfill 연산을 하고 나온 결과물을 반환
/// </summary>
/// <param name="ATImage">AdoptedThresold()의 결과이미지.</param>
/// <param name="compositeImage"> ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV))의 결과이미지.</param>
/// <returns></returns>
Mat analyzer::getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage)
{
	Mat filteredImage_BGR = ATImage.clone();
	cvtColor(filteredImage_BGR, filteredImage_BGR, COLOR_GRAY2BGR);

	// 1. outimage에 mergedImage에 흰색인 점 좌표에 빨간색으로 floodfill() 수행 
	int height = filteredImage_BGR.rows;
	int width = filteredImage_BGR.cols;
	Vec3b whiteColor = { 255, 255, 255 };
	Vec3b redColor = { 0, 0, 255 };
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Vec3b color = filteredImage_BGR.at<Vec3b>(Point(x, y));
			if (compositeImage.at<uchar>(Point(x, y)) == 255 && color == whiteColor)	// 좌표색이 흰색이면
				floodFill(filteredImage_BGR, Point(x, y), redColor);
		}
	}
	// 2. 빨간색으로 inRange() 하여 다시 흰색 이미지를 얻음
	Mat filteredImageToWhite;
	inRange(filteredImage_BGR, Scalar(0, 0, 254), Scalar(0, 0, 255), filteredImageToWhite);	// binarize by rgb

	return filteredImageToWhite;
}


/// <summary>
/// binary 이미지를 텍스트 파일로 바꿈.
/// </summary>
/// <param name="lineSize">Size of the line.</param>
/// <param name="videoPath">The video path.</param>
void analyzer::capturedLinesToText(int lineSize, string videoPath)
{
	// "Output/Captures/LineX_bin.jpg"
	for (int i = 0; i < lineSize; i++)
	{
		string targetPath = videoPath + "/Captures/Line" + to_string(i) + "_Bin.jpg";	// ./Output/Captures/
		if (fileManager::isExist(targetPath) != true)
		{
			printf("%s is not exist. \r\n", targetPath.c_str());
			continue;
		}

		string desName = videoPath + "/Lines/Line" + to_string(i);
		runOCR(targetPath, desName);
	}
}

/// <summary>
/// OCR 수행함
///  Command line 예 : tesseract/tesseract_5.0.exe Output/movie.mp4/Captures/Line18_bin.jpg 0 -L tha+eng --oem 1 --psm 7 -c tessedit_char_blacklist=ABCDE
///  Options :
///  OCR Engine modes(–oem) :
///  	0 - Legacy engine only.
///  	1 - Neural nets LSTM engine only.
///  	2 - Legacy + LSTM engines.
///  	3 - Default, based on what is available.
///  Page segmentation modes(–psm) :
///  	0 - Orientation and script detection(OSD) only.
///  	1 - Automatic page segmentation with OSD.
///  	2 - Automatic page segmentation, but no OSD, or OCR.
///  	3 - Fully automatic page segmentation, but no OSD. (Default)
///  	4 - Assume a single column of text of variable sizes.
///  	5 - Assume a single uniform block of vertically aligned text.
///  	6 - Assume a single uniform block of text.
///  	7 - Treat the image as a single text line.
///  	8 - Treat the image as a single word.
///  	9 - Treat the image as a single word in a circle.
///  	10 - Treat the image as a single character.
///  	11 - Sparse text.Find as much text as possible in no particular order.
///  	12 - Sparse text with OSD.
///  	13 - Raw line.Treat the image as a single text line, bypassing hacks that are Tesseract - specific.
/// </summary>
/// <param name="targetImage">OCR에 입력될 이미지파일 경로.</param>
/// <param name="outFileName">출력 OCR 파일 경로.</param>
void analyzer::runOCR(string targetImage, string outFileName)
{
	// $tesseract o.jpg o.out -l kor
	// 인자 : "인풋이미지" + "아웃.txt경로" + "-l tha+eng"
	string procName = "tesseract_5.0.exe";		// tesseract 경로
	string options = " -l tha+eng --oem 1 --psm 7";
	string tessdataPath = " --tessdata-dir tessdata";
	string blackList = " -c tessedit_char_blacklist=\"|:;\" ";
	string commandString = procName + " " + targetImage + " " + outFileName + options + blackList;
	wstring args = s2ws(commandString);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HANDLE H;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));	// 프로그램 매모리 할당

	printf("runOCR : %s  \r\n", commandString.c_str());

	if (!CreateProcess(NULL, (LPWSTR)args.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		fprintf(stderr, "Fail. \r\n");
		return;
	}
	H = pi.hProcess;
	WaitForMultipleObjects(1, &H, true, INFINITE);	// 모든 child가 완료될때까지 대기
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	//ExitProcess(0);
}

wstring analyzer::s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

/// <summary>
/// line의 text 파일을 종합하여 .lrc 포맷의 파일로 반환.
/// </summary>
/// <param name="lines">The lines.</param>
/// <param name="videoPath">The video path.</param>
void analyzer::makeLyrics(vector<pair<int, int>> lines, string videoPath)
{
	vector<string> vecLyricLine;
	for (int i = 0; i < lines.size(); i++)
	{
		string lineFileName = videoPath + "/Lines/Line" + to_string(i) + ".txt";
		if (fileManager::isExist(lineFileName) != true)
		{
			printf("%s is not exist. \r\n", lineFileName.c_str());
			continue;
		}
		string line;
		fileManager::readLine(lineFileName, line);

		String startTime = videoHandler::frameToTime(lines[i].first, *videoCapture);
		String endTime = videoHandler::frameToTime(lines[i].second, *videoCapture);

		//string lyricLine = "[" + to_string(lines[i].first) + "]\t" + line + "\t[" + to_string(lines[i].second) + "]";
		string lyricLine = "[" + startTime + "]\t" + line + "\t[" + endTime + "]";

		printf("Line%d: %s\r\n", i, lyricLine.c_str());

		vecLyricLine.push_back(lyricLine);
	}
	string filename = "Lyrics.txt";
	fileManager::writeVector(filename, vecLyricLine);
}

bool analyzer::setVideo(string videoPath)
{
	videoCapture = new VideoCapture(videoPath);
	if (!videoCapture->isOpened())
	{
		cout << "fail to open the video" << endl;
		return false;
	}
	video_Frame = (int)videoCapture->get(CAP_PROP_FRAME_COUNT);
	video_Width = (int)videoCapture->get(CAP_PROP_FRAME_WIDTH);
	video_Height = (int)videoCapture->get(CAP_PROP_FRAME_HEIGHT);

	return true;
}

void analyzer::closeVideo()
{
	if(videoCapture->isOpened())
		videoCapture->release();

	delete videoCapture;
}



