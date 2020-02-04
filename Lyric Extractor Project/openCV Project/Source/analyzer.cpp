#include "analyzer.h"
#include "testClass.h"

void analyzer::initVariables()
{
	m_lyric.init();
}

int analyzer::getContourCount(Mat cannyImage)
{
	vector<vector<Point>> contours;
	findContours(cannyImage, contours, RETR_TREE, CHAIN_APPROX_SIMPLE); // Image에서 Contour를 찾음

	return contours.size();
}

bool analyzer::videoAnalization(string videoPath)
{
	if (!setVideo(videoPath))
	{
		return false;
	}
	
	initVariables();

	Mat orgImage;
	Mat subImage;
	Mat binImage;
	Mat beforeBinImage;	// 이전 바이너리 이미지
	vector<int> vecWhitePixelCounts;			// 프래임 별 흰색 개수
	vector<int> vecWhitePixelChangedCounts;		// 이전 프래임 대비 흰색 변화량
	vector<vector<int>> verticalProjectionDatasOfDifferenceImage;	// 디프런트 이미지의 흰색 vertical Projection데이터의 모임

	vecWhitePixelCounts.push_back(0);				// 프래임과 동기화 위해 배열 0번은 더미
	vecWhitePixelChangedCounts.push_back(0);		// 프래임과 동기화 위해 배열 0번은 더미
	//verticalProjectionDatasOfDifferenceImage.push_back( vector<int>(videoCapture->get(CAP_PROP_FRAME_WIDTH)) );	// 프래임과 동기화 위해 배열 0번은 더미

	int curFrame = 0;
	/* 이미지 분석 */
	while (videoCapture->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*videoCapture);
		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

		if (orgImage.rows != 720)
			orgImage = imageHandler::resizeImageToAnalize(orgImage);

		subImage = imageHandler::getSubtitleImage(orgImage);

		//binImage = imageHandler::getCompositeBinaryImages(subImage);	// 파빨간색으로 

		Mat fullContrastimage_NotBulr;
		fullContrastimage_NotBulr = imageHandler::getFullyContrastImage(subImage);
		binImage = imageHandler::getPaintedBinImage(fullContrastimage_NotBulr);

		//White dot Count
		vecWhitePixelCounts.push_back(imageHandler::getWihtePixelCount(binImage));

		if (beforeBinImage.empty() == true)
		{
			verticalProjectionDatasOfDifferenceImage.push_back(vector<int>(binImage.cols));	// 프래임과 동기화 위해 배열 0번은 더미
			verticalProjectionDatasOfDifferenceImage.push_back(imageHandler::getVerticalProjectionData(binImage));	// insert zero row

			vecWhitePixelChangedCounts.push_back(0);
		}
		else
		{
			Mat DifferenceImage = imageHandler::getDifferenceImage(binImage, beforeBinImage);
			verticalProjectionDatasOfDifferenceImage.push_back(imageHandler::getVerticalProjectionData(DifferenceImage) ); // 현 프레임의 vertical 프로젝션

			vecWhitePixelChangedCounts.push_back(imageHandler::getWihtePixelCount(DifferenceImage));
		}

		beforeBinImage = binImage;

	}	// end while

	fileManager::initDirectory(videoPath);

	string fileName = "WhitePixelCount.txt";
	fileManager::writeVector(fileName, vecWhitePixelCounts);
	fileName = "WhitePixelChangedCount.txt";
	fileManager::writeVector(fileName, vecWhitePixelChangedCounts);


	Mat changeHistogramMat; // 체인지 히스토그램	//changeHistogram = getChangeHistorgramMat(verticalProjectionDatasOfDifferenceImage, 10);//= Mat::ones(orgImage.cols, 0, CV_64F);	// col, row
	vector<vector<bool>> changeHistorgramData = getChangeHistorgramData(verticalProjectionDatasOfDifferenceImage, 10);
	changeHistogramMat = vectorToBinaryMat(changeHistorgramData);
	imwrite(fileManager::getSavePath() + "ChangeHistogram.jpg", changeHistogramMat);

	vector<int> verticalHistogramAverage = getVerticalHistogramAverageData(changeHistorgramData);	// getChangeStatusAverage -> getChangeAverageHistogramData
	fileName = "WhitePixelChangedCountAverage.txt";
	fileManager::writeVector(fileName, verticalHistogramAverage); 

	Mat changeHistogramAverageMat = averageVectorToBinaryMat(verticalHistogramAverage, changeHistogramMat.cols); // 체인지 히스토그램의 평균점 이미지
	imwrite(fileManager::getSavePath() + "changeHistogramAverage.jpg", changeHistogramAverageMat);
	
	/* 1. 라인 확정 */	// 여기서는 유효한 라인만 걸러냄 (start-end frame은 대략적인부분으로)
	getJudgedLine(vecWhitePixelCounts, verticalHistogramAverage);

	/* 2. 라인 보정 */ // 라인의 정확한 시작-끝 시간을 맞춤
	calibrateLines();

	// 단어 나누기 wordJudge();
	wordJudge();
	
	/* Save pictures */
	captureLines(fileManager::getSavePath());
	capturedLinesToText(fileManager::getSavePath());

	makeLyrics(fileManager::getSavePath());
	makeLyrics_withWord();

	closeVideo();
	return true;
}

bool analyzer::videoAnalization1(string videoPath)
{
	if (!setVideo(videoPath))
	{
		return false;
	}

	Mat orgImage, subImage;
	Mat paintedBinImage;
	vector<int> vecPaintedPixelCounts;		// 흰점수
	// 체인지 영역 

	vecPaintedPixelCounts.push_back(0);

	while (videoCapture->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*videoCapture);

		if (orgImage.rows != 720)
			orgImage = imageHandler::resizeImageToAnalize(orgImage);

		subImage = imageHandler::getSubtitleImage(orgImage);
		

		paintedBinImage = getPaintedBinImage(subImage);
		vecPaintedPixelCounts.push_back(imageHandler::getWihtePixelCount(paintedBinImage));	// 픽셀 수 구함

	}

	string fileName = "PaintedPixelCount.txt";
	fileManager::writeVector(fileName, vecPaintedPixelCounts);

	return true;
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

/// <summary>
/// 분석한 데이터로 가사 Line들의 시작점-끝점을 판단함
/// </summary>
/// <param name="vecWhitePixelCounts"> 프레임에 존재하는 흰 점의 합 </param>
/// <param name="changeStatusAverage"> 변한 흰색의 평균 위치</param>
void analyzer::getJudgedLine(vector<int> vecWhitePixelCounts, const vector<int> verticalHistogramAverage)
{
	vector<int> peakValues;

	// Peak 추출
	peakValues = getPeakFromWhitePixelCounts(vecWhitePixelCounts);

	for (int i = 0; i < peakValues.size(); i++)
		printf("Peak %2d : %d\r\n", i, peakValues[i]);

	string fileName = "peak.txt";
	fileManager::writeVector(fileName, peakValues);

	getLinesFromPeak(peakValues, vecWhitePixelCounts);

	linesRejudgeByLineLength();
	lineRejudgeByPixelCount(vecWhitePixelCounts);
	// lineRejudgeByVerticalHistogramAverage(lines, verticalHistogramAverage);	// 왼쪽의 점 좌표 평균, 오른쪽의 점 좌표평균

	vector<string> lines_string;
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		string line_string = to_string(line->startFrame) + "\t" + to_string(line->endFrame);
		lines_string.push_back(line_string);// ("lines %2d : %d - %d\r\n", i, lines[i].first, lines[i].second);
	}

	fileName = "lineStart_End.txt";
	fileManager::writeVector(fileName, lines_string);
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
	//for (int frameNum = 0; frameNum < vecWhitePixelCounts.size(); frameNum++)
	//{
	//	if (vecWhitePixelCounts[frameNum] > 500)	// 노이즈 레벨
	//	{
	//		bool isPeak = true;
	//		bool isHaveDrop = false;
	//		for (int checkRange = (frameNum - DEFAULT_FPS / 2); checkRange < frameNum + DEFAULT_FPS / 2; checkRange++)	// x-12
	//		{
	//			if (vecWhitePixelCounts.size() <= checkRange)
	//				break;

	//			if (frameNum == checkRange)
	//				continue;

	//			if (checkRange<0 || checkRange>vecWhitePixelCounts.size())
	//			{
	//				isPeak = false;
	//				break;	// 시작점, 끝점은 안봄
	//			}

	//			//  조건1. 자신 주변에 자신 이상의 값을 가진것이 없슴.
	//			if (vecWhitePixelCounts[checkRange] > vecWhitePixelCounts[frameNum])
	//			{
	//				isPeak = false;
	//				break;
	//			}
	//		}
	//		for (int checkRange = frameNum; checkRange < frameNum + DEFAULT_FPS * 2; checkRange++)	// 2Sec (fade-out 까지 포함하는 초)
	//		{
	//			if (vecWhitePixelCounts.size() <= checkRange)
	//				break;
	//			// 자신보다 뒤쪽에 자신의 70% 하락한 값보다 작은 값이 있어야 함 - 이거..
	//			if (checkRange > frameNum)
	//			{
	//				if (vecWhitePixelCounts[checkRange] < (vecWhitePixelCounts[frameNum] / 10) * 7)
	//					isHaveDrop = true;
	//			}
	//		}

	//		if (isPeak && isHaveDrop)
	//		{
	//			int index = 0;
	//			while (true)
	//			{	
	//				if (vecWhitePixelCounts[frameNum] < vecWhitePixelCounts[frameNum + 1] + 200)	// 이전값과 별 차이 안나면 가장 나중값으로 정함
	//					frameNum++;
	//				else
	//					break;
	//			}

	//			if (!peakValues.empty())
	//			{
	//				if (peakValues.back() == frameNum)
	//					peakValues.back() = frameNum;	// update
	//				else
	//					peakValues.push_back(frameNum);
	//			}
	//			else
	//				peakValues.push_back(frameNum);
	//		}
	//	}

	//	// (x-500)<0 인 구간을 구함
	//	// max값 index -> peak
	//	//max_element(vecWhitePixelCounts.begin()+5, vecWhitePixelCounts.end());

	//	
	//}

	bool zeroZone = false;
	int notZeroStart = 0;
	int maxValueIndex = 0;

	for (int frameNum = 0; frameNum < vecWhitePixelCounts.size(); frameNum++)
	{
		if (zeroZone==false )
		{
			if (vecWhitePixelCounts[frameNum] < 500)
			{
				peakValues.push_back(maxValueIndex);	// 최대값 저장
				zeroZone = true;
				maxValueIndex = 0;
			}
			if (vecWhitePixelCounts[maxValueIndex] < vecWhitePixelCounts[frameNum])
			{
				maxValueIndex = frameNum;
			}
		}
		else // zeroZone
		{
			if (vecWhitePixelCounts[frameNum] >= 500)
			{
				zeroZone = false;
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
void analyzer::getLinesFromPeak(vector<int> peaks, vector<int> vecWhitePixelCounts)
{
	for (int i = 0; i < peaks.size(); i++)	// find start point
	{
		// 이전peak값 index보다 큼
		int minRange;
		if (i == 0)
			minRange = 0;
		else
			minRange = peaks[i - 1];

		int minIndex = minRange;
		for (int index = minRange; index < peaks[i]; index++)	// 이전 피크부터 현재 피크까지의 값중 가장 흰점이 작은 점 중 가장 뒤에 있는 곳.
		{
			if (vecWhitePixelCounts[minIndex] >= vecWhitePixelCounts[index])	// 가장 마지막 0이 됨
			{
				minIndex = index;
			}
		}
		
		// new code
		Line line;
		line.startFrame = minIndex;
		line.endFrame = peaks[i];
		m_lyric.addLine(line);
	}
}

/// <summary>
/// 가사 라인들 중 특정 프래임동안 지속되지 않는 line을 걸러냄
/// </summary>
/// <param name="fps">The FPS.</param>
void analyzer::linesRejudgeByLineLength(int fps)
{
	for (int i =0; i<m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		printf("lines (%d - %d )\r\n", line->startFrame, line->endFrame);

		if (lineRejudgeByLineLength(line->startFrame, line->endFrame, fps) == false)
		{
			printf("exceptionLine: (%d - %d)\r\n", line->startFrame, line->endFrame);
			line->isValid = false;
		}
		else
			line->isValid = true;
	}
	m_lyric.removeInvalidLines();
}

bool analyzer::lineRejudgeByLineLength(int startFrame, int endFrame, int fps)
{
	const int limitMSec = 400;	// 400ms = 10frame
	int limitFrame = limitMSec / (1000 / fps);

	int lineLenght = endFrame - startFrame;

	if (lineLenght < limitFrame)
	{
		printf("Line Lenghth is short: %d (Limit:%d)\r\n", lineLenght, limitFrame);
		return false;
	}

	return true;
}

void analyzer::lineRejudgeByPixelCount(vector<int> vecWhitePixelCounts)
{
	printf("@@lineRejudgeByPixelCount_ \r\n");
	int lineCount = 0;

	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		float pixelRatio = vecWhitePixelCounts[line->startFrame] / (float)vecWhitePixelCounts[line->endFrame];

		printf("lines (%d - %d) pixelRatio: %.2f (%d/%f) \r\n", line->startFrame, line->endFrame, pixelRatio, vecWhitePixelCounts[line->startFrame], (float)vecWhitePixelCounts[line->endFrame]);
		if (0.5 < pixelRatio)	// startframe픽셀값 / endframe픽셀값 이 0.5보다 높다면 라인이 아닌걸로 판단.
		{
			line->isValid = false;
			printf("lines (%d - %d) invaild \r\n", line->startFrame, line->endFrame);
		}
		else
		{
			line->isValid = true;
		}
	}
	m_lyric.removeInvalidLines();

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
			printf("exceptionLine: %d(div by zero)\r\n", lineCount);
			it = judgedLines.erase(it);
			lineCount++;
			continue;
		}

		avgLeft = avgLeft / avgLeftCount;
		avgRight = avgRight / avgRightCount;
		float ratio = avgLeft / (float)avgRight;
		printf("lines %2d : Left:%d  Right:%d \t", lineCount, avgLeft, avgRight);
		printf("(ratio : %3.2f)\r\n", ratio);

		if (ratio > 0.85)
		{
			printf("exceptionLine : %d\r\n", lineCount);
			it = judgedLines.erase(it);
		}
		else
			++it;

		lineCount++;
	}
}

void analyzer::calibrateLines()
{
	Mat maskImage;
	int invaildCount = 0;

	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		printf("Cal Line%d : %d - %d\r\n", i, line->startFrame, line->endFrame);
		if (lineCalibration(line->startFrame, line->endFrame, maskImage) == false)
		{
			line->isValid = false;
			printf("Caled Line%d is invaild will be removed.\r\n", i);
			invaildCount++;
		}
		else
		{
			line->isValid = true;
			printf("Caled Line%d : %d - %d\r\n", i, line->startFrame, line->endFrame);

			/* 라인 시작점으로 마스크 얻음*/
			//Mat newMaskImage = getSharpenAndContrastImage(line->startFrame);	// 1. 라인 시작지점으로 마스크 얻음
			//					// 1-1. get ContrastImage
			//Mat newMaskImage_hsv, newMaskImage_gray;
			//cvtColor(newMaskImage, newMaskImage_hsv, COLOR_BGR2HSV);
			//cvtColor(newMaskImage, newMaskImage_gray, COLOR_BGR2GRAY);
			//Mat newMaskImage_bin;
			//inRange(newMaskImage_hsv, Scalar(0, 0, 254), Scalar(255, 255, 255), newMaskImage_bin);	// 1-2. ContrastImage->hsv filtering
			//// 부족하면 이전or다음 프레임 동원하여 해결할것.
			//newMaskImage_bin = removeLint(newMaskImage_bin, newMaskImage_gray);	// 튀어나온것 제거

			//newMaskImage = getBinImageByFloodfillAlgorism(newMaskImage_bin, maskImage);

			////Mat mofImag = imageHandler::getMorphImage(newMaskImage, MorphTypes::MORPH_DILATE);	// as

			//line->maskImage = newMaskImage.clone();
			//catpureBinaryImageForOCR(newMaskImage, i-invaildCount, fileManager::getSavePath());

			/* 라인 끝점으로 마스크 얻음*/
			line->maskImage = maskImage.clone();
			catpureBinaryImageForOCR(maskImage, i - invaildCount, fileManager::getSavePath());
		}
	}
	m_lyric.removeInvalidLines();
	   	
	linesRejudgeByLineLength();

}

/// <summary>
/// 0. mask값에서 최좌측, 최우측 x 좌표 구함(기준으로 함)
/// while()
/// 1. 이전 프레임 저장
/// 2. Diff Image 생성 (현재프래임binImage-이전프래임binImage)
/// 3. Diff Image에서 흰색점의 평균 x좌표 구함	-- 
/// 4. 평균 x변경점이 mask의 최좌측 과 최우측 기준으로 몇 % 쯤에 속하는지 구함(가장 높은 퍼센테이지를 갖는 곳이 끝점, whiteCount가 가장작은 부분이 시작점)
/// </summary>
/// <param name="startFrame">The start frame.</param>
/// <param name="endFrame">The end frame.</param>
bool analyzer::lineCalibration(int& startFrame, int& endFrame, Mat& maskImage)
{
	bool isEffictiveLine = true;
	Mat readImage;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)endFrame-1);
	videoCapture->read(readImage);
	if (readImage.rows != 720)
		readImage = imageHandler::resizeImageToAnalize(readImage);
	maskImage = imageToSubBinImage(readImage);

	int maskImage_leftDot_x = getLeftistWhitePixel_x(maskImage);
	int maskImage_rightDot_x = getRightistWhitePixel_x(maskImage);
	int maskImage_middleDot_x = (maskImage_leftDot_x + maskImage_rightDot_x) / 2;
	int image_center_x = readImage.cols / 2;

	// 1. 마스크 노이즈 제거 
	if ( (maskImage_middleDot_x > (image_center_x)+10) || (maskImage_middleDot_x < (image_center_x)-10) )
	{	// 두 지점(가장 왼, 가장 오른쪽) 중 중앙에 가까운 쪽을 기준으로 대칭하여 자름 - 왼, 오른쪽 멀리 떨어져 있는 노이즈 제거
		int lengthOfLine;
		int leftLength = image_center_x - maskImage_leftDot_x;
		int rightLength = maskImage_rightDot_x - image_center_x;
		Mat lyricMask;
		if (leftLength > rightLength)
		{
			lyricMask = getLyricMask(maskImage.clone(), image_center_x - (rightLength + 10), image_center_x + (rightLength + 10));// middle - (rightLength +30) ~ middle + (rightLength + 30) == 이미지 범위
		}
		else
		{
			lyricMask = getLyricMask(maskImage.clone(), image_center_x-(leftLength+10), image_center_x +(leftLength+10));// middle - (leftLength +30) ~ middle + (leftLength + 30) == 이미지 범위
		}
		maskImage = getBinImageByFloodfillAlgorism(maskImage, lyricMask);

		maskImage_leftDot_x = getLeftistWhitePixel_x(maskImage);
		maskImage_rightDot_x = getRightistWhitePixel_x(maskImage);
		maskImage_middleDot_x = (maskImage_leftDot_x + maskImage_rightDot_x) / 2;
	}

	int maskImage_pixelCount = imageHandler::getWihtePixelCount(maskImage);
	// 2. 마스크 유효성 판단 - 흰점 500이하이면 탈락
	printf("MaskImage Info - left: %d right: %d middle: %d(%d) \r\n", maskImage_leftDot_x, maskImage_rightDot_x, maskImage_middleDot_x, image_center_x);
	printf("MaskImage Info - whiteCount: %d \r\n", maskImage_pixelCount);
	if (maskImage_pixelCount < 500)
	{
		isEffictiveLine = false;
		printf("line Exception : MaskImage whiteCount is 500 Under.\r\n");
		return isEffictiveLine;
	}


	Mat subImage;
	Mat binImage, beforeBinImage;
	int beforePixelCount=0;
	int diffImage_avgPoint_PerMax = 0;	
	int MinimumPixelCount = 500;	// 임의 초기값
	int startframe_org = startFrame;

	//bool additionalNoiseRemovedAtPer[10] = { false, };	// 진행 구간별로 노이즈 제거하기 위한 알고리즘
	//additionalNoiseRemovedAtPer[9] = true;	// 100 이하에서 한번 진행
	//additionalNoiseRemovedAtPer[8] = true;	// 90 이하에서 한번 진행
	//additionalNoiseRemovedAtPer[7] = true;	// 80 이하에서 한번 진행
	//additionalNoiseRemovedAtPer[6] = true;	// 70 이하에서 한번 진행
	//additionalNoiseRemovedAtPer[5] = true;	// 60 이하에서 한번 진행

	int frameIndex = endFrame;
	int expectedAvgPoint = maskImage_rightDot_x;
	// 3. 진행도 측정 -> 라인의 시작과 끝점 파악
	while (true)	
	{
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)frameIndex-1);	// frameIndex-1
		videoCapture->read(readImage);

		if (readImage.rows != 720)
			readImage = imageHandler::resizeImageToAnalize(readImage);

		subImage = imageHandler::getSubtitleImage(readImage);

		Mat image_binIR_RGB_R;
		inRange(subImage, Scalar(0, 0, 130), Scalar(50, 50, 255), image_binIR_RGB_R);	// binarize by rgb
		Mat image_binIR_RGB_B;
		inRange(subImage, Scalar(140, 0, 0), Scalar(255, 40, 50), image_binIR_RGB_B);	// binarize by rgb

		bitwise_or(image_binIR_RGB_B, image_binIR_RGB_R, binImage, maskImage);

		if (beforeBinImage.empty())
			beforeBinImage = binImage;

		Mat DifferenceImage = imageHandler::getDifferenceImage(binImage, beforeBinImage);

		Mat image_dilateion;
		Mat element(3, 3, CV_8U, Scalar(1));
		element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		erode(DifferenceImage, image_dilateion, element);	// 침식연산 - 노이즈제거 

		Mat avgPixelMask = imageHandler::getColumMaskImage(binImage.cols, binImage.rows, 200, expectedAvgPoint);
		bitwise_and(image_dilateion.clone(), avgPixelMask, image_dilateion);

		int pixelCount = imageHandler::getWihtePixelCount(binImage);
		int diffImage_avgPoint = getWhitePixelAverage(image_dilateion);
		int diffImage_avgPoint_Per = (int)((diffImage_avgPoint - maskImage_leftDot_x) / ( (maskImage_rightDot_x - maskImage_leftDot_x) / 100.0));

		printf("frame %d - diffAvgPoint: %d, diffImage_avgPoint_Per: %d \r\n", frameIndex, diffImage_avgPoint, diffImage_avgPoint_Per);
		printf("frame %d - pixelCount: %d \r\n", frameIndex, pixelCount);

		if (diffImage_avgPoint_PerMax <= diffImage_avgPoint_Per)	// 가장 큰 Percent 변경점이 시작점, => 마스크 가장 오른쪽 점 
		{
			endFrame = frameIndex+1;
			diffImage_avgPoint_PerMax = diffImage_avgPoint_Per;
		}

		if (MinimumPixelCount >= pixelCount)	// 가장 작은 pixel수 부분이 끝점, 
		{
			MinimumPixelCount = pixelCount;
			startFrame = frameIndex+1;
		}

		// 노이즈 제거 알고리즘
		/*	변경방향
		 - 라인 끝지점부터 시작지점까지 계속 진행 (100% 아래로 내려가면 그때부터 시작, difPoint가 양수이면 cutLineX업데이트)
		 
		 + 추가로 흰색으로 필터링 한 이미지로 floodfill 진행 ( cutLineX 우측만 검사)

		 - 
		*/
		//if (diffImage_avgPoint_Per > 0)		
		//{
		//	if (additionalNoiseRemovedAtPer[diffImage_avgPoint_Per / 10] == true)
		//	{
		//		;	// (diffImage_avgPoint_Per / 10 -1) *10 % 에 해당하는 x 축 기준으로 왼쪽만 flood fill 진행 //
		//		int cutLineX = (maskImage_rightDot_x - maskImage_leftDot_x) / 10 * ((diffImage_avgPoint_Per / 10) - 1) + maskImage_leftDot_x;	//  ( 오른쪽-왼쪽 / 10 * x ) + 왼쪽
		//		Mat postMaskImage = getBinImageByFloodfillAlgorismforNoiseRemove(maskImage, binImage, cutLineX);	// x좌표 이전까지만 검사함

		//		maskImage = postMaskImage;
		//		additionalNoiseRemovedAtPer[diffImage_avgPoint_Per / 10] = false;
		//	}
		//}
		
		beforeBinImage = binImage.clone();
		if(diffImage_avgPoint!=0)
			expectedAvgPoint = diffImage_avgPoint;

		frameIndex --;		
		//if (frameIndex < startframe_org)
		//	break;
		if (MinimumPixelCount <= 0)
			break;
	}

	// 마스크 업데이트 -> 노이즈 제거
	// start -> end 까지 pixel 각각의 누적값으로 
	// 어떤 값을 쓸것인가?
	//		1. 엣지의 누적
	//		2. RGB색의 누적 (흰, 파, 빨 전부 or한 값)
	//		3. HSV색의 누적 (흰, 파, 빨 전부 or한 값)

	if (isEffictiveLine)		// 
	{
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)startFrame - 2);	// frameIndex-1
		videoCapture->read(readImage);

		if (readImage.rows != 720)
			readImage = imageHandler::resizeImageToAnalize(readImage);

		subImage = imageHandler::getSubtitleImage(readImage);
		// 1. 흰색의 이진화한 이미지 구함
		// 2. floodfill (maskImage, 1번한것)
	// inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), image_bin_inRange);	
		inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), binImage);
		maskImage = getBinImageByFloodfillAlgorism(maskImage, binImage);
	}	// 


	// 마지막으로 유효한 라인인지 걸러냄
	maskImage_pixelCount = imageHandler::getWihtePixelCount(maskImage);
	if (maskImage_pixelCount < 500)	// 마스크 픽셀이 500개 이하
	{
		isEffictiveLine = false;
		printf("MaskImage Info - whiteCount: %d \r\n", maskImage_pixelCount);
		printf("line Exception : MaskImage whiteCount is 500 Under.\r\n");
		return isEffictiveLine;
	}

	if (lineRejudgeByLineLength(startFrame, endFrame) == false)	// 길이가 400 이하 
	{
		isEffictiveLine = false;
		printf("line Exception : LineLength is unter 400ms.\r\n");
		return isEffictiveLine;
	}
	// save 
	return isEffictiveLine;
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
/// Line들의 시작점, 끝점의 이미지 + 끝점의 이미지를 저장함.
/// </summary>
/// <param name="videoPath">비디오 경로.</param>
void analyzer::captureLines(string videoPath)
{
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		Mat startImage, endImage;
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)line->startFrame - 2);
		videoCapture->read(startImage);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)line->endFrame);	// -1
		videoCapture->read(endImage);
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_Start.jpg", startImage);
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_End.jpg", endImage);
	}
}

/// <summary>
/// 끝점의 binary 이미지를 저장함.
/// </summary>
/// <param name="lines">라인들(시작점 프레임, 끝점 프레임).</param>
/// <param name="videoPath">비디오 경로.</param>
void analyzer::catpureBinaryImageOfLinesEnd(vector<pair<int, int>> lines, string videoPath)
{
	for (int i = 0; i < lines.size(); i++)
	{
		Mat startImage, endImage;
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)lines[i].second -1);
		videoCapture->read(endImage);
		// make sub bin Image 
		Mat subBinImage = imageToSubBinImage(endImage);
		bitwise_not(subBinImage, subBinImage);
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_Bin.jpg", subBinImage);
	}
}

void analyzer::catpureBinaryImageForOCR(Mat binImage, int lineNum, string videoPath)
{


	bitwise_not(binImage, binImage);
	// resize

	//resize(binImage, binImage, cv::Size(binImage.cols * 0.6, binImage.rows * 0.6), 0, 0, cv::INTER_CUBIC);

	//resize(binImage, binImage, cv::Size(0, 0), 0.8, 0.8, cv::INTER_CUBIC);	// for text Height Size

	//threshold(binImage, binImage, 128, 255, THRESH_BINARY);

	imwrite(videoPath + "/Captures/Line" + to_string(lineNum) + "_Bin.jpg", binImage);
}

/// <summary>
/// TargetImage에서 subtitle부분을 자르고 흑백연산 + 노이즈 제거 의 결과 이미지 반환.
/// </summary>
/// <param name="targetImage">The target image.</param>
/// <returns></returns>
Mat analyzer::imageToSubBinImage(Mat targetImage)
{
	if (targetImage.rows != 720)
		targetImage = imageHandler::resizeImageToAnalize(targetImage);

	Mat subImage = imageHandler::getSubtitleImage(targetImage);
	Mat binCompositeImage = imageHandler::getCompositeBinaryImages(subImage);	

	Mat image_gray;
	cvtColor(subImage, image_gray, COLOR_BGR2GRAY);
	Mat binATImage;
	adaptiveThreshold(image_gray, binATImage, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 5);	// 11 -> 9
	
	Mat image_floodFilled_AT = imageHandler::getFloodProcessedImage(binATImage, true);				// 
	Mat image_floodFilled_AT2 = imageHandler::getFloodProcessedImage(image_floodFilled_AT, false);	//
	Mat image_floodFilled_AT2_Not;
	bitwise_not(image_floodFilled_AT2, image_floodFilled_AT2_Not);

	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색(파랑, 빨강), 
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
	inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);		//파, 빨
	//imshow("image_HSV_S", image_HSV_S);

	Mat image_out;
	//image_out = getBinImageByFloodfillAlgorism(image_floodFilled_AT2_Not, binCompositeImage);
	image_out = getBinImageByFloodfillAlgorism(subImage_hsv, binCompositeImage);

	image_out = imageHandler::getFloodProcessedImage(image_out, true);

	Mat element(5, 5, CV_8U, Scalar(1));
	element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
	//Mat image_close;
	//morphologyEx(image_getBlue, image_close, MORPH_CLOSE, element5);	// Close 연산 (침식->팽창)
	morphologyEx(image_out, image_out, MORPH_CLOSE, element);	// Open 연산  (팽창->침식)

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

Mat analyzer::getBinImageByFloodfillAlgorismforNoiseRemove(Mat ATImage, Mat compositeImage, int limitX)
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
			if (x < limitX)	// 
			{
				if (compositeImage.at<uchar>(Point(x, y)) == 255 && color == whiteColor)	// 좌표색이 흰색이면
					floodFill(filteredImage_BGR, Point(x, y), redColor);
			}
			else	// 
			{
				if (ATImage.at<uchar>(Point(x, y)) == 255 && color == whiteColor)	// 원래이미지
					floodFill(filteredImage_BGR, Point(x, y), redColor);
			}
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
void analyzer::capturedLinesToText(string videoPath)
{
	// "Output/Captures/LineX_bin.jpg"
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
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
///  Command line 예 : tesseract/tesseract_5.0.exe Output/movie.mp4/Captures/Line18_bin.jpg 0 -L tha+eng -c tessedit_char_blacklist=ABCDE--oem 1 --psm 7
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
	// tesseract_5.0.exe . . 
	// $tesseract o.jpg o.out -l kor -l tha+eng --oem 1 --psm 7 -c 
	// 인자 : "인풋이미지" + "아웃.txt경로" + "-l tha+eng"
	string procName = "tesseract_5.0.exe";		// tesseract 경로
	string options = " -l tha+eng --oem 1 --psm 7";
	string tessdataPath = " --tessdata-dir tessdata";
	string params = " -c tessedit_char_blacklist=\"|:;/\" -c preserve_interword_spaces=1  -c load_system_dawg=0 -c load_freq_dawg=0";	// 블랙리스트, 띄어쓰기 제거
	string commandString = procName + " " + targetImage + " " + outFileName + options + params;
	wstring args = stringToWstring(commandString);
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

/// <summary>
/// std::string to std::wstring 
/// </summary>
/// <param name="s">string.</param>
/// <returns>wstring</returns>
wstring analyzer::stringToWstring(const std::string& s)
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
/// <param name="videoPath">The video path.</param>
void analyzer::makeLyrics(string videoPath)
{
	vector<string> vecLyricLine;
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		string lineFileName = videoPath + "/Lines/Line" + to_string(i) + ".txt";
		if (fileManager::isExist(lineFileName) != true)
		{
			printf("%s is not exist. \r\n", lineFileName.c_str());
			continue;
		}
		string ocredText;
		fileManager::readLine(lineFileName, ocredText);
		Line* line = m_lyric.getLine(i);

		line->text = ocredText;
		line->splitLineTextToWord();

		String startTime = videoHandler::frameToTime(line->startFrame, *videoCapture);
		String endTime = videoHandler::frameToTime(line->endFrame, *videoCapture);

		//string lyricLine = "[" + to_string(lines[i].first) + "]\t" + line + "\t[" + to_string(lines[i].second) + "]";
		string lyricLine = "[" + startTime + "]\t" + line->text + "\t[" + endTime + "]";

		printf("Line%d: %s\r\n", i, lyricLine.c_str());

		vecLyricLine.push_back(lyricLine);
	}

	string filename = "Lyrics.txt";
	fileManager::writeVector(filename, vecLyricLine);
}

void analyzer::makeLyrics_withWord()
{
	vector<string> vecLyricLine;
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		for (int j = 0; j < line->words.size(); j++)
		{
			//String startTime = videoHandler::frameToTime(line->words[j].startFrame, *videoCapture);
			//String endTime = videoHandler::frameToTime(line->words[j].endFrame, *videoCapture);
			String startTime = to_string(line->words[j].startFrame);
			String endTime = to_string(line->words[j].endFrame);

			string stringLine = to_string(i) + "\t" + to_string(j) + "\t[" + startTime + "]\t[" + endTime + "]\t" + line->words[j].text;
			vecLyricLine.push_back(stringLine);
		}
	}
	string filename = "Lyrics_withWord.txt";
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
	boost::filesystem::path p(videoPath);
	fileManager::videoName = p.filename().string();

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

Mat analyzer::getLyricMask(Mat imageToCopy, int startX, int endX)
{
	int height = imageToCopy.rows;
	int width = imageToCopy.cols;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (x < startX || x > endX)
				imageToCopy.at<uchar>(y, x) = 0;
			else
				imageToCopy.at<uchar>(y, x) = 255;
		}
	}
	return imageToCopy;
}

Mat analyzer::getSharpenAndContrastImage(int frameNum)
{			// 빵구매꾸기 위해 두번 함
	Mat sourceImg;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)frameNum - 1);
	videoCapture->read(sourceImg);
	if (sourceImg.rows != 720)
		sourceImg = imageHandler::resizeImageToAnalize(sourceImg);

	sourceImg = imageHandler::getSubtitleImage(sourceImg);

	Mat sharpenImage;
	sharpenImage = getSharpenImage(sourceImg);
	Mat contrastImage;
	contrastImage = getFullyContrastImage(sharpenImage);

	Mat reTouch;
	GaussianBlur(contrastImage, reTouch, Size(3, 3), 2);
	Mat sharpenImage1;
	sharpenImage1 = getSharpenImage(reTouch);
	Mat contrastImage1;
	contrastImage1 = getFullyContrastImage(sharpenImage1);

	return contrastImage1;
}

Mat analyzer::getSharpenImage(Mat srcImage)
{
	double sigma = 5, threshold = 0, amount = 5;		// setting (for sharpen)
	// sharpen image using "unsharp mask" algorithm
	Mat blurred;
	GaussianBlur(srcImage, blurred, Size(), sigma, sigma);
	Mat lowContrastMask = abs(srcImage - blurred) < threshold;
	Mat sharpened = srcImage * (1 + amount) + blurred * (-amount);
	//srcImage.copyTo(sharpened, lowContrastMask);

	return sharpened;
}

Mat analyzer::getFullyContrastImage(Mat srcImage)
{
	// RGB 분리
	Mat bgr[3];
	split(srcImage, bgr);
	// 각각의 128 임계로 한 이미지 얻음
	threshold(bgr[0], bgr[0], 127, 255, THRESH_BINARY);
	threshold(bgr[1], bgr[1], 127, 255, THRESH_BINARY);
	threshold(bgr[2], bgr[2], 127, 255, THRESH_BINARY);
	// RGB 합침
	Mat mergedImage;
	merge(bgr, 3, mergedImage);
	return mergedImage;
}

/*
	보풀 제거 : 특점 흰점의 좌우 또는 위아래가 흑색인 경우 흑색으로 변경 and HSV필터링 한 이미지가 흰색이 아닌 경우

*/
Mat analyzer::removeLint(Mat srcImage, Mat refImage)
{
	int height = srcImage.rows;
	int width = srcImage.cols;
	Mat outLintedImage = srcImage.clone();

	for (int y = 1; y < height - 1; y++)
	{
		uchar* yPtr = srcImage.ptr<uchar>(y);
		uchar* yPtr_up = srcImage.ptr<uchar>(y - 1);
		uchar* yPtr_down = srcImage.ptr<uchar>(y + 1);
		for (int x = 1; x < width - 1; x++)
		{
			if (refImage.ptr<uchar>(y)[x] == 255)	// 완전 흰색인경우 연산에서 제외
				continue;

			bool isLint = false;
			if (0 != yPtr[x])	// 흑색이 아닌경우
			{
				if (yPtr[x - 1] == 0 && yPtr[x + 1] == 0)	// 좌 우
					isLint = true;
				else if (yPtr_up[x] == 0 && yPtr_down[x] == 0) // 상 하
					isLint = true;
				else if (yPtr_up[x - 1] == 0 && yPtr_down[x + 1] == 0)	// 좌상 우하
					isLint = true;
				else if (yPtr_down[x - 1] == 0 && yPtr_up[x + 1] == 0) // 좌하 우상
					isLint = true;

				if (isLint)
					outLintedImage.ptr<uchar>(y)[x] = 0;
			}
		}
	}
	return outLintedImage;
}

/// <summary>
/// 색칠된 부분찾아 흑백으로 표시한 이미지
/// 흰점 : 단순화된 이미지(FullyContrast)에서 흰점 기준 좌우상하 검사함
/// 조건 : 흰점기준으로 한쪽은 빨강or파랑이 2점 이상 연속되고, 반대쪽은 검정이 2점 이상 연속됨 
/// </summary>
/// <param name="srcImage">The source image.</param>
/// <returns></returns>
Mat analyzer::getPaintedBinImage(Mat srcImage)
{
	Mat fullyContrastImage = getFullyContrastImage(srcImage);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage;
	cvtColor(fullyContrastImage, outImage, COLOR_BGR2GRAY);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			bool isRight = false;	// 조건만족?
			if (isWhite(yPtr_FCImage[x]))////if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	
			{
				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;
				while (x - count > 0)	// 아래쪽 색 확인
				{
					Vec3b v3p = yPtr_FCImage[x - count];
					if (isWhite(v3p))
					{
						count++;//x_m--;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (x - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x - (count + 1)]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_m = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (x - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x - (count + 1)]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlack(v3p_))
							{
								color_m = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_m = -1;	// Other Color
						break;
					}
				}

				count = 1;
				while (x + count < width)	// 위쪽 색 확인
				{
					Vec3b v3p = yPtr_FCImage[x + count];//FCImage.at<cv::Vec3b>(y, x + count);
					if (isWhite(v3p))
					{
						count++;//x_p++;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (x + (count + 1) < width)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_p = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (x + (count + 1) < width)	// 연속되는지 확인
						{
							Vec3b v3p_ = yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlack(v3p_))
							{
								color_p = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_p = -1;	// Other Color
						break;
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족
			}

			if (isRight)	// 조건에 만족함
				yPtr[x] = 255;
			else
				yPtr[x] = 0;
		}
	}

	// 열연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //

		for (int x = 0; x < width; x++)
		{
			if (isWhite(yPtr[x]))	// 이미 흰색인곳
				continue;

			bool isRight = false;	// 조건만족?
			if (isWhite(yPtr_FCImage[x]))
			{
				int count = 1;
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (y - count > 0)	// 아래쪽 색 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(y - count)[x];
					if (isWhite(v3p))
					{
						count++;//x_m--;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (y - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y - (count + 1))[x];//yPtr_FCImage[x - (count + 1)]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_m = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (y - (count + 1) > 0)	// 연속되는지 확인
						{
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y - (count + 1))[x]; //yPtr_FCImage[x - (count + 1)]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
							if (isBlack(v3p_))
							{
								color_m = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_m = -1;	// Other Color
						break;
					}
				}

				count = 1;
				while (y + count < height)	// 위쪽 색 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(y + count)[x]; //yPtr_FCImage[x + count];//FCImage.at<cv::Vec3b>(y, x + count);
					if (isWhite(v3p))
					{
						count++;//x_p++;
						if (count > 4)
							break;
					}
					else if (isBlue(v3p) || isRed(v3p))
					{
						if (y + (count + 1) < height)	// 연속되는지 확인
						{
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlue(v3p_) || isRed(v3p_))
							{
								color_p = 1;	// B or R
							}
						}
						break;
					}
					else if (isBlack(v3p))
					{
						if (y + (count + 1) < height)	// 연속되는지 확인
						{
							Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(y + (count + 1))[x];//yPtr_FCImage[x + count + 1]; //FCImage.at<cv::Vec3b>(y, x + (count + 1));
							if (isBlack(v3p_))
							{
								color_p = 2;	// Black
							}
						}
						break;
					}
					else
					{
						color_p = -1;	// Other Color
						break;
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족
			}

			if (isRight)	// 조건에 만족함
				yPtr[x] = 255;
			//else
			//	yPtr[x] = 0;
		}
	}

	return outImage;
}

bool analyzer::isWhite(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 255 && ptr[2] == 255)
		return true;
	return false;
}

bool analyzer::isBlack(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool analyzer::isBlue(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool analyzer::isRed(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 255)
		return true;
	return false;
}


/// <summary>
/// 성공적으로 Calibration 된 라인에서 word를 나눕니다.
/// 1. Line측정에 사용된 mask이미지로 x축 프로잭션 진행
/// 2. 시작x~끝x 사이에 빈 픽샐이 약 10개정도 있는 부분을 Separation으로 함
//		2-1. Sep이 0개인 경우 = 통 라인, 라인정보가 첫번째 word 정보와 같아짐
//		2-2. Sep이 n개인 경우 = word수는 n+1
/// </summary>
void analyzer::wordJudge()
{
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		line->getSpacingWordsFromMaskImage();	

		printf("Line%d : spacingWords : %d \r\n", i, line->spacingWords.size());

		wordCalibration(*line);
		//if (line->spacingWords.empty() == true)	// 스페이스바가 없는 라인
		//	;
		//else	// Word separting 해야함
		//{
		//	// diff이미지로 분석, diff되는 부분 중 가장 오른쪽 점을 좌표기준으로 함
		//	// diff image = fullContrastimage_NotBlur=>binImage - beforeimage (and use mask)
		//	// 좌표기준이 sep좌표를 넘지 않고 증가가 없어지는 시기 = word-end,
		//	// 좌표기준이 sep좌표를 넘어 증가가 시작되는 시기 = word-start,

		//}

	

	}

}

void analyzer::wordCalibration(Line& line)
{
	if (line.spacingWords.size() == 0)
	{
		Word word = Line::lineToWord(line);
		line.words.push_back(word);
		return;
	}
	Mat maskImage = line.maskImage;
	Mat readImage;
	Mat subImage;
	Mat beforeBinImage;
	int rightistPoint = 0;
	vector<pair<int, int>> paintedPoint;
	int frameIndex = line.startFrame;
	while (true)
	{
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)frameIndex - 1);
		videoCapture->read(readImage);
		if (readImage.rows != 720)
			readImage = imageHandler::resizeImageToAnalize(readImage);
		subImage = imageHandler::getSubtitleImage(readImage);

		Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
		Mat binImage;
		inRange(fullyContrastImage, Scalar(254, 254, 254), Scalar(255, 255, 255), binImage);
		
		if (beforeBinImage.empty())
			beforeBinImage = binImage;

		Mat diffBinImage = imageHandler::getDifferenceImage(binImage, beforeBinImage);
		bitwise_and(diffBinImage, line.maskImage, diffBinImage);

		if (rightistPoint == 0)// init first value
			rightistPoint = getLeftistWhitePixel_x(maskImage);
		rightistPoint = getRightistWhitePixel_x(diffBinImage, rightistPoint, 35, 3);  // getRightistWhitePixel_x( diffBinImage, 관심영역x축, 타겟+x축(약 15pix), 3);

		//Mat diffBinMorphImage = imageHandler::getMorphImage(diffBinImage, MORPH_ERODE);	// 느린 진행시 삭제됨..
		//int rightistPoint = getRightistWhitePixel_x(diffBinMorphImage);	// getRightistWhitePixel_x함수 튜닝 (관심영역x축, 타겟+x축(약 15pix) )

		if (paintedPoint.empty())
		{
			paintedPoint.push_back(make_pair(frameIndex, rightistPoint));
		}
		else
		{
			if (paintedPoint.back().second >= rightistPoint)
			{
				;// paintedPoint.push_back(make_pair(frameIndex, paintedPoint.back().second));
			}
			else
			{
				paintedPoint.push_back(make_pair(frameIndex, rightistPoint));
			}
		}
		printf("%d %d \r\n", paintedPoint.back().first, paintedPoint.back().second);
		

		// diff 의 흰점으로 분석
		// 의미있는 흰점중 가장 오른쪽점을 진행도로 함
		// vector<pair<int, int>> paintedPoint; 에 정보를 모음	 ( pair<frame, xPoint> )
		// (후처리 필요시 진행)

		beforeBinImage = binImage.clone();

		frameIndex++;
		if (frameIndex > line.endFrame)
			break;
	}
	
	// Line.spacingWord[x]보다 높은 값이 나오는 첫frame -> start
	// Line.spacingWord[x]로 이미지를 잘라 가장 오른쪽 흰점의 좌표를 구함
	// 좌표보다 큰 값이 나오는 첫 지점 -> end
	//vector<pair<int, int>> paintedPoint;
	vector<Word> words;
	// init
	for (int i = 0; i < line.spacingWords.size()+1; i++)
	{	// init
		Word newWord;
		if (i == 0)	// 첫 워드
			newWord.startFrame = line.startFrame;
		if (i == line.spacingWords.size() - 1)	// 마지막 워드 
			newWord.endFrame = line.endFrame;
		words.push_back(newWord);
	}

	for (int i = 0; i < line.spacingWords.size(); i++)	// words[i], words[i+1]
	{
		int spacingWordX = line.spacingWords[i];		// spacingWords[0]의 끝프래임은 spacing[0]보다 작은 값 중 가장 큰 값.
		printf("SpaceWord[%d] : %d\r\n",i , spacingWordX);
		for (int j = 0; j < paintedPoint.size(); j++)
		{
			if (spacingWordX < paintedPoint[j].second)	// 띄어쓰기보다 커지는순간
			{
				words[i + 1].startFrame = paintedPoint[j].first;	// 다음 워드의 시작점
				int offset = 1;
				//while(true)
				//{
				//	if (paintedPoint[j-1].second != paintedPoint[(j) - offset].second)
				//		break;
				//	offset++;
				//}
				words[i].endFrame = paintedPoint[j-(offset)].first;	// 현재 워드의 끝점 ()
				break;
			}
		}
	}
	line.words = words;
}

int analyzer::getLeftistWhitePixel_x(Mat binImage)
{
	int leftist_x = 0;

	//int height = binImage.rows;
	//int width = binImage.cols;

	for (int width = 0; width < binImage.cols; width++)
	{
		for (int hight = 0; hight < binImage.rows; hight++)
		{
			if (binImage.at<uchar>(hight, width) != 0)
				return width;
		}
	}
	return leftist_x;
}

int analyzer::getRightistWhitePixel_x(Mat binImage)
{
	int rightist_x = 0;

	//int height = binImage.rows;
	//int width = binImage.cols;

	for (int width = binImage.cols -1; width > 0; width--)
	{
		for (int hight = 0; hight < binImage.rows; hight++)
		{
			if (binImage.at<uchar>(hight, width) != 0)
				return width;
		}
	}
	return 	rightist_x = 0;
}

/// <summary>
/// Gets the rightist white pixel x.
/// </summary>
/// <param name="binImage">The bin image.</param>
/// <param name="targetStartX">대상이 될시작지점.</param>
/// <param name="range">대상 x 부터 +range까지 검사.</param>
/// <param name="threshold"> 대상이 될 col의 흰점 최소 갯수.</param>
/// <returns></returns>
int analyzer::getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold)
{	// ys 
	// get Vertical Projection
	// 대상 x 부터 x+range 까지 검사한 것 중 thresold보다 큰 값,
	// 만약 최대 x가 결과라면 그 앞쪽까지 더 확인?
	vector<int> vecVProjection = imageHandler::getVerticalProjectionData(binImage);
	

	// 뒤에서부터 검사
	for (int i = targetStartX+range; i > targetStartX; i--)
	{
		if (vecVProjection.size() <= i)
			break;

		if (vecVProjection[i] >= threshold)
		{
			if (targetStartX + range == i)	//
				i = getRightistWhitePixel_x(binImage, targetStartX+range, range, threshold);
			return i;
		}
	}

	return targetStartX;	// 못찾으면 값 유지 
}

int analyzer::getWhitePixelAverage(Mat binImage)
{
	int height = binImage.rows;
	int width = binImage.cols;
	int whiteCount = 0;
	int whitePixelXSum = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// 흑색이 아닌경우
			{
				whiteCount++;
				whitePixelXSum += x;
			}
	}
	
	if (whiteCount == 0)
		return 0;
	else 
		return whitePixelXSum/whiteCount;
}



