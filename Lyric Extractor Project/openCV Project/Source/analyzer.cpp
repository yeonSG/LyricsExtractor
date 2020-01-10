﻿#include "analyzer.h"
#include "testClass.h"

int analyzer::getContourCount(Mat cannyImage)
{
	vector<vector<Point>> contours;
	findContours(cannyImage, contours, RETR_TREE, CHAIN_APPROX_SIMPLE); // Image에서 Contour를 찾음

	return contours.size();
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

bool analyzer::videoAnalization(string videoPath)
{
	if (!setVideo(videoPath))
	{
		return false;
	}
	string savePath = fileManager::getSavePath();

	Mat orgImage;
	Mat subImage;
	Mat binImage;
	Mat beforeBinImage;	// 이전 바이너리 이미지
	vector<int> vecWhitePixelCounts;			// 프래임 별 흰색 개수
	vector<int> vecWhitePixelChangedCounts;		// 이전 프래임 대비 흰색 변화량
	vector<vector<int>> verticalProjectionDatasOfDifferenceImage;	// 디프런트 이미지의 흰색 vertical Projection데이터의 모임

	vecWhitePixelCounts.push_back(0);				// 프래임과 동기화 위해 배열 0번은 더미
	vecWhitePixelChangedCounts.push_back(0);		// 프래임과 동기화 위해 배열 0번은 더미
	verticalProjectionDatasOfDifferenceImage.push_back( vector<int>(videoCapture->get(CAP_PROP_FRAME_WIDTH)) );	// 프래임과 동기화 위해 배열 0번은 더미

	/* 이미지 분석 */
	while (videoCapture->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*videoCapture);

		if (orgImage.rows != 720)
			orgImage = imageHandler::resizeImageToAnalize(orgImage);

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
			verticalProjectionDatasOfDifferenceImage.push_back( getVerticalProjectionData(DifferenceImage) ); // 현 프레임의 vertical 프로젝션

			vecWhitePixelChangedCounts.push_back(getWihtePixelCount(DifferenceImage));
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
	imwrite(savePath + "ChangeHistogram.jpg", changeHistogramMat);

	vector<int> verticalHistogramAverage = getVerticalHistogramAverageData(changeHistorgramData);	// getChangeStatusAverage -> getChangeAverageHistogramData
	fileName = "WhitePixelChangedCountAverage.txt";
	fileManager::writeVector(fileName, verticalHistogramAverage); 

	Mat changeHistogramAverageMat = averageVectorToBinaryMat(verticalHistogramAverage, changeHistogramMat.cols); // 체인지 히스토그램의 평균점 이미지
	imwrite(savePath + "changeHistogramAverage.jpg", changeHistogramAverageMat);
	
	/* 1. 라인 확정 */	// 여기서는 유효한 라인만 걸러냄 (start-end frame은 대략적인부분으로)
	vector<pair<int, int>> lines = getJudgedLine(vecWhitePixelCounts, verticalHistogramAverage);
	catpureBinaryImageOfLinesEnd(lines, savePath);

	/* 2. 라인 보정 */ // 라인의 정확한 시작-끝 시간을 맞춤
	calibrateLines(lines);
	
	//catpureBinaryImageOfLinesEnd(lines, savePath);
	/* Save pictures */
	captureLines(lines, savePath);
	capturedLinesToText(lines.size(), savePath);
	makeLyrics(lines, savePath);

	closeVideo();
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
/// <returns>가사 Line들의 시작점-끝점 Pair.</returns>
vector<pair<int, int>> analyzer::getJudgedLine(vector<int> vecWhitePixelCounts, const vector<int> verticalHistogramAverage)
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

	for (int i = 0; i < lines.size(); i++)
		printf("Line%d : %d - %d\r\n", i, lines[i].first, lines[i].second);

	lineRejudgeByLineLength(lines);
	lineRejudgeByPixelCount(lines, vecWhitePixelCounts);
	// lineRejudgeByVerticalHistogramAverage(lines, verticalHistogramAverage);	// 왼쪽의 점 좌표 평균, 오른쪽의 점 좌표평균

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
		if (vecWhitePixelCounts[frameNum] > 500)	// 노이즈 레벨
		{
			bool isPeak = true;
			bool isHaveDrop = false;
			for (int checkRange = (frameNum - DEFAULT_FPS / 2); checkRange < frameNum + DEFAULT_FPS / 2; checkRange++)	// x-12
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
				int index = 0;
				while (true)
				{	
					if (vecWhitePixelCounts[frameNum] < vecWhitePixelCounts[frameNum + 1] + 200)	// 이전값과 별 차이 안나면 가장 나중값으로 정함
						frameNum++;
					else
						break;
				}

				if (!peakValues.empty())
				{
					if (peakValues.back() == frameNum)
						peakValues.back() = frameNum;	// update
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
		for (int index = minRange; index < peaks[i]; index++)	// 이전 피크부터 현재 피크까지의 값중 가장 흰점이 작은 점 중 가장 뒤에 있는 곳.
		{
			if (vecWhitePixelCounts[minIndex] >= vecWhitePixelCounts[index])	// 가장 마지막 0이 됨
			{
				minIndex = index;
			}
		}

		lines.push_back(make_pair(minIndex, peaks[i]));	
	}

	return lines;
}

/// <summary>
/// 가사 라인들 중 특정 프래임동안 지속되지 않는 line을 걸러냄
/// </summary>
/// <param name="judgedLines">The judged lines.</param>
/// <param name="fps">The FPS.</param>
void analyzer::lineRejudgeByLineLength(vector<pair<int, int>>& judgedLines, int fps)
{
	const int limitMSec = 400;	// 400ms = 10frame
	int limitFrame = limitMSec / (1000 / fps);
	printf("@@lineRejudgeByLineLength_Line minimum frame length(msec) : %d(%d)\r\n", limitFrame, limitMSec);

	int lineCount = 0;
	for (vector<pair<int, int>>::iterator it = judgedLines.begin(); it != judgedLines.end(); /*it++*/)
	{
		int lineLenght = it->second - it->first;

		printf("lines %2d Length: %d \r\n", lineCount, lineLenght);
		if (lineLenght < limitFrame)
		{
			printf("exceptionLine: %d\r\n", lineCount);
			it = judgedLines.erase(it);
			lineCount++;
			continue;
		}

		++it;
		lineCount++;
	}
}

void analyzer::lineRejudgeByPixelCount(vector<pair<int, int>>& judgedLines, vector<int> vecWhitePixelCounts)
{
	printf("@@lineRejudgeByPixelCount_ \r\n");
	int lineCount = 0;

	for (vector<pair<int, int>>::iterator it = judgedLines.begin(); it != judgedLines.end(); /*it++*/)
	{
		float pixelRatio = vecWhitePixelCounts[it->first] / (float)vecWhitePixelCounts[it->second];

		printf("lines %2d pixelRatio: %.2f (%d/%f) \r\n", lineCount, pixelRatio, vecWhitePixelCounts[it->first] ,(float)vecWhitePixelCounts[it->second]);
		if (0.5 < pixelRatio)	// startframe픽셀값 / endframe픽셀값 이 0.5보다 높다면 라인이 아닌걸로 판단.
		{
			printf("exceptionLine: %d\r\n", lineCount);
			it = judgedLines.erase(it);
			lineCount++;
			continue;
		}

		++it;
		lineCount++;
	}
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

void analyzer::calibrateLines(vector<pair<int, int>>& lines)
{
	int lineCount = 0;
	for (vector<pair<int, int>>::iterator it = lines.begin(); it != lines.end(); /*it++*/)
	{
		printf("Cal Line%d : %d - %d\r\n", lineCount, it->first, it->second);
		if (lineCalibration(it->first, it->second)==false)
		{
			printf("Cal Line%d : false.\r\n\r\n", lineCount);
			//printf("Cal Line%d : removed.\r\n\r\n", lineCount);
			//it = lines.erase(it);
			//lineCount++;
			//continue;
		}

		printf("Cal Line%d : %d - %d updated.\r\n\r\n", lineCount, it->first, it->second);
		++it;
		lineCount++;
	}
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
bool analyzer::lineCalibration(int& startFrame, int& endFrame)
{
	bool isEffictiveLine = true;
	Mat readImage;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)endFrame-1);
	videoCapture->read(readImage);
	if (readImage.rows != 720)
		readImage = imageHandler::resizeImageToAnalize(readImage);
	Mat maskImage = imageToSubBinImage(readImage);

	int maskImage_leftist_x = getLeftistWhitePixel_x(maskImage);
	int maskImage_rightest_x = getRightistWhitePixel_x(maskImage);
	int maskImage_middle_x = (maskImage_leftist_x + maskImage_rightest_x) / 2;

	int maskImage_pixelCount = getWihtePixelCount(maskImage);

	printf("MaskImage Info - left: %d right: %d middle: %d(%d) \r\n", maskImage_leftist_x, maskImage_rightest_x, maskImage_middle_x, readImage.cols / 2);
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

	int frameIndex = endFrame;
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

		int pixelCount = getWihtePixelCount(binImage);
		int diffImage_avgPoint = getWhitePixelAverage(image_dilateion);
		int diffImage_avgPoint_Per = (int)((diffImage_avgPoint - maskImage_leftist_x) / ( (maskImage_rightest_x - maskImage_leftist_x) / 100.0));

		printf("frame %d - diffAvgPoint: %d, diffImage_avgPoint_Per: %d \r\n", frameIndex, diffImage_avgPoint, diffImage_avgPoint_Per);
		printf("frame %d - pixelCount: %d \r\n", frameIndex, pixelCount);

		if (diffImage_avgPoint_PerMax <= diffImage_avgPoint_Per)
		{
			endFrame = frameIndex;
			diffImage_avgPoint_PerMax = diffImage_avgPoint_Per;
		}

		if (MinimumPixelCount >= pixelCount)
		{
			MinimumPixelCount = pixelCount;
			startFrame = frameIndex;
		}
		
		beforeBinImage = binImage.clone();

		frameIndex --;		
		if (frameIndex < startframe_org)
			break;
	}

	return isEffictiveLine;
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
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)lines[i].first -1);		
		videoCapture->read(startImage);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)lines[i].second);
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
		imwrite(videoPath + "/Captures/Line" + to_string(i) + "_Bin.jpg", subBinImage);
	}
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
	inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);
	//imshow("image_HSV_S", image_HSV_S);

	Mat image_out;
	//image_out = getBinImageByFloodfillAlgorism(image_floodFilled_AT2_Not, binCompositeImage);
	image_out = getBinImageByFloodfillAlgorism(subImage_hsv, binCompositeImage);

	image_out = imageHandler::getFloodProcessedImage(image_out, true);

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
	// $tesseract o.jpg o.out -l kor
	// 인자 : "인풋이미지" + "아웃.txt경로" + "-l tha+eng"
	string procName = "tesseract_5.0.exe";		// tesseract 경로
	string options = " -l tha+eng --oem 1 --psm 7";
	string tessdataPath = " --tessdata-dir tessdata";
	string blackList = " -c tessedit_char_blacklist=\"|:;\" ";
	string commandString = procName + " " + targetImage + " " + outFileName + options + blackList;
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



