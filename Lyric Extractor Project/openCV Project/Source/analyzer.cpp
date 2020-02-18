#include "analyzer.h"
#include "testClass.h"

bool desc(pair<int,int> a, pair<int, int> b)
{
	return a.first > b.first;
}


void analyzer::initVariables()
{
	vecWhitePixelCounts.clear();
	vecWhitePixelChangedCounts.clear();

	m_lyric.init();
	videoHandler::closeVideo();

}

int analyzer::getContourCount(Mat cannyImage)
{
	vector<vector<Point>> contours;
	findContours(cannyImage, contours, RETR_TREE, CHAIN_APPROX_SIMPLE); // Image에서 Contour를 찾음

	return contours.size();
}

bool analyzer::videoAnalization(string videoPath)
{
	initVariables();

	if (!setVideo(videoPath))
	{
		return false;
	}	
	videoCapture = videoHandler::getVideoCapture();
	if (videoCapture == nullptr)
	{
		return false;
	}

	//Mat readImage, maskImage;	// Debug
	//videoCapture->set(CAP_PROP_POS_FRAMES, (double)4906 - 1);
	//videoCapture->read(readImage);
	//	readImage = imageHandler::getResizeAndSubtitleImage(readImage);
	//maskImage = imageToSubBinImage(readImage);
	//imageHandler::getNoiseRemovedImage(maskImage, true);
	//maskImage = imageHandler::removeSubLyricLine(maskImage);

	Mat orgImage;
	Mat subImage;
	Mat binImage;
	Mat beforeBinImage;	// 이전 바이너리 이미지
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
		
		subImage = imageHandler::getResizeAndSubtitleImage(orgImage); 

		//binImage = imageHandler::getCompositeBinaryImages(subImage);	// 파빨간색으로 

		binImage = imageHandler::getPaintedBinImage(subImage);

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
	getJudgedLine(verticalHistogramAverage);

	/* 2. 라인 보정 */ // 라인의 정확한 시작-끝 시간을 맞춤
	calibrateLines();

	// 단어 나누기 wordJudge();
	// wordJudge();

	/* Save pictures */
	captureLines(fileManager::getSavePath());
	capturedLinesToText(fileManager::getSavePath());

	readLyricsFromFile(fileManager::getSavePath());
	m_lyric.writeLyricFile(videoCapture);

	// new wordCalibration..
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		wordCalibrationByOCRText(*m_lyric.getLine(i));
	}

	m_lyric.writeLyric_withWordFile(videoCapture);

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
		
		subImage = imageHandler::getResizeAndSubtitleImage(orgImage);

		paintedBinImage = imageHandler::getPaintedBinImage(subImage);
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
void analyzer::getJudgedLine(const vector<int> verticalHistogramAverage)
{
	vector<int> peakValues;

	// Peak 추출
	peakValues = getPeakFromWhitePixelCounts(vecWhitePixelCounts);

	for (int i = 0; i < peakValues.size(); i++)
		printf("Peak %2d : %d\r\n", i, peakValues[i]);

	string fileName = "peak.txt";
	fileManager::writeVector(fileName, peakValues);

	getLinesFromPeak(peakValues, vecWhitePixelCounts);

	//linesRejudgeByLineLength();
	//lineRejudgeByPixelCount(vecWhitePixelCounts);

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

	bool isFadeOut = false;
	int fadeOutCount = 0;
	int maxValueFrame = 0;
	bool zeroZone = true;
	int noneZeroZonCount = 0;
	for (int frameNum = 1; frameNum < vecWhitePixelCounts.size(); frameNum++)
	{
		bool isZeroZone;
		if (vecWhitePixelCounts[frameNum] < 100)
		{
			isZeroZone = true;
		}
		else if (vecWhitePixelCounts[frameNum-1] > 1000)	// 90%이상 하락일 시 peak로 판단하기 위한..
		{
			if (vecWhitePixelCounts[frameNum - 1] / 10 > vecWhitePixelCounts[frameNum])
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

			if (vecWhitePixelCounts[maxValueFrame] < vecWhitePixelCounts[frameNum])
				maxValueFrame = frameNum;

			if (vecWhitePixelCounts[frameNum] < vecWhitePixelCounts[frameNum - 1])	// 이전값이 큰경우가 연속일 경우...
				fadeOutCount++;
			else
				fadeOutCount = 0;

			if (isZeroZone == true)	// noneZero -> Zero,	
			{
				zeroZone = true;

				if (noneZeroZonCount < 5)
					continue;

				if(vecWhitePixelCounts[maxValueFrame]/2 > vecWhitePixelCounts[frameNum-1])
					peakValues.push_back(maxValueFrame);
				//if(fadeOutCount>3)
				//	peakValues.push_back(maxValueFrame);
				else
				{
					peakValues.push_back(frameNum - 1);	
				}
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
			if (vecWhitePixelCounts[index] < 100)
			{
				minIndex = index;	// 100보다 작은 값인 곳 중 뒷값
			}
			if (vecWhitePixelCounts[minIndex] >= vecWhitePixelCounts[index])	// 가장 마지막 0이 됨
			{
				minIndex = index;	// 무조건 작은값인 곳 중 뒷값
			}
		}
		
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
		int minFrame;
		if (i > 0)
			minFrame = m_lyric.getLine(i - 1)->endFrame;
		else
			minFrame = 0;

		printf("Cal Line%d : %d - %d\r\n", i, line->startFrame, line->endFrame);
		if (i == 9)
			i = 9;
		
		if (lineCalibration(line->startFrame, line->endFrame, maskImage, minFrame) == false)
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
			Mat sourceImg;
			videoCapture->set(CAP_PROP_POS_FRAMES, (double)line->startFrame - 2);
			videoCapture->read(sourceImg);

			sourceImg = imageHandler::getResizeAndSubtitleImage(sourceImg);

			Mat newMaskImage = imageHandler::getSharpenAndContrastImage(sourceImg); // 1. 라인 시작지점으로 마스크 얻음
								// 1-1. get ContrastImage
			Mat newMaskImage_hsv, newMaskImage_gray;
			cvtColor(newMaskImage, newMaskImage_hsv, COLOR_BGR2HSV);
			cvtColor(newMaskImage, newMaskImage_gray, COLOR_BGR2GRAY);
			Mat newMaskImage_bin;
			inRange(newMaskImage_hsv, Scalar(0, 0, 254), Scalar(255, 255, 255), newMaskImage_bin);	// 1-2. ContrastImage->hsv filtering
			// 부족하면 이전or다음 프레임 동원하여 해결할것.
			newMaskImage_bin = removeLint(newMaskImage_bin, newMaskImage_gray);	// 튀어나온것 제거

	// 1. fullContrastImage 
	// 2. 흰색만 분리
	// 3. and 연산 
			videoCapture->set(CAP_PROP_POS_FRAMES, (double)(line->startFrame - 1 )- 1);
			videoCapture->read(sourceImg);

			sourceImg = imageHandler::getResizeAndSubtitleImage(sourceImg);
			Mat fullyContrastImage = getFullyContrastImage(sourceImg);
			Mat fullyContrastImage_white;
			inRange(fullyContrastImage, Scalar(250, 250, 250), Scalar(255, 255, 255), fullyContrastImage_white);


			bitwise_and(newMaskImage_bin, fullyContrastImage_white, newMaskImage_bin);

			newMaskImage = getBinImageByFloodfillAlgorism(newMaskImage_bin, maskImage);

			//Mat mofImag = imageHandler::getMorphImage(newMaskImage, MorphTypes::MORPH_DILATE);	// as
			imageHandler::getNoiseRemovedImage(newMaskImage, true);
			imageHandler::getBorderFloodFilledImage(newMaskImage, true);
			line->maskImage = newMaskImage.clone();
			catpureBinaryImageForOCR(line->maskImage.clone(), i-invaildCount, fileManager::getSavePath());

			/* 라인 끝점으로 마스크 얻음*/
			//line->maskImage = maskImage.clone();
			//catpureBinaryImageForOCR(maskImage, i - invaildCount, fileManager::getSavePath());
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
bool analyzer::lineCalibration(int& startFrame, int& endFrame, Mat& maskImage, static int minStartFrame)
{
	bool isEffictiveLine = true;
	Mat readImage;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)endFrame-1);
	videoCapture->read(readImage);

	maskImage = imageToSubBinImage(readImage);	
	maskImage = imageHandler::getNoiseRemovedImage(maskImage, true);
	Mat maskImage_back = maskImage.clone();
	maskImage_back = imageHandler::removeSubLyricLine(maskImage);
	maskImage = imageHandler::removeNotPrimeryLyricLine(maskImage);	// maskImage 

	int maskImage_leftDot_x = imageHandler::getLeftistWhitePixel_x(maskImage);
	int maskImage_rightDot_x = imageHandler::getRightistWhitePixel_x(maskImage);
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

		maskImage_leftDot_x = imageHandler::getLeftistWhitePixel_x(maskImage);
		maskImage_rightDot_x = imageHandler::getRightistWhitePixel_x(maskImage);
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
	int diffImage_avgPoint_PerMin = 100;
	int MinimumPixelCount = 500;	// 임의 초기값
	int startframe_org = startFrame;
	
	int frameIndex = endFrame;
	int expectedRightistPoint = maskImage_rightDot_x;
	bool underMiddlePoint = false;

	// 3. 진행도 측정 -> 라인의 시작과 끝점 파악
	while (true)	
	{

		videoCapture->set(CAP_PROP_POS_FRAMES, (double)frameIndex-1);	// frameIndex-1
		videoCapture->read(readImage);

		Mat subImage = imageHandler::getResizeAndSubtitleImage(readImage); 
		Mat binCompositeImage = imageHandler::getCompositeBinaryImages(subImage);
		Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색(파랑, 빨강), 
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);		//파, 빨

		bitwise_and(subImage_hsv, maskImage, binImage);

		if (beforeBinImage.empty())
			beforeBinImage = binImage.clone();

		Mat DifferenceImage = imageHandler::getDifferenceImage(binImage, beforeBinImage);

		Mat image_dilation;
		Mat element(3, 3, CV_8U, Scalar(1));
		element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
		erode(DifferenceImage, image_dilation, element);	// 침식연산 - 노이즈제거 

		//Mat avgPixelMask = imageHandler::getColumMaskImage(binImage.cols, binImage.rows, 200, expectedAvgPoint);
		//bitwise_and(image_dilateion.clone(), avgPixelMask, image_dilateion);

		int pixelCount = imageHandler::getWihtePixelCount(binImage);
		int diffImage_rightistPoint = imageHandler::getRightistWhitePixel_x(image_dilation);//getWhitePixelAverage(image_dilateion);	
		int diffImage_rightistPoint_Per = (int)((diffImage_rightistPoint - maskImage_leftDot_x) / ( (maskImage_rightDot_x - maskImage_leftDot_x) / 100.0));

		printf("frame %d - diffAvgPoint: %d, diffImage_avgPoint_Per: %d \r\n", frameIndex, diffImage_rightistPoint, diffImage_rightistPoint_Per);
		printf("frame %d - pixelCount: %d \r\n", frameIndex, pixelCount);
		
		if (MinimumPixelCount > pixelCount)
		{
			MinimumPixelCount = pixelCount;
			startFrame = frameIndex + 1;
		}
		if (MinimumPixelCount < 100 && pixelCount>1000)	// 최소 픽셀카운트가 100이다가 갑자기 증가할때
			break;

		if (diffImage_avgPoint_PerMax <= diffImage_rightistPoint_Per)	// 가장 큰 Percent 변경점이 시작점, => 마스크 가장 오른쪽 점 
		{					
			endFrame = frameIndex + 1;
			diffImage_avgPoint_PerMax = diffImage_rightistPoint_Per;
		}
	
		beforeBinImage = binImage.clone();

		frameIndex --;		
		if (pixelCount == 0 || diffImage_rightistPoint_Per==0)
			break;
	}
	
	if (isEffictiveLine)		
	{
		printf("StartFrame : %d \r\n", startFrame -1);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)startFrame - 2);	// frameIndex-1
		videoCapture->read(readImage);

		subImage = imageHandler::getResizeAndSubtitleImage(readImage);
		// 1. 흰색의 이진화한 이미지 구함
		// 2. floodfill (maskImage, 1번한것)
	// inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), image_bin_inRange);	
		inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), binImage);
		maskImage = getBinImageByFloodfillAlgorism(maskImage_back, binImage);
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
	binImage = imageHandler::cutColumByHistorgram(binImage);

	bitwise_not(binImage, binImage);
	// resize

	//resize(binImage, binImage, cv::Size(binImage.cols * 0.6, binImage.rows * 0.6), 0, 0, cv::INTER_CUBIC);

	resize(binImage, binImage, cv::Size(0, 0), 0.8, 0.8, cv::INTER_CUBIC);	// for text Height Size

	threshold(binImage, binImage, 128, 255, THRESH_BINARY);

	imwrite(videoPath + "/Captures/Line" + to_string(lineNum) + "_Bin.jpg", binImage);
}

/// <summary>
/// TargetImage에서 subtitle부분을 자르고 흑백연산 + 노이즈 제거 의 결과 이미지 반환.
/// </summary>
/// <param name="targetImage">The target image.</param>
/// <returns></returns>
Mat analyzer::imageToSubBinImage(Mat targetImage)
{
	Mat subImage = imageHandler::getResizeAndSubtitleImage(targetImage);
	//Mat binCompositeImage = imageHandler::getCompositeBinaryImages(subImage);
	Mat justFullContrastImage = imageHandler::getFullyContrastImage(subImage);
	Mat sharpenContrastImage = imageHandler::getSharpenAndContrastImage(subImage);	// YSYS
	// justFullContrastImage에 sharpenContrastImage에 흰색인 곳의 좌표에 흰색처리함
	Mat mixedImage = justFullContrastImage.clone();
	Vec3b whiteColor;
	whiteColor[0] = 255;
	whiteColor[1] = 255;
	whiteColor[2] = 255;

	for (int row = 0; row < justFullContrastImage.rows; row++)
	{
		Vec3b* yPtr_target = sharpenContrastImage.ptr<Vec3b>(row); //
		Vec3b* yPtr_destination = mixedImage.ptr<Vec3b>(row); //
		for (int col = 0; col < justFullContrastImage.cols; col++)
		{
			Vec3b v3b = yPtr_target[col];
			if (imageHandler::isWhite(v3b) == true)
				yPtr_destination[col] = whiteColor;
		}
	}

	Mat printImage = imageHandler::getPaintedBinImage_inner(mixedImage);
	//printImage = imageHandler::getMorphImage(printImage, MORPH_ERODE);
	/*
		
	*/

	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색(파랑, 빨강), 
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
	inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);		//파, 빨

	Mat image_out;
	image_out = getBinImageByFloodfillAlgorism(subImage_hsv, printImage);

	image_out = imageHandler::getBorderFloodFilledImage(image_out, true);

	image_out = imageHandler::getMorphImage(image_out, MORPH_CLOSE);

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

void analyzer::readLyricsFromFile(string videoPath)
{
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
	}
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

bool analyzer::setVideo(string videoPath)
{
	return videoHandler::setVideo(videoPath);
}

void analyzer::closeVideo()
{
	videoHandler::closeVideo();
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

		printf("Line%d : spacingWords : %d \r\n", i, line->spacingWords.size());

		wordCalibration(*line);
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
	vector<pair<int, int>> paintedPoint;	// <frameNum, 진행도(x좌표)>	// out

	paintedPoint = getPaintedPoint(line);	//
	
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

void analyzer::wordCalibrationByOCRText(Line& line)
{
	// Line space 구함
	// 결과물 길이로 소팅
	// line에 ' '의 개수만큼 space기준으로 word화 

	vector<pair<int, int>> spaces;			// <start, end>
	vector<pair<int, int>> spacesLength;	// <length, spaces_index>
	vector<int> projection = imageHandler::getVerticalProjectionData(line.maskImage);	// [x]가 0이면 space
	bool spaceZone = false;
	int startPoint = 0;
	int endPoint = 0;

	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)	// Not blackZone
		{
			if (spaceZone == true)	// blackZone -> Not BlackZone
			{// save island
				if (startPoint != 0)
				{
					spaces.push_back(make_pair(startPoint, i));
					spacesLength.push_back(make_pair(i - startPoint, spaces.size() - 1));
				}
			}

			spaceZone = false;
		}
		else // blackZone
		{
			if (spaceZone == false)	// Not blackZone -> blackZone
			{	
				startPoint = i;
			}
			//
			spaceZone = true;
		}
	}

	sort(spacesLength.begin(), spacesLength.end(), desc);	// 확인 요(Length로 sorting 되어야 함) YS

	vector<string> tokens;
	boost::split(tokens, line.text, boost::is_any_of(" ")); 
	vector<Word> words;
	for (int i = 0; i < tokens.size(); i++)
	{
		Word word;
		if (tokens[i].find_first_not_of(' ') != std::string::npos)
		{
			// There's a non-space.
			word.text = tokens[i];
			words.push_back(word);
		}
	}	// 가사 먼저 넣음.

	//vector<pair<int, int>> spacesLength_cut;	// <index, length>
	//spacesLength_cut.assign(spacesLength.begin(), spacesLength.begin() + words.size());

	for (int i = 0; i < words.size()-1; i++)	// token만큼의 스페이스바
	{
		int startFrame = spaces[spacesLength[i].second].first;  // [index].first
		int endFrame = spaces[spacesLength[i].second].second;

		line.spacingWords.push_back((endFrame-startFrame)/2 + startFrame);
	}
	sort(line.spacingWords.begin(), line.spacingWords.end());

	vector<pair<int, int>> paintedPoint;
	paintedPoint = getPaintedPoint(line);

	// init
	//for (int i = 0; i < line.spacingWords.size(); i++)
	//{	// init
	//	if (i == 0)	// 첫 워드
	//		words[i].startFrame = line.startFrame;
	//	else if (i == line.spacingWords.size()-1)	// 마지막 워드 
	//		words[i].endFrame = line.endFrame;
	//}
	words[0].startFrame = line.startFrame;
	words[words.size()-1].endFrame = line.endFrame;
	//words.begin()->startFrame = line.startFrame;
	//words.end()->startFrame = line.endFrame;

	for (int i = 0; i < line.spacingWords.size(); i++)	// words[i], words[i+1]
	{
		int spacingWordX = line.spacingWords[i];		// spacingWords[0]의 끝프래임은 spacing[0]보다 작은 값 중 가장 큰 값.
		printf("SpaceWord[%d] : %d\r\n", i, spacingWordX);
		for (int j = 0; j < paintedPoint.size(); j++)
		{
			if (spacingWordX < paintedPoint[j].second)	// 띄어쓰기보다 커지는순간
			{
				if(words.size()>i)
					words[i + 1].startFrame = paintedPoint[j].first;	// 다음 워드의 시작점
				int offset = 1;
				words[i].endFrame = paintedPoint[j - (offset)].first;	// 현재 워드의 끝점 ()
				break;
			}
		}
	}
	line.words = words;
	
}

vector<pair<int, int>> analyzer::getPaintedPoint(Line line)
{
	vector<pair<int, int>> paintedPoint;

	Mat maskImage = line.maskImage;
	Mat readImage;
	Mat subImage;
	int frameIndex = line.startFrame;
	int rightistPoint = 0;
	while (true)	// startFrame -> endFrame	// 한 라인의 진행도
	{
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)frameIndex - 1);
		videoCapture->read(readImage);
		subImage = imageHandler::getResizeAndSubtitleImage(readImage);
		Mat binImage;

		// start
		Mat subImage_hsv;
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);		//파, 빨
		bitwise_and(subImage_hsv, maskImage, binImage);	// 마스크 얻는방식 변경
		// end


		if (rightistPoint == 0)// init first value
			rightistPoint = imageHandler::getLeftistWhitePixel_x(maskImage);
		else
			rightistPoint = imageHandler::getRightistWhitePixel_x(binImage, rightistPoint, 200, 3);  // getRightistWhitePixel_x( diffBinImage, 관심영역x축, 타겟+x축(약 15pix), 3);

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

		frameIndex++;
		if (frameIndex > line.endFrame)
			break;
	}


	return paintedPoint;
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



