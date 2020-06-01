#include "analyzer.h"
#include "testClass.h"
#include "loger.h"
#include <boost/log/trivial.hpp>
#include <Python.h>


bool desc(pair<int,int> a, pair<int, int> b)
{
	return a.first > b.first;
}
bool asc(pair<int, int> a, pair<int, int> b)
{
	return a.first < b.first;
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

bool analyzer::startVideoAnalization(string videoPath)
{
	initVariables();
	if (!setVideo(videoPath))
	{
		cout << "fail to open the video : " << videoPath << endl;
		return false;
	}
	fileManager::initDirectory(fileManager::videoName);	// 
	loger_init(fileManager::getSavePath());
	videoHandler::printVideoSpec();

	clock_t startClock = (int)clock();
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Start Analization";
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Output dir : " << fileManager::getSavePath();

	bool isSuccessed = false;
	
	try 
	{
		// @@@@@@@@@@@ START @@@@@@@@@@@@@@
		isSuccessed = videoAnalization3(videoPath);	//	
		//isSuccessed = videoAnalization2(videoPath);	// for noew
		//isSuccessed = videoAnalization(videoPath);
	}
	catch (exception & e)
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::error) << "*****Exception***** : " << e.what();
	}

	if (isSuccessed == false)
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::error) << "nProcess Not Successed : " << (clock() - startClock) / CLOCKS_PER_SEC << "Sec" << endl;
		printf("\r\nProcess Failed : %0.1fSec\r\n", (float)(clock() - startClock) / CLOCKS_PER_SEC);
	}
	else
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "nProcess Successed : " << (clock() - startClock) / CLOCKS_PER_SEC << "Sec" << endl;
		printf("\r\nProcess Successed : %0.1fSec\r\n", (float)(clock() - startClock) / CLOCKS_PER_SEC);
	}

	return true;
}

bool analyzer::videoAnalization(string videoPath)
{
	videoCapture = videoHandler::getVideoCapture();
	if (videoCapture == nullptr)
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::error) << "fail to getVideoCapture";
		return false;
	}
	
	Mat orgImage;
	Mat subImage;
	Mat binImage;
	Mat beforeBinImage;	// 이전 바이너리 이미지
	vector<vector<int>> verticalProjectionDatasOfDifferenceImage;	// 디프런트 이미지의 흰색 vertical Projection데이터의 모임

	vecWhitePixelCounts.push_back(0);				// 프래임과 동기화 위해 배열 0번은 더미
	vecWhitePixelChangedCounts.push_back(0);		// 프래임과 동기화 위해 배열 0번은 더미
	//verticalProjectionDatasOfDifferenceImage.push_back( vector<int>(videoCapture->get(CAP_PROP_FRAME_WIDTH)) );	// 프래임과 동기화 위해 배열 0번은 더미

	int curFrame = 0; 
	//curFrame = 6390;	// debug
	//videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);
	//for(int i=0; i<curFrame; i++)
	//	vecWhitePixelCounts.push_back(0);

	/* 이미지 분석 */
	while (videoCapture->read(orgImage))
	{
		//if (curFrame == 9430)		// debug
		//	break;
		videoHandler::printCurrentFrameSpec(*videoCapture);
		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);	// curFrame 을 얻어서 다시 세팅해 주는 이유는 순차적으로 읽다가 frame이 높은 프레임에 도달했을 때 읽힌 image가 실재 frame의 image+-1의 이미지가 읽히는 등의 현상이 생김.
		
		subImage = imageHandler::getResizeAndSubtitleImage(orgImage); 		
		binImage = imageHandler::getPaintedBinImage(subImage);

		//White dot Count
		vecWhitePixelCounts.push_back(imageHandler::getWhitePixelCount(binImage));
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Frame : " << curFrame << " pixelCount : " << vecWhitePixelCounts.back();

		/*	// 추가정보
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

			vecWhitePixelChangedCounts.push_back(imageHandler::getWhitePixelCount(DifferenceImage));
		}

		beforeBinImage = binImage;
		*/
	}	// end while

	string fileName = "WhitePixelCount.txt";
	fileManager::writeVector(fileName, vecWhitePixelCounts);

	/*	// 추가정보
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
	*/
	
	/* 1. 라인 확정 */	// 여기서는 유효한 라인만 걸러냄 (start-end frame은 대략적인부분으로)
	getJudgedLine();

	/* 2. 라인 보정 */ // 라인의 정확한 시작-끝 시간을 맞춤
	calibrateLines();
	
	/* Save pictures */
	captureLines();
	capturedLinesToText();

	readLyricsFromFile();
	m_lyric.writeLyricFile(videoCapture);

	wordCalibration();

	m_lyric.writeLyric_withWordFile(videoCapture);

	m_lyric.getTimeDataFromframeNum(videoCapture);
	Json json;
	json.makeJson(m_lyric);

	closeVideo();
	return true;
}

//
//bool analyzer::videoAnalization2(string videoPath)
//{
//	videoCapture = videoHandler::getVideoCapture();
//	if (videoCapture == nullptr)
//	{
//		BOOST_LOG_SEV(my_logger::get(), severity_level::error) << "fail to getVideoCapture";
//		return false;
//	}
//
//	/* 새 알고리즘 */
//	LineInfoFinder lineFinder(videoCapture);
//	m_lyric;
//	// lineFinder.start2();
//	if (1)
//	{
//		vector<PeakInfo> peaks;
//		vector<LineInfo> lineInfo;
//		vector<int> peaks_int;
//		//peaks = lineFinder.start2_useContour(0);// test
//		lineInfo = lineFinder.start2_useContour2(0, Scalar(255, 255, 255));// test
//		//peaks = lineFinder.start2_getLinePeak(0);	// blueColoer
//		printf(" \r\n	Color_blue \r\n");
//		for (int i = 0; i < lineInfo.size(); i++)
//		{
//			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Peak " << i << " : " << peaks[i].frameNum;
//			printf("peak %d : %d \r\n", i, peaks[i].frameNum);
//			peaks_int.push_back(peaks[i].frameNum);
//		}
//
//		vector<LineInfo> mergeJudgeLineInfo = lineFinder.mergeAndJudgeLineInfo(lineInfo);	//
//		printf(" \r\n	Color_blue(merge_judgeLines \r\n");
//		for (int i = 0; i < mergeJudgeLineInfo.size(); i++)
//		{
//			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Peak " << i << " : " << peaks[i].frameNum;
//			printf("peak %d : %d \r\n", i, peaks[i].frameNum);
//			peaks_int.push_back(peaks[i].frameNum);
//		}
//		
//		Vec3b upColor = lineFinder.findUnprintColor(peaks);	// Unprint 컬러 파악 루틴
//		m_lyric.setUnprintColor(upColor);
//
//		vector<Line> lines = lineFinder.peakToLine(peaks, m_lyric.getUnprintColor());
//		for(int i=0; i< lines.size(); i++)
//			m_lyric.addLine(lines[i]);
//
//		string fileName = "getPeak_0(Blue).txt";
//		fileManager::writeVector(fileName, peaks_int);
//
//		lineFinder.WriteLineInfo_toLog(lines);
//	}
//	if (1)
//	{
//		vector<PeakInfo> peaks;
//		vector<int> peaks_int;
//		peaks = lineFinder.start2_getLinePeak(1);	// redColoer
//		printf(" \r\n	Color_red \r\n");
//		for (int i = 0; i < peaks.size(); i++)
//		{
//			printf("peak %d : %d \r\n", i, peaks[i].frameNum);
//			peaks_int.push_back(peaks[i].frameNum);
//		}
//
//		Vec3b upColor = lineFinder.findUnprintColor(peaks);	// Unprint 컬러 파악 루틴
//		m_lyric.setUnprintColor(upColor);
//
//		vector<Line> lines = lineFinder.peakToLine(peaks, m_lyric.getUnprintColor());
//		for (int i = 0; i < lines.size(); i++)
//			m_lyric.addLine(lines[i]);
//
//		string fileName = "getPeak_1(Red).txt";
//		fileManager::writeVector(fileName, peaks_int);
//
//		lineFinder.WriteLineInfo_toLog(lines);
//	}
//	if (1)
//	{
//		vector<PeakInfo> peaks;
//		vector<int> peaks_int;
//		peaks = lineFinder.start2_getLinePeak(2);	// purpleColoer
//		printf(" \r\n	Color_purple \r\n");
//		for (int i = 0; i < peaks.size(); i++)
//		{
//			printf("peak %d : %d \r\n", i, peaks[i].frameNum);
//			peaks_int.push_back(peaks[i].frameNum);
//		}
//
//		Vec3b upColor = lineFinder.findUnprintColor(peaks);	// Unprint 컬러 파악 루틴
//		m_lyric.setUnprintColor(upColor);
//
//		vector<Line> lines = lineFinder.peakToLine(peaks, m_lyric.getUnprintColor());
//		for (int i = 0; i < lines.size(); i++)
//			m_lyric.addLine(lines[i]);
//
//		string fileName = "getPeak_2(Purple).txt";
//		fileManager::writeVector(fileName, peaks_int);
//
//		lineFinder.WriteLineInfo_toLog(lines);
//	}
//
//	m_lyric.sortingLine();
//
//	m_lyric.saveBinaryImage(fileManager::getSavePath());		// catpureBinaryImageOfLinesEnd()
//	capturedLinesToText();
//
//	readLyricsFromFile();
//	m_lyric.writeLyricFile(videoCapture);
//
//	// wordCalibration();	 << 재구현 해야함
//	//m_lyric.writeLyric_withWordFile(videoCapture);	// WordCal 완료시 수행할 것
//
//	m_lyric.getTimeDataFromframeNum(videoCapture);
//
//	Json json;
//	json.makeJson(m_lyric);
//
//	return 0;		// END,
//
//	
//	Mat orgImage;
//	Mat subImage;
//	Mat binImage;
//	Mat beforeBinImage;	// 이전 바이너리 이미지
//
//	Mat stackBinImage;
//	Mat FCImage_before;
//	Mat BWBPatternImage_before;
//
//	vecWhitePixelCounts.push_back(0);				// 프래임과 동기화 위해 배열 0번은 더미
//	vecWhitePixelChangedCounts.push_back(0);		// 프래임과 동기화 위해 배열 0번은 더미
//
//	int curFrame = 0;
//
//	/* 이미지 분석 */
//	while (videoCapture->read(orgImage))
//	{
//		videoHandler::printCurrentFrameSpec(*videoCapture);
//		curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
//		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);	// curFrame 을 얻어서 다시 세팅해 주는 이유는 순차적으로 읽다가 frame이 높은 프레임에 도달했을 때 읽힌 image가 실재 frame의 image+-1의 이미지가 읽히는 등의 현상이 생김.
//
//		Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
//		//Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
//		Mat fullyContrastImage = imageHandler::getFullyContrast_withDilate(subImage, Scalar(255, 0, 0));
//		Mat BWBPatternImage = imageHandler::getBWBPatternImage(fullyContrastImage.clone());
//
//		Mat printImage_blue = imageHandler::getPaintedBinImage_inner(fullyContrastImage, true);
//		Mat printImage_red = imageHandler::getPaintedBinImage_inner(fullyContrastImage, false);
//		Mat printImage;
//		bitwise_or(printImage_blue, printImage_red, printImage);
//
//		Mat mergedImage;
//		//binImage = imageHandler::getPaintedBinImage(subImage);
//
//		if (stackBinImage.empty() == true)
//			stackBinImage = Mat::zeros(subImage.rows, subImage.cols, CV_8U);	//dummy
//		if (FCImage_before.empty() != true && BWBPatternImage_before.empty() != true)
//		{
//			imageHandler::stackFCImage(fullyContrastImage, FCImage_before, stackBinImage, BWBPatternImage_before);
//			//imshow("stackBinImage", stackBinImage);	// stackBinImage 이거의 화이트카운트
//			bitwise_and(printImage, stackBinImage, mergedImage);
//		}
//
//		//White dot Count
//		//vecWhitePixelCounts.push_back(imageHandler::getWhitePixelCount(binImage));
//		vecWhitePixelCounts.push_back(imageHandler::getWhitePixelCount(mergedImage));
//	
//		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Frame : " << curFrame << " pixelCount : " << vecWhitePixelCounts.back();
//
//		FCImage_before = fullyContrastImage.clone();
//		BWBPatternImage_before = BWBPatternImage.clone();
//	}	// end while
//
//	string fileName = "WhitePixelCount.txt";
//	fileManager::writeVector(fileName, vecWhitePixelCounts);
//
//	/* 1. 라인 확정 */	// 여기서는 유효한 라인만 걸러냄 (start-end frame은 대략적인부분으로)
//	getJudgedLine();
//
//	//return true;		// debug
//	/* 2. 라인 보정 */ // 라인의 정확한 시작-끝 시간을 맞춤
//	calibrateLines();
//
//	/* Save pictures */
//	captureLines();
//	capturedLinesToText();
//
//	readLyricsFromFile();
//	m_lyric.writeLyricFile(videoCapture);
//
//	//wordCalibration();
//
//	//m_lyric.writeLyric_withWordFile(videoCapture);
//
//	//m_lyric.getTimeDataFromframeNum(videoCapture);
//	//Json json;
//	//json.makeJson(m_lyric);
//
//	closeVideo();
//	return true;
//}


bool analyzer::videoAnalization3(string videoPath)
{
	m_lyric;
	vector<LineInfo> lineInfo_all;

	videoCapture = videoHandler::getVideoCapture();
	if (videoCapture == nullptr)
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::error) << "fail to getVideoCapture";
		return false;
	}

	Scalar unPrintColor;
	bool isFoundColor = getUnprintColorRutin(unPrintColor);
	//Scalar unPrintColor = Scalar( 255, 255, 255 );	// YSYSYS - for debug
	printf("Unprint Color : { %f %f %f } \r\n", unPrintColor[0], unPrintColor[1], unPrintColor[2]);
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Unprint Color : { " << (int)unPrintColor[0] << " " << (int)unPrintColor[1] << " " << (int)unPrintColor[2] << "}" << endl;

	/* 새 알고리즘 */
	LineInfoFinder lineFinder(videoCapture);
	//Scalar unPrintColor = lineFinder.findUnprintColor(0);	// 파랑으로 UnprintColor 찾음
	//return false;	// fortest
	int lineFoundProcess_Lines[3] = { 0, };
	int lineFoundProcess_Lines_error[3] = { 0, };

	int mergeProcess_Lines[3] = { 0, };
	int mergeProcess_Lines_error[3] = { 0, };

	for(int nColor = 0; nColor < 3; nColor++)	// 0=blue, 1=red, 2=purple
	{
		
		//if (nColor == 0)		// YSYSYS - debug
		//	continue;
		//if (nColor == 2)	// RED
		//	continue;

		vector<LineInfo> lineInfo;
		vector<LineInfo> notErrorLineInfo;
		lineInfo = lineFinder.start2_useContour2(nColor, unPrintColor);
		printf(" \r\n	Color : %d \r\n", nColor);
		for (int i = 0; i < lineInfo.size(); i++)
		{
			lineFoundProcess_Lines[nColor]++;
			if (lineInfo[i].isValid != true)
			{
				lineFoundProcess_Lines_error[nColor]++;	// 라인 중 에러였던 것.
				BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line " << i << " : ERROR:"<< lineInfo[i].errorNumber << "(" << lineInfo[i].frame_start << " ~ " << lineInfo[i].frame_end << ")";
				continue;
			}

			notErrorLineInfo.push_back(lineInfo[i]);
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line " << i << " : " << lineInfo[i].frame_start << " ~ " << lineInfo[i].frame_end;
			printf("Line %d : %d ~ %d \r\n", i, lineInfo[i].frame_start, lineInfo[i].frame_end);
			Mat bin_Debug;
			inRange(lineInfo[i].maskImage_withWeight, 1, 255, bin_Debug);
			imwrite(fileManager::getSavePath() + "/Captures/"+to_string(nColor)+"_" + to_string(i) + ".jpg", lineInfo[i].maskImage_withWeight);
			imwrite(fileManager::getSavePath() + "/Captures/"+to_string(nColor)+"_d_" + to_string(i) + ".jpg", bin_Debug);
		}

		vector<LineInfo> mergeJudgeLineInfo = lineFinder.mergeAndJudgeLineInfo(notErrorLineInfo);	//
		vector<LineInfo> noErrorMergeJudgeLineInfo;

		printf(" \r\n	Color(merge_judgeLines \r\n");
		for (int i = 0; i < mergeJudgeLineInfo.size(); i++)
		{
			mergeProcess_Lines[nColor]++;
			if (!mergeJudgeLineInfo[i].isValid != true)
			{
				mergeProcess_Lines_error[nColor]++;	// 라인 중 에러였던 것.
				BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line " << i << " : ERROR:" << mergeJudgeLineInfo[i].errorNumber;
				continue;
			}
			noErrorMergeJudgeLineInfo.push_back(mergeJudgeLineInfo[i]);
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line " << i << " : " << mergeJudgeLineInfo[i].frame_start << " ~ " << mergeJudgeLineInfo[i].frame_end;
			printf("Line %2d : %d ~ %d \r\n", i, mergeJudgeLineInfo[i].frame_start, mergeJudgeLineInfo[i].frame_end);
			Mat bin_Debug;
			inRange(mergeJudgeLineInfo[i].maskImage_withWeight, 1, 255, bin_Debug);
			imwrite(fileManager::getSavePath() + "/Captures/" + to_string(nColor) + "_merge" + to_string(i) + ".jpg", mergeJudgeLineInfo[i].maskImage_withWeight);
			imwrite(fileManager::getSavePath() + "/Captures/" + to_string(nColor) + "_merge_d_" + to_string(i) + ".jpg", bin_Debug);

			mergeJudgeLineInfo[i].printColor = nColor;	// 컬러코드 정의필요()
			lineInfo_all.push_back(mergeJudgeLineInfo[i]);	// 통합라인에 추가
		}

	}
	
	// 라인, 에러 통계 출력
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "FOUND-LINE, ERROR-LINE";
	for (int i = 0; i < 3; i++)	// color Types
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << lineFoundProcess_Lines[i] << "	" << lineFoundProcess_Lines_error[i];
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << mergeProcess_Lines[i] << "	" << mergeProcess_Lines_error[i];
	}

	
	vector<LineInfo> mergeJudgeLineInfo = lineFinder.mergeLineInfo(lineInfo_all);	//전체 라인 머지	

	// lineInfo_all Sorting
	// lineFinder.mergeAndJudgeLineInfo() 수행
	// 0. 라인정보 이미지 저장
	// 1. OCR 수행

		for (int i = 0; i < mergeJudgeLineInfo.size(); i++)	// LineInfo -> Line 변환
		{
			Line line;
			Mat saveImage;
			inRange(mergeJudgeLineInfo[i].maskImage_withWeight, 1, 255, saveImage);
			//catpureBinaryImageForOCR(saveImage, i, fileManager::getSavePath()); // 이미지 저장

			line.maskImage = saveImage; //mergeJudgeLineInfo[i].maskImage_withWeight;
			line.startFrame = mergeJudgeLineInfo[i].frame_start;
			line.endFrame = mergeJudgeLineInfo[i].frame_end;
			line.text = "OCR TEXT";
			m_lyric.addLine(line);
		}
		captureLines();
		capturedLinesToText();

		readLyricsFromFile();
		findErrorFromLyrics();
		m_lyric.writeLyricFile(videoCapture);

		m_lyric.getTimeDataFromframeNum(videoCapture);
		Json json;
		json.makeJson(m_lyric);

	// 2. json 파일 생성

	/**** TO DO **** 
	0. unPrint 색정하는것
	0. B, R, P 라인 머지
	0. OCR 처리
	0. "Lyric.json" 생성
	0. binImage 데이터 255 이상까지 저장할 수 있도록 데이터타입 변경

	-> 체크포인트 : 모든 라인들이 다 잡아지는지 확인

	1. 얻은 라인들 병합하기 (R G B 로 찾은것들)
		- 얻은 라인의 총 픽셀수 or 위엣라인 or 프레임시간? 적절한것으로
		- 
	
	2. 후처리
		- 파일저장, OCR, json etc..

	*/
}
void analyzer::findErrorFromLyrics()
{
	int errorCount = 0;
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		if (line->text.size() < 5)	// 글자수가 5이하일때
		{
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Error Found from lyric : " << line->text <<" (size under 5)" ;
			line->text = line->text + " (ERROR:size under 5)";
			errorCount++;
		}
		else if (line->text.find("  ") != std::string::npos)	 // 글자에 띄어쓰기가 연속으로 들어갈 떄
		{
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Error Found from lyric : " << line->text << " (have continity space word)";
			line->text = line->text + " (ERROR:have continity space word 5)";
			errorCount++;
		}
		
	}
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "lyric Error : "<< errorCount <<"/" << m_lyric.getLinesSize();
	return;
}

bool analyzer::getUnprintColorRutin(Scalar& color)
{
	videoCapture = videoHandler::getVideoCapture();
	if (videoCapture == nullptr)
	{
		BOOST_LOG_SEV(my_logger::get(), severity_level::error) << "fail to getVideoCapture";
		return false;
	}
	LineInfoFinder lineFinder(videoCapture);

	vector<PeakInfo> peaks;
	peaks = lineFinder.start2_getLinePeak(0);	// blueColoer
	// y-start, y-end 를 구해 
	// y-length가 100 pixel 이상이면 
	// 절반 중 아래는 지움 ( 자막으로 판단..)
	for (int i = 0; i < peaks.size(); i++)
	{
		Mat binPeakImage;
		inRange(peaks[i].PeakImage, Scalar(1, 1, 1), Scalar(255, 255, 255), binPeakImage);
		Mat peakMask = imageHandler::removeNotPrimeryLyricLine(binPeakImage);
		//cvtColor(peakMask, peakMask, COLOR_GRAY2BGR);
		bitwise_and(peakMask, peaks[i].PeakImage, peaks[i].PeakImage);
	}

	Vec3b upColor;
	bool isFound = lineFinder.findUnprintColor(peaks, upColor);	// Unprint 컬러 파악 루틴

	Scalar UnprintColor = { (double)upColor[0], (double)upColor[1], (double)upColor[2] };
	color = UnprintColor;
	return isFound;
}
/*
	1. 백->흑으로 바뀌는 점이 많은 Frame 탐색
	 - Frame이 2000 이상이면서
	 - Frame-1 과 Frame+1 보다 10배이상 많은곳.
	2. 해당 Frame 기준으로 정보를 얻음
*/
MVInformation analyzer::findLineInfo(VideoCapture* videoCapture)
{
	MVInformation mvInfo;
	int type_PrintCount[3] = { 0, };
	int type_unPrintCount[2] = { 0, };
	
	vector<pair<int, int>> diffDotCount;	// <frame, count>
	vector<Scalar> vecPrintTypes;
	vecPrintTypes.push_back(Scalar(255, 0, 0));	// Blue
	vecPrintTypes.push_back(Scalar(0, 0, 255));	// Red
	vecPrintTypes.push_back(Scalar(255, 0, 255));	// Purple

	vector<Scalar> vecUnPrintTypes;
	vecUnPrintTypes.push_back(Scalar(255, 255, 255));	// white
	vecUnPrintTypes.push_back(Scalar(0, 255, 255));		// yellow(Orange)
	
	Mat stackBinImage;
	Mat stackBinImage_before, stackBinImage_diff;
	Mat FCImage_before;
	Mat subImage_before;
	videoHandler::printVideoSpec();

	Mat orgImage;

	while (videoCapture->read(orgImage))
	{
		videoHandler::printCurrentFrameSpec(*videoCapture);
		int curFrame = (int)videoCapture->get(CAP_PROP_POS_FRAMES);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)curFrame);

		Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);

		//Mat binImage = imageHandler::getPaintedBinImage(subImage);
		//imshow("subImage", subImage);
		//imshow("fullyContrastImage", fullyContrastImage);
		//imshow("binImage", binImage);
			   
		if (stackBinImage.empty() == true)
			stackBinImage = Mat::zeros(subImage.rows, subImage.cols, CV_8U);	//dummy
		if (FCImage_before.empty() != true)
		{
			stackBinImage_before = stackBinImage.clone();
			imageHandler::stackFCImage_BlackToWhite(subImage, subImage_before, stackBinImage);
			stackBinImage_diff = imageHandler::getWhiteToBlackImage(stackBinImage_before, stackBinImage);

			//	stackBinImage_diff = imageHandler::getBorderFloodFilledImage(stackBinImage_diff);
			//imshow("stackBinImage", stackBinImage);	// stackBinImage 이거의 화이트카운트
			//imshow("stackBinImage_diff", stackBinImage_diff);
			// stackBinImage_diff 노이즈 연산
			Mat diff_Denoise = imageHandler::removeNotLyricwhiteArea(stackBinImage_diff);
			//imshow("diff_Denoise", diff_Denoise);

			int dotCount = imageHandler::getWhitePixelCount(diff_Denoise);
			diffDotCount.push_back(make_pair(curFrame, dotCount));

			/* 다음 루틴 : */
			// Mat HorizontalProjection = imageHandler::getHorizontalProjection(stackBinImage_diff);
			// imshow("HorizontalProjection", HorizontalProjection);
			// vector<int> verticalProjectionData = imageHandler::getHorizontalProjectionData(stackBinImage_diff);

		}
		
		FCImage_before = fullyContrastImage.clone();
		subImage_before = subImage.clone();
	}

	vector<int> ExpectedFrame;		// 예상되는 프레임
	for (int i = 0; i < diffDotCount.size(); i++)
	{
		cout << "frame " << i << " : \t" << diffDotCount[i].second << endl;
		if (i != 0 && i < diffDotCount.size() - 1)
		{
			if (diffDotCount[i].second < 5000)	// 기본적으로 5000 이상인값중
				continue;

			if (diffDotCount[i - 1].second < diffDotCount[i].second / 2 &&
				diffDotCount[i + 1].second < diffDotCount[i].second / 2)	// 이전, 이후 프레임의 whiteCount보다 두배이상 많아야함
				ExpectedFrame.push_back(diffDotCount[i].first);

		}
	}

	vector<int> lineAppireCoordinate;
	for (int i = 0; i < stackBinImage.rows; i++)
		lineAppireCoordinate.push_back(0);

	cout << "ExpectedFrame size : " << ExpectedFrame.size() << endl;

	for (int i = 0; i < ExpectedFrame.size(); i++)
	{
		cout << "ExpectedFrame" << i << " : \t" << ExpectedFrame[i] << "\t";

		videoCapture->set(CAP_PROP_POS_FRAMES, (double)ExpectedFrame[i] - 2);
		videoCapture->read(orgImage);
		Mat subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
		Mat fullyContrastImage = imageHandler::getFullyContrastImage(subImage);
		Mat erodeImage_Denoise[3];
		int printTypePixelCount[3] = { 0, };	// [vecPrintTypes.size()] 
		// 라인 분석 루틴
		for (int i = 0; i < vecPrintTypes.size(); i++)	// 색깔 타입 검출
		{
			Mat PatternBin = imageHandler::getPaintedPattern(subImage, vecPrintTypes[i]);	// 내부에서 dilate함

			Mat FC_Bin;
			Scalar patternMin = vecPrintTypes[i];
			for (int i = 0; i < 3; i++)
				if (patternMin[i] != 0) patternMin[i] = patternMin[i] - 1;

			inRange(fullyContrastImage, patternMin, vecPrintTypes[i], FC_Bin);	// 파랑만이미지
			cvtColor(FC_Bin, FC_Bin, COLOR_GRAY2BGR);
			Mat PatternFullfill;
			PatternFullfill = imageHandler::getFloodfillImage(FC_Bin, PatternBin);	// FullCont 이미지에 패턴으로 인식한 좌표로 패인트동연산
			PatternFullfill = imageHandler::getBorderFloodFilledImage(PatternFullfill);
			Mat erodeImage = imageHandler::getMorphImage(PatternFullfill, MORPH_ERODE);	// 침식연산
			erodeImage_Denoise[i] = imageHandler::removeNotLyricwhiteArea(erodeImage);
			int pixelCount = imageHandler::getWhitePixelCount(erodeImage_Denoise[i]);

			printTypePixelCount[i] = pixelCount;
			// erode의 결과에서 contour 연산 추가하여 정확도 높일 수 있음
		}
		int type_Print = 0;
		for (int i = 0; i < 3; i++)
			if (printTypePixelCount[type_Print] < printTypePixelCount[i])
				type_Print = i;	// Print 타입 체크
		printf("%d	%d	%d	(type : %d)\r\n", printTypePixelCount[0], printTypePixelCount[1], printTypePixelCount[2], type_Print);
		if (printTypePixelCount[0] + printTypePixelCount[1] + printTypePixelCount[2] < 100)	// 점 합이 100 이하면 재대로된 라인이 아니라 판단하고 넘김
			continue;
		type_PrintCount[type_Print]++;

		// 라인 분석 루틴 : Unprint 색 검출
		/*	1. expectLine 의 마스크, expectLine과 expectLine-10 Frame
			2. 마스크지역의 변화 ( expectLine - expectLine-10f )
			3. 변한 지역이 흰색 or 노란색 인지 확인(type1 or type2)
		*/

		Mat fullyContrastImage_before;		
		{
			Mat middleImage;		// printTypePixelCount[type_Print] 의 50% 아래가 되는 프레임
			for (int j = 5; j < 10 * 25; j += 5)	// frame 5단위로 최대 10초까지 진행
			{
				videoCapture->set(CAP_PROP_POS_FRAMES, (double)ExpectedFrame[i] - j);	// 이부분 동적으로 적용해야함
				videoCapture->read(orgImage);
				subImage = imageHandler::getResizeAndSubtitleImage(orgImage);
				fullyContrastImage_before = imageHandler::getFullyContrastImage(subImage);
				// FC에서 type_print인 부분에 대한 흑백이미지 get
				Mat binTypeColor = imageHandler::getBinImageTargetColored(fullyContrastImage_before, vecPrintTypes[type_Print]);
				// getBinImage()
				Mat binAndInput = erodeImage_Denoise[type_Print];
				Mat binAndImage;
				bitwise_and(erodeImage_Denoise[type_Print], binTypeColor, binAndImage);
				// 해당이미지 BitwiseAnd erodeImage_Denoise
				int before_whitecount = imageHandler::getWhitePixelCount(binAndImage);
				if ((printTypePixelCount[type_Print] / 2) > before_whitecount)
				{
					printf("whiteCount(-%dframe) : %d %d \r\n", j, printTypePixelCount[type_Print], before_whitecount);
					Mat hProjection = imageHandler::getHorizontalProjection(binAndInput);

					vector<int> hData = imageHandler::getHorizontalProjectionData(binAndInput);
					int sum = 0;
					int hCount=0;
					int hAvg;
					for (int i = 0; i < hData.size(); i++)
					{
						if (hData[i] != 0)
						{
							sum += hData[i];
							hCount++;
						}
					}
					hAvg = sum / hCount;	// 평균
					for (int i = 0; i < hData.size(); i++)
					{
						hData[i] = hData[i] - hAvg;
					}

					vector<pair<int, int>> hIslands;
					int start = 0;
					for (int i = 0; i < hData.size(); i++)
					{
						if (hData[i] > 0)
						{
							if (start == 0)
								start = i;
						}
						else
						{
							if (start == 0)
								;
							else
							{
								hIslands.push_back(make_pair(start, i));
								start = 0;
							}
						}
					}

					for (int i = hIslands[0].first; i < hIslands[0].second; i++)
						lineAppireCoordinate[i]++;

					// 1. binAndInput 이거 흰점있는곳을 vecVertical에 ++해둠 
					// 1-1. 1의 결과에서 여러개의 섬 중 가장 규모가 큰2개의 섬만 남긴 후 위에 섬만 살림 (즉 한 frame에선 하나의 섬만 입력되도록)
					// 1-2. 그 결과에 해당하는 y좌표를 vecVertical에 ++해둠
					/*
						-> binAndInput에서 Vertical 누적함
						-> 0이 아닌값중 평균 이하 버림 (위에붙는 짜잘이 사라짐)
						-> 남은 값 중 섬이 2개면 위에 섬만 남김

						=> 누적 int[width] lineAppareCoordinate;
					*/
					
					// 2. 0이 아닌 vecVertical의 섬이 몇게인지 카운팅 -> 1개면 1라인, 2개면 TwinLine
					//

					break;
				}
			}
		}
		//Mat diffimage = imageHandler::getDifferenceImage(fullyContrastImage, fullyContrastImage_before);

		Mat mask = imageHandler::getMaskedImage(fullyContrastImage_before, erodeImage_Denoise[type_Print]);
		Mat printColorRemovedMask = imageHandler::getColorToBlackImage(mask, vecPrintTypes[type_Print]);// printColorRemovedMask

		Mat delateMask = imageHandler::getMorphImage(printColorRemovedMask, MORPH_ERODE);

		Vec3b mostRGB = imageHandler::getMostHaveRGB(printColorRemovedMask);	// mostHaveColor

		// 바뀐영역의 색 + type_print 색 하면  => 흰색인지 노랑색인지 알 수 있음
		Vec3b unPrintColor = mostRGB;
		//if(unPrintColor)

		int type_unPrint = 0;	// 0 = white, 1 = orange
		if (imageHandler::isBlack(unPrintColor))
			continue;
		else if (unPrintColor[0] == 0 &&
			unPrintColor[1] == 255 &&
			unPrintColor[2] == 255)
			type_unPrint = 1;
		else
			type_unPrint = 0;	// white

		printf("unPrint Color = %d %d %d  (type : %d)\r\n", unPrintColor[0], unPrintColor[1], unPrintColor[2], type_unPrint);
		// 가장 많은 경우의 수로 type 정함
		type_unPrintCount[type_unPrint]++;

		// 라인 분석 루틴 : TwinLine 검출
		/*	1. Print, UnPrint 색 알고있음, 
			2. ExpectedFrame에서 			
		*/
		
	}
	printf("Blue	Red	Purple	\r\n");


	vector<pair<int, int>> hIslands;
	int start = 0;
	for (int i = 0; i < lineAppireCoordinate.size(); i++)
	{
		if (lineAppireCoordinate[i] != 0)
		{
			if (start == 0)
				start = i;
		}
		else
		{
			if (start == 0)
				;
			else
			{
				hIslands.push_back(make_pair(start, i));
				start = 0;
			}
		}
		//printf("lineAppireCoordinate_hIslands : %d \r\n", hIslands.size());
	}


	// 길이 20 이하 ㄷ ㅏ짤라 & 
	for (vector<pair<int, int>>::iterator iter = hIslands.begin(); iter != hIslands.end(); )
	{
		if ((iter->second - iter->first) < 20)
		{
			printf("erase : %d ~ %d \r\n", iter->first ,iter->second);
			iter = hIslands.erase(iter);
			continue;
		}
		iter++;
	}

	cout << "h Islands size : " << hIslands.size() << endl;
	if (hIslands.size() == 0)
		return mvInfo;

	//printf("lineAppireCoordinate_hIslands : %d \r\n", hIslands.size());
	// 라인간 차이가 20이하면 합침
	sort(hIslands.begin(), hIslands.end(), asc);
	for (int i = 0; i < hIslands.size()-1; i++)
	{
		if (hIslands[i].second + 20 > hIslands[i+1].first)
		{
			hIslands[i].second = hIslands[i + 1].second;
			vector<pair<int, int>>::iterator iter = hIslands.begin();
			iter += i + 1;
			hIslands.erase(iter);
			i--;
		}
	}
	printf("lineAppireCoordinate_hIslands : %d \r\n", hIslands.size());
	switch (hIslands.size())
	{
	case 1:
		mvInfo.m_TwinLine = false;
		break;
	case 2:
		mvInfo.m_TwinLine = true;
		break;
	default:
		mvInfo.m_TwinLine = false;
		break;
	}
	// 라인 수

	if (type_PrintCount[2] > type_PrintCount[1])	// purple > red
		mvInfo.m_PrintType = PrintType::PINKnSKYBLUE;
	else
		mvInfo.m_PrintType = PrintType::REDnBLUE;

	if (type_unPrintCount[0] > type_unPrintCount[1])	// white > orange
		mvInfo.m_UnprintType = UnprintType::WHITE;
	else
		mvInfo.m_UnprintType = UnprintType::ORANGE;

	mvInfo.m_isinfoCatched = true;
	return mvInfo;
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
void analyzer::getJudgedLine()
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "getJudgedLine()";
	vector<int> peakValues;
	vector<Line> lines;
	// Peak 추출
	peakValues = getPeakFromWhitePixelCounts(vecWhitePixelCounts);

	for (int i = 0; i < peakValues.size(); i++)
	{
		printf("Peak %2d : %d\r\n", i, peakValues[i]);
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Peak " << i << " : " << peakValues[i];
	}

	string fileName = "peak.txt";
	fileManager::writeVector(fileName, peakValues);

	/* peaks -> lines */
	lines = getLinesFromPeak(peakValues, vecWhitePixelCounts);	// peak에서 라인 얻어내는데 시작지점이 이전피크보다 낮지 못하도록 설정되어있음.
	m_lyric.setLines(lines);

	// lineRejudgeByVerticalHistogramAverage(lines, verticalHistogramAverage);	// 왼쪽의 점 좌표 평균, 오른쪽의 점 좌표평균

	vector<string> lines_string;
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		string line_string = to_string(line->startFrame) + "\t" + to_string(line->endFrame);
		lines_string.push_back(line_string);
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
		if (vecWhitePixelCounts[frameNum] < 100)		// 100 이하면 ZeroZone 으로
		{
			isZeroZone = true;
		}
		else if (vecWhitePixelCounts[frameNum-1] > 1000)	// 100 이상이면서, 이전값이 1000이상임 
		{
			if ((vecWhitePixelCounts[frameNum - 1] / 10)*5 > vecWhitePixelCounts[frameNum])	// 50%이상 급락시 Zero Zone으로 판단
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

				peakValues.push_back(maxValueFrame);
				if(vecWhitePixelCounts[maxValueFrame]/2 > vecWhitePixelCounts[frameNum-1])	// 전프레임의 흰점수 < 최대값/2 인경우 맥스값 넣음
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

/// <summary>
/// 피크와 흰 점의 개수들을 이용하여 가사 Line의 시작점-끝점을 반환
/// </summary>
/// <param name="vecWhitePixelCounts">">프래임 별 흰 점의 테이터</param>
/// <param name="peaks">피크</param>
vector<Line> analyzer::getLinesFromPeak(vector<int> peaks, vector<int> vecWhitePixelCounts)
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "getLinesFromPeak()";
	vector<Line> lines;
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
		lines.push_back(line);
	}

	return lines;
}

/// <summary>
/// 가사 라인들 중 특정 프래임동안 지속되지 않는 line을 걸러냄
/// </summary>
/// <param name="fps">The FPS.</param>
void analyzer::linesRejudgeByLineLength(int fps)
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "linesRejudgeByLineLength()";
	for (int i =0; i<m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		printf("lines (%d - %d )\r\n", line->startFrame, line->endFrame);

		if (lineRejudgeByLineLength(line->startFrame, line->endFrame, fps) == false)
		{
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "exceptionLine: " << line->startFrame <<"-"<< line->endFrame;
			printf("exceptionLine: (%d - %d)\r\n", line->startFrame, line->endFrame);
			line->isValid = false;
		}
		else
			line->isValid = true;
	}
	m_lyric.cleanupInvalidLines();
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
	m_lyric.cleanupInvalidLines();

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
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "calibrateLines()";
	//int invaildCount = 0;

	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		int minFrame;
		if (i > 0)
			minFrame = m_lyric.getLine(i - 1)->endFrame;
		else
			minFrame = 0;

		printf("Cal Line%d : %d - %d\r\n", i, line->startFrame, line->endFrame);
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Cal Line" << i << ":" << line->startFrame << "-"<< line->endFrame;
		
		if (lineCalibration(*line, minFrame) == false)
		{
			line->isValid = false;
			printf("Caled Line%d is invaild will be removed.\r\n", i);
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Caled Line" << i << " is invaild will be removed.";
			//invaildCount++;
		}
		else
		{
			imwrite(fileManager::getSavePath() + "/Captures/DebugT2_" + to_string(line->endFrame) + "_imageToSubBinImage.jpg", line->maskImage);

			line->isValid = true;
			printf("Caled Line%d : %d - %d\r\n", i, line->startFrame, line->endFrame);
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Caled Line" << i << ": " << line->startFrame << "-" <<line->endFrame;

			//catpureBinaryImageForOCR(line->maskImage.clone(), i-invaildCount, fileManager::getSavePath());
		}
	}
	m_lyric.cleanupInvalidLines();
	   	
	linesRejudgeByLineLength();

}

/// <summary>
/// 라인의 끝점과 시작점이 정확한 frame번호를 갖고 maskimage를 얻기 위함
/// </summary>
bool analyzer::lineCalibration(Line& line, static int minStartFrame)
{
	bool isEffictiveLine = true;
	Mat readImage;
	videoCapture->set(CAP_PROP_POS_FRAMES, (double)line.endFrame-1);
	videoCapture->read(readImage);
	Mat sourceImg = imageHandler::getResizeAndSubtitleImage(readImage);
	Mat maskImage;
	if (USE_ML)	
	{
		Mat deblurImg = getDeblurImage(sourceImg, line.endFrame - 1);	
		maskImage = imageToSubBinImage(deblurImg);
	}
	else
	{
		maskImage = imageToSubBinImage(sourceImg);
	}

	imwrite(fileManager::getSavePath()+"/Captures/DebugT1_" + to_string(line.endFrame-1) + "_imageToSubBinImage.jpg", maskImage);
	maskImage = imageHandler::getNoiseRemovedImage(maskImage, true);

	int maskImage_leftDot_x = imageHandler::getLeftistWhitePixel_x(maskImage);
	int maskImage_rightDot_x = imageHandler::getRightistWhitePixel_x(maskImage);
	int maskImage_middleDot_x = (maskImage_leftDot_x + maskImage_rightDot_x) / 2;
	int image_center_x = maskImage.cols / 2;

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

		Mat lyricMask_not;
		bitwise_not(lyricMask, lyricMask_not);
		Mat otherSide = getBinImageByFloodfillAlgorism(maskImage, lyricMask_not);
		Mat otherSide_not;
		bitwise_not(otherSide, otherSide_not);
		bitwise_and(otherSide_not, maskImage, maskImage);

		maskImage_leftDot_x = imageHandler::getLeftistWhitePixel_x(maskImage);
		maskImage_rightDot_x = imageHandler::getRightistWhitePixel_x(maskImage);
		maskImage_middleDot_x = (maskImage_leftDot_x + maskImage_rightDot_x) / 2;
	}

	Mat maskImage_back = maskImage.clone();
	maskImage_back = imageHandler::removeSubLyricLine(maskImage);
	maskImage = imageHandler::removeNotPrimeryLyricLine(maskImage);	// maskImage 

	int maskImage_pixelCount = imageHandler::getWhitePixelCount(maskImage);
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
	int startframe_org = line.startFrame;
	
	int frameIndex = line.endFrame;
	int expectedRightistPoint = maskImage_rightDot_x;
	bool underMiddlePoint = false;

	// 3. 진행도 측정 -> 라인의 시작과 끝점 파악
	while (true)	
	{

		videoCapture->set(CAP_PROP_POS_FRAMES, (double)frameIndex-1);	// frameIndex-1
		videoCapture->read(readImage);

		Mat subImage = imageHandler::getResizeAndSubtitleImage(readImage); 
		//Mat binCompositeImage = imageHandler::getCompositeBinaryImages(subImage);
		Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색(파랑, 빨강), 
		cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);
		inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);		//파, 빨
		subImage_hsv = imageHandler::getBorderFloodFilledImage(subImage_hsv);

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

		int pixelCount = imageHandler::getWhitePixelCount(binImage);
		int diffImage_rightistPoint = imageHandler::getRightistWhitePixel_x(image_dilation);//getWhitePixelAverage(image_dilateion);	
		int diffImage_rightistPoint_Per = (int)((diffImage_rightistPoint - maskImage_leftDot_x) / ( (maskImage_rightDot_x - maskImage_leftDot_x) / 100.0));


		printf("frame %d - diffAvgPoint: %d, diffImage_avgPoint_Per: %d \r\n", frameIndex, diffImage_rightistPoint, diffImage_rightistPoint_Per);
		printf("frame %d - pixelCount: %d \r\n", frameIndex, pixelCount);
		
		if (MinimumPixelCount > pixelCount)
		{
			MinimumPixelCount = pixelCount;
			line.startFrame = frameIndex + 1;
		}
		if (MinimumPixelCount < 100 && pixelCount>1000)	// 최소 픽셀카운트가 100이다가 갑자기 증가할때
			break;

		if (diffImage_avgPoint_PerMax <= diffImage_rightistPoint_Per)	// 가장 큰 Percent 변경점이 시작점, => 마스크 가장 오른쪽 점 
		{					
			line.endFrame = frameIndex + 1;
			diffImage_avgPoint_PerMax = diffImage_rightistPoint_Per;
		}
	
		beforeBinImage = binImage.clone();

		if (minStartFrame == frameIndex)
			break;

		frameIndex --;		
		if (pixelCount == 0 || (pixelCount<100&&diffImage_rightistPoint_Per==0))
			break;

	}
	
	if (isEffictiveLine)		
	{
	//	printf("StartFrame : %d \r\n", line.startFrame -1);
	//	videoCapture->set(CAP_PROP_POS_FRAMES, (double)line.startFrame - 2);	// frameIndex-1
	//	videoCapture->read(readImage);

	//	subImage = imageHandler::getResizeAndSubtitleImage(readImage);
	//	// 1. 흰색의 이진화한 이미지 구함
	//	// 2. floodfill (maskImage, 1번한것)
	//// inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), image_bin_inRange);	
	//	inRange(subImage, Scalar(190, 190, 190), Scalar(255, 255, 255), binImage);
	//	maskImage = getBinImageByFloodfillAlgorism(maskImage_back, binImage);
	//	line.maskImage = maskImage;
		line.maskImage = maskImage_back;
	}	// 


	// 마지막으로 유효한 라인인지 걸러냄 - pixel Count 대신에 diffImage_avgPoint_Per 의 연속적인 하락의 개수로 판별하는건 어떨까
	maskImage_pixelCount = imageHandler::getWhitePixelCount(maskImage);
	if (maskImage_pixelCount < 500)	// 마스크 픽셀이 500개 이하
	{
		// 시작지점이 화면 전채에 대한 fade-in 인 경우를 방지하는 코드
		vector<Mat> chennels;
		split(subImage, chennels);
		equalizeHist(chennels[0], chennels[0]);
		equalizeHist(chennels[1], chennels[1]);
		equalizeHist(chennels[2], chennels[2]);
		Mat merged;
		merge(chennels, merged);
		inRange(merged, Scalar(230, 230, 230), Scalar(255, 255, 255), binImage);
		binImage = imageHandler::getMorphImage(binImage, MorphTypes::MORPH_ERODE);
		maskImage = getBinImageByFloodfillAlgorism(maskImage_back, binImage);
		line.maskImage = maskImage;

		maskImage_pixelCount = imageHandler::getWhitePixelCount(maskImage);
		if (maskImage_pixelCount < 500)	// 마스크 픽셀이 500개 이하
		{
			isEffictiveLine = false;
			printf("MaskImage Info - whiteCount: %d \r\n", maskImage_pixelCount);
			printf("line Exception : MaskImage whiteCount is 500 Under.\r\n");
			return isEffictiveLine;
		}
	}

	if (lineRejudgeByLineLength(line.startFrame, line.endFrame) == false)	// 길이가 400 이하 
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
void analyzer::captureLines()
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "captureLines()";
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		Mat startImage, endImage;
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)line->startFrame - 2);
		videoCapture->read(startImage);
		videoCapture->set(CAP_PROP_POS_FRAMES, (double)line->endFrame);	// -1
		videoCapture->read(endImage);
		imwrite(fileManager::getSavePath() + "/Captures/Line" + to_string(i) + "_Start.jpg", startImage);
		imwrite(fileManager::getSavePath() + "/Captures/Line" + to_string(i) + "_End.jpg", endImage);

		catpureBinaryImageForOCR(line->maskImage.clone(), i, fileManager::getSavePath());
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
		captureBinaryImage(videoPath, i, subBinImage);
	}
}

void analyzer::captureBinaryImage(string videoPath, int index, Mat image)
{

	imwrite(videoPath + "/Captures/Line" + to_string(index) + "_Bin.jpg", image);
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
	//Mat binCompositeImage = imageHandler::getCompositeBinaryImages(subImage);
	Mat justFullContrastImage = imageHandler::getFullyContrastImage(targetImage);		// FC이미지는 SC보다 Painted컬러를 잘 살려냄
	Mat sharpenContrastImage = imageHandler::getSharpenAndContrastImage(targetImage);	// SC이미지는 FC보다 흰색 태두리를 더 잘 살려냄
	// justFullContrastImage에 sharpenContrastImage에 흰색인 곳의 좌표에 흰색처리함
	Mat mixedImage = justFullContrastImage.clone();
	Vec3b whiteColor;
	whiteColor = { 255, 255, 255 };

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

	Mat printImage_blue = imageHandler::getPaintedBinImage_inner(mixedImage, true);
	Mat printImage_red = imageHandler::getPaintedBinImage_inner(mixedImage, false);
	printImage_blue = imageHandler::getMorphImage(printImage_blue, MorphTypes::MORPH_DILATE);
	printImage_red = imageHandler::getMorphImage(printImage_red, MorphTypes::MORPH_DILATE);

	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))	// 채도가 255가까울수록 단색(파랑, 빨강), 
	cvtColor(targetImage, subImage_hsv, COLOR_BGR2HSV);
	inRange(subImage_hsv, Scalar(0, 170, 100), Scalar(255, 255, 255), subImage_hsv);		//파, 빨 (단색)

	Mat printImage_blue_filled = getBinImageByFloodfillAlgorism(subImage_hsv, printImage_blue);
	Mat printImage_red_filled = getBinImageByFloodfillAlgorism(subImage_hsv, printImage_red);
	
	Mat printImage_blue_filled_NR = imageHandler::getDotRemovedImage(printImage_blue_filled);
	Mat printImage_red_filled_NR = imageHandler::getDotRemovedImage(printImage_red_filled);

	int alignContoursCount_blue = imageHandler::getAlinedContoursCount(printImage_blue_filled_NR);
	int alignContoursCount_red = imageHandler::getAlinedContoursCount(printImage_red_filled_NR);
	
	Mat image_out;
	Mat biasedImage;
	if (alignContoursCount_blue > alignContoursCount_red)
	{
		biasedImage = imageHandler::getBiasedColorImage(targetImage, Color::BLUE); //image_out = printImage_blue_filled;
		image_out = getBinImageByFloodfillAlgorism(biasedImage, printImage_blue_filled_NR);
		image_out = imageHandler::getMorphImage(image_out, MORPH_OPEN);
		image_out = getBinImageByFloodfillAlgorism(image_out, printImage_blue_filled_NR);
	}
	else
	{
		biasedImage = imageHandler::getBiasedColorImage(targetImage, Color::RED); //image_out = printImage_red_filled;
		image_out = getBinImageByFloodfillAlgorism(biasedImage, printImage_red_filled_NR);
		image_out = imageHandler::getMorphImage(image_out, MORPH_OPEN);
		image_out = getBinImageByFloodfillAlgorism(image_out, printImage_red_filled_NR);
	}

	image_out = imageHandler::getBorderFloodFilledImage(image_out, true);
//	image_out = imageHandler::getMorphImage(image_out, MORPH_CLOSE);	// 삭제

	Mat binCompositeImage = imageHandler::getCompositeBinaryImages(targetImage);
	image_out = getBinImageByFloodfillAlgorism(image_out, binCompositeImage);

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
void analyzer::capturedLinesToText()
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "capturedLinesToText()";
	// "Output/Captures/LineX_bin.jpg"
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		string targetPath = fileManager::getSavePath() + "Captures/Line" + to_string(i) + "_Bin.jpg";	// ./Output/Captures/
		if (fileManager::isExist(targetPath) != true)
		{
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << targetPath<<" is not exist.";
			printf("%s is not exist. \r\n", targetPath.c_str());
			continue;
		}

		string desName = fileManager::getSavePath() + "Lines/Line" + to_string(i);
		OCRHandler ocrHandler;
		ocrHandler.runOCR(targetPath, desName);
	}
}

void analyzer::readLyricsFromFile()
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "readLyricsFromFile()";
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		string lineFileName = fileManager::getSavePath() + "/Lines/Line" + to_string(i) + ".txt";	// YS - 에러 발생시킴
		Line* line = m_lyric.getLine(i);
		if (fileManager::isExist(lineFileName) != true)
		{
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << lineFileName <<" is not exist.";
			printf("%s is not exist. \r\n", lineFileName.c_str());

			//continue;	// 에러 처리 필요			
		}
		string ocredText;
		fileManager::readLine(lineFileName, ocredText);

		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Read text: " << ocredText;
		line->text = ocredText;
	}
}

bool analyzer::setVideo(string videoPath)
{
	return videoHandler::setVideo(videoPath);
}

void analyzer::closeVideo()
{
	videoHandler::closeVideo();
	videoCapture = nullptr;
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
void analyzer::wordCalibration()
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "wordCalibration()";
	for (int i = 0; i < m_lyric.getLinesSize(); i++)
	{
		Line* line = m_lyric.getLine(i);
		wordCalibrationByOCRText(*line);
	}
}

/*
1. 마스크 이미지(흑백)에서 가로축으로 모든 섬들을 구함
2. OCR 결과를 ' ' 단위로 분리
3. getPaintedPoint()로 패인팅되는 좌표를 구함 (색깔로 연산하기 때문에 바꿔야함)
*/
void analyzer::wordCalibrationByOCRText(Line& line)
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "wordCalibrationByOCRText()";
	cout << "wordCalibrationByOCRText() : \r\n";
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

	sort(spacesLength.begin(), spacesLength.end(), desc);	
	
	vector<string> tokens;	// OCR out text에서 ' ' 단위로 분리해낸 string
	boost::split(tokens, line.text, boost::is_any_of(" ")); 
	vector<Word> words;		// 워드 text, frame start-end를 가지는 객체

	printf("line.text : %s \r\n", line.text);
	for (int i = 0; i < tokens.size(); i++)
	{
		Word word;
		//if (tokens[i].find_first_not_of(' ') != std::string::npos)	// ' '가 없다면 npos를 반환함
		{
			// There's a non-space.
			printf("token %d : %s \r\n", i, tokens[i]);
			word.text = tokens[i];
			words.push_back(word);
		}
	}	// 가사 먼저 넣음.

	//vector<pair<int, int>> spacesLength_cut;	// <index, length>
	//spacesLength_cut.assign(spacesLength.begin(), spacesLength.begin() + words.size());

	printf("words.size() : %d\r\n", words.size());
	printf("spacesLength.size() : %d\r\n", spacesLength.size());
	for (int i = 0; i < static_cast<int>(words.size()-1); i++)	// token만큼의 스페이스바 - word 수가 더 많으면 에러 발생 YS
	{
		if (spacesLength.size() < words.size()-1)
			break;
		int startFrame = spaces[spacesLength[i].second].first;  // [index].first
		int endFrame = spaces[spacesLength[i].second].second;

		line.spacingWords.push_back((endFrame-startFrame)/2 + startFrame);
		cout << i << "\n";
	}
	sort(line.spacingWords.begin(), line.spacingWords.end());

	vector<pair<int, int>> paintedPoint;	// ( pair<frame, xPoint> )
	paintedPoint = getPaintedPoint(line);	

	words[0].startFrame = line.startFrame;			// 배열 첫번째에 시작점 넣음
	words[words.size()-1].endFrame = line.endFrame; // 배열에 마지막에 끝점 넣음

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

Mat analyzer::getDeblurImage(Mat sourceImage, int frameNum)
{
	/*
	1. 이미지 저장
	2. python 실행
	3. 이미지 로드
	*/
	imwrite("ML\\deBlur_before.jpg", sourceImage);
	system("python ML\\deblur.py ML\\deBlur_before.jpg");
	
	// read file last line
	ifstream readFile;
	string lastLine;
	readFile.open("ML\\PythonLog.txt");
	if (readFile.is_open())
	{
		while (!readFile.eof())
		{
			string tmp;
			getline(readFile, tmp);
			cout << "line:: " <<tmp << endl;
			if(tmp.length()>1)
				lastLine = tmp;
		}
	}
	cout << "last line : "<< lastLine << endl;

	bool isSucess = false;
	if (string::npos != lastLine.find("successed"))
		isSucess = true;
	
	if (isSucess)
	{
		Mat deBlurImage = imread("ML\\deBlur_after.png");
		imwrite(fileManager::getSavePath() + "\\Captures\\ML_" + to_string(frameNum) + "_imageToSubBinImage.jpg", deBlurImage);
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "python deblur sucessed";
		return deBlurImage;
	}
	else
	{
		// python deblur Failed!
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "python deblur Not sucessed";
		return sourceImage;
	}
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



