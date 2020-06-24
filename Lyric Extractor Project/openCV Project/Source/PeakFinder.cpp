#include "PeakFinder.h"
#include "loger.h"

const int PeakFinder::JUDGE_TIMEOUT = 5;

vector<contourLineInfoSet> PeakFinder::frameImage_process(Mat frameImage, int frameNumber, Scalar targetColor, Mat refUnprintImage)
{	
	Mat patternFill = imageHandler::getPatternFillImage_2(frameImage, targetColor);		// 노이즈 제거한거
	Mat patternFill_RemoveDepthContour = imageHandler::getDepthContourRemovedMat(patternFill); // patternFill 에서 뎁스가 1이상인 곳 삭제 (O 안에 노이즈가 있을 때 문제 발생가능.. )
	//Mat test;
	//inRange(patternFill_RemoveDepthContour, 1, 255, test);
	//imshow("patternFill_RemoveDepthContour", patternFill_RemoveDepthContour);

	Mat FC_Bin = imageHandler::getFillImage(frameImage, targetColor);
	inRange(FC_Bin, Scalar(254, 254, 254), Scalar(255, 255, 255), FC_Bin);	// to 1 demend
	Mat refPatternStack = FC_Bin;//accMat.accumulateProcess(FC_Bin);	// 노이즈 제거안한거

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
		// 원래
//		if (frameNumber == 3158)
//		{
//			Mat test_refUnprintImage = refUnprintImage;
//			Mat test1;
//		//	Mat test2;
//			inRange(m_expectedLineInfos[0].first.progress.weightMat.binImage, 1, 255, test1);
//		//	inRange(m_expectedLineInfos[2].first.progress.weightMat.binImage, 1, 255, test2);
//		}
//		if (m_expectedLineInfos.size() != 0)
//		{
//			Mat test1;
//			inRange(m_expectedLineInfos[0].first.progress.weightMat.binImage, 1, 255, test1);
//
//		}
		//Mat test_m_stackBinImage = m_stackBinImage.clone();
		
		Mat bin_refUnprintImage;
		m_stackBinImage = stackBinImage(m_stackBinImage, patternFill_RemoveDepthContour, refUnprintImage);	// patternStack도 사용할수있음
		// 이 이미지를 통하여 컨투어 판단을 하고 라인으로 처리함

#ifdef _DEBUG
		Mat stackBinimage = m_stackBinImage;
		Mat stackBinimage_t;
		inRange(m_stackBinImage, 1, 255, stackBinimage_t);
#endif
		//m_stackBinImage = stackBinImage2(m_stackBinImage, refPatternStack, refUnprintImage);
		//m_stackBinImage = stackBinImage_noiseRemove(m_stackBinImage, patternFill);
		
		// m_stackBinImage , patternFill에다가 노이즈제거 하고 0인것을 m_stackBinImage에다가 적용 
		// 조건 : m_stackBinImage[x]가 10 이상, patternFill[x]가 0

		//m_stackBinImage = imageHandler::getBorderFloodFilledImage(m_stackBinImage);

		//m_weightSumAtBinImage = imageHandler::getSumOfBinImageValues(m_stackBinImage);
		//m_colorPixelSumAtBinImage = imageHandler::getWhitePixelCount(m_stackBinImage);

		// 추가 연산 : 
		if(0)//if (m_expectedLineInfos.size() != 0)
		{
			Mat expectedLineInfosMask = Mat::zeros(m_stackBinImage.rows, m_stackBinImage.cols, CV_8U);;
			for (int i = 0; i < m_expectedLineInfos.size(); i++)
			{
				Mat temp;
				inRange(m_expectedLineInfos[i].first.maximum.weightMat.binImage, 1, 255, temp);
				bitwise_or(expectedLineInfosMask, temp, expectedLineInfosMask);				
			}
			Mat exceptionMask;	// 살려야 할 부분
			Mat WhiteToBlackMat = imageHandler::getWhiteToBlackImage(FC_Bin, patternFill);
			bitwise_and(WhiteToBlackMat, expectedLineInfosMask, exceptionMask);
			if (imageHandler::getWhitePixelCount(exceptionMask) != 0)
			{
				Mat neverZeroMat;	// 죽이면 안되는부분
				bitwise_and(FC_Bin, patternFill, neverZeroMat);

				Mat matmat;
				bitwise_or(exceptionMask, neverZeroMat, matmat);

				bitwise_and(m_stackBinImage, matmat, m_stackBinImage);	//라인이 사라졌을때만 이걸 수행해야함
			}
		}

		makeContourMaxBinImageAndContourInfos();
		makeExpectedLineInfos();
		printf("[found CLine %d]", m_contourMaxBinImage_expectedLineInfo.size());
		printf("[ex CLine %d]", m_expectedLineInfos.size());
#ifdef _DEBUG
		if (m_expectedLineInfos.size() != 0)
		{
			printf("[%d]", imageHandler::getLeftistWhitePixel_value(m_expectedLineInfos[0].first.progress.weightMat.binImage));
		}
#endif
		
		foundExpectedLines = getJudgeLineByFrameFlow();	// 
		if (foundExpectedLines.size() != 0)
		{
			for (int i = 0; i < foundExpectedLines.size(); i++)
			{
				;
				foundExpectedLines[i].maximum = removeLeftNoise(foundExpectedLines[i].maximum);
				//
			}
			//removeLeftNoise

		}
	}
	Mat m_stackBinImage_debug;
	inRange(m_stackBinImage, 1, 255, m_stackBinImage_debug);

	return foundExpectedLines;
}


// x라인이 확정되면 확정된 이미지보다 여전히 큰 값을 가지는 점을 보정해주어 
// 라인이 확정되면 해당하는 y축의 binImage를 0으로 만듦.
// 이 후 라인 판별될 때 앞에 라인까지 잡아먹는 현상을 제거하기 위함
void PeakFinder::stackBinImageCorrect(contourLineInfoSet lineSet)//Mat validImage)
{
	Mat corrImage = m_stackBinImage.clone();
	int height = corrImage.rows;
	int width = corrImage.cols;

	Mat validImage = lineSet.progress.weightMat.binImage;

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
				// 결과물에서 컨투어 딴
			}
		}
	}

	// 대상 : corrImage
	// 필요한것: corrImage의 컨투어들, lineSet.progress.weightMat.binImage의 binImage
	// 할것 : corr각각의 컨투어의 점들이 위 이미지의 흰점에만 존제한다면 삭제함.
	Mat corrImage_bin;
	inRange(corrImage, 1, 255, corrImage_bin);
	vector<vector<Point>> contours;
	findContours(corrImage_bin, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(m_stackBinImage.rows, m_stackBinImage.cols, CV_8U);;
		// 컨투어별 마스크
		vector<vector<Point>> contours_picked;
		contours_picked.push_back(contours[i]);
		fillPoly(contourMask, contours_picked, 255);
		vector<Point> indices;		// 내부의 점들을 얻음
		bitwise_and(contourMask, corrImage_bin, contourMask);
		findNonZero(contourMask, indices);

		bool isHaveOnlyIn = true;
		for (int idx = 0; idx < indices.size(); idx++)
		{
			uchar* yPtr_val = validImage.ptr<uchar>(indices[idx].y);	// 이전에 확정난 이미지
			uchar* yPtr_con = contourMask.ptr<uchar>(indices[idx].y);	//확인해야할
			//sum += yPtr[indices[idx].x];
			if (yPtr_val[indices[idx].x]==0)	// 다른곳에 점이 있다.
			{
				isHaveOnlyIn = false;	// 유지함
				break;
			}
		}
		if (isHaveOnlyIn == true)
		{
			floodFill(corrImage_bin, indices[0], 0);
			// 지워버림
		}
	}
	bitwise_and(corrImage, corrImage_bin, corrImage);


	// 해당 Y 축에 5 이상인 값들 전부 삭제.
	int removeY_start = imageHandler::getHighistWhitePixel_y(lineSet.maximum.weightMat.binImage);//lineSet.maximum.coorY_start;	// 적용할것
	int removeY_end = imageHandler::getLowistWhitePixel_y(lineSet.maximum.weightMat.binImage);//lineSet.maximum.coorY_end;

	for (int y = removeY_start; y < removeY_end; y++)
	{
		uchar* yPtr_corr = corrImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if(yPtr_corr[x]> JUDGE_TIMEOUT)
				yPtr_corr[x] = 0;
		}
	}
	m_stackBinImage = corrImage.clone();
}

// patternImage에 대하여 추가 연산 필요 ()
Mat PeakFinder::stackBinImage(Mat stackBinImage, Mat patternImage, Mat refUnprintImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	Mat bin_refUnprintImage;
	inRange(refUnprintImage, 0, 4, bin_refUnprintImage);// 최근 3프래임중 흰색이었던곳
	   
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_stack = stackBinImage.ptr<uchar>(y);
		uchar* yPtr_pattern = patternImage.ptr<uchar>(y);
		uchar* yPtr_refUnprint = bin_refUnprintImage.ptr<uchar>(y);

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
				yPtr_stack[x] = 1;	// 시작조건
			}

		}
	}
	return stackBinImage;
}

// refUnprintImage가 [0,255] 가 아닌 unPaint weight 를 받음
// patternImage[x]와 refUnpaintImage[x]의 차이가 +-2 일 경우 웨이트 값 부여
// stack 이미지에 웨이트값 누적,
Mat PeakFinder::stackBinImage2(Mat stackBinImage, Mat patternImage, Mat refUnprintImage)
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
			// 패턴값과 Unpaint 값의 차이가 +-2임
			// 패턴이 흰색 and Unprint가 흰색이었던곳 -> 1처리 (시작)
			// 스택이미지가 이미 칠해져 있으면서 패턴인곳

			bool isPatternDot;

			int dif = yPtr_refUnprint[x] - yPtr_pattern[x];
			if (dif < 3 && dif > 0)	// +- 2 인 곳				
			//	&& (abs(yPtr_stack[x] - yPtr_pattern[x]) < 10)// stackImage 와 비교했을 때 차이가 크지 않은 것 	
			//	)
//			if(yPtr_refUnprint[x] == yPtr_pattern[x]
//				&& (yPtr_refUnprint[x] - yPtr_pattern[x] < 5)//  	
//				)
			{
				isPatternDot = true;
			}
			else
			{
				isPatternDot = false;
			}

			if (yPtr_pattern[x] == 255 && yPtr_refUnprint[x] == 255)
				yPtr_stack[x] == yPtr_stack[x];	// 둘다 255 이면 255프래임동안 색칠된게 유지되었거나, 초기화된 상황임(에러 처리 필요!)
			else if (isPatternDot == true && yPtr_stack[x] != 255)
				yPtr_stack[x]++;
			else
				yPtr_stack[x] = 0;

			//

		}
	}	// ysys
	return stackBinImage;
}

Mat PeakFinder::stackBinImage_noiseRemove(Mat stackBinImage, Mat fillImage)
{
	Mat PatternFullfill;
	PatternFullfill = imageHandler::getBorderFloodFilledImage(fillImage);
	Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(PatternFullfill);	// 사각박스있는곳 제거

	Mat outImage = stackBinImage.clone();

	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_stack = outImage.ptr<uchar>(y);
		uchar* yPtr_noiseRemoved = erodeImage_Denoise.ptr<uchar>(y);	// 0 이면 노이즈 제거된 곳임

		for (int x = 0; x < width; x++)
		{
			if (yPtr_stack[x] >= 10 && yPtr_noiseRemoved[x] == 0)	// stackbin[x]가 10이상이면서 노이즈 제거된 부분 제거
			{
				yPtr_stack[x] = 0;
			}
		}
	}

	return outImage;

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
		
		vector<int> includeValues;
		for (int idx = 0; idx < indices.size(); idx++)	// 해당 컨투어에 맥스값을 칠함
		{
			uchar* yPtr_stack = m_stackBinImage.ptr<uchar>(indices[idx].y);	//in
			uchar* yPtr_out = outImage.ptr<uchar>(indices[idx].y);	//in
			//yPtr[indices[idx].x] = avgContourColor;
			//yPtr_out[indices[idx].x] = max;						// 이거 하지않는것은..?	 YS-TAG : max 값이 아닌 가지고 있는 값들의 배열을 갖음
			yPtr_out[indices[idx].x] = yPtr_stack[indices[idx].x];	// 원본복사
			includeValues.push_back(yPtr_stack[indices[idx].x]);
		}
		includeValues = contourInfo::includeValuesDeduplicate(includeValues);
		contourInfo conInfo = getContourInfoFromPixels(indices);
		//conInfo.maxValue = max;
		conInfo.includeValues = includeValues;
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
	// ys-tag
	vector<contourLineInfo> conLineInfos;	// Expect contour Line
	
		;	// contourLineInfo 생성 루틴
			// 2.1 전채순회하여 모든 연결 구함 (모든 컨투어 진행)
			// 2.2 
		for (int i = 0; i < contourInfos.size(); i++)	// contourInfos 는 x_start좌표 기준으로 정렬되어있음
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
						for (int rec = 0; rec < contourLineInfo.contours.size(); rec++)	// 이전값이 값이 넣으려는 값보다 큰게 있어야함 AND X값이 멀지않아야함
						{
							if (contourInfos[j].getMaxValue() < contourLineInfo.contours[rec].getMaxValue()	// weight가 더 큰게 있어야함
								&& contourInfos[j].coorX_start < contourLineInfo.contours[rec].coorX_end+200 ) // X값이 +200pix 안에 있어야함 
							{
								contourLineInfo.contours.push_back(contourInfos[j]);
								contourInfos[j].isRefed = true;
								break;
							}
						}
					}
					contourLineInfo.pixelCount = imageHandler::getContourLineInfoVolume(contourLineInfo);
				}

				conLineInfos.push_back(contourLineInfo);
			}
		}	

		for (int i = 0; i < conLineInfos.size(); i++)
		{
			conLineInfos[i].maxValue = conLineInfos[i].getMaxValue();
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
	else
		m_contourMaxBinImage_expectedLineInfo.clear();

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
	for (int i = 0; i < conLineInfos.size(); i++)	// 2. -> 3번이 먼저 해야 하지 않을까..?
	{
		bool isPassOnSize = false;
		bool isPassOnVolume = false;
		bool isPassOnMaxWeight = false;
		if (conLineInfos[i].contours.size() >= 2)		// ys - 이걸로 안되면 다른방법 강구 : includeValue..
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

	for (int i = 0; i < conLineInfos.size(); i++)	// 3. Y 기준으로 머지
	{
		bool isMerged = false;
		for (int j = 0; j < conLineInfos_buffer.size(); j++)
		{
			// 머지조건 : Y축 겹침 AND X축이 앞선것이 max값이 크거나 같음
			if (imageHandler::isRelation(conLineInfos_buffer[j].coorY_start, conLineInfos_buffer[j].coorY_end, conLineInfos[i].coorY_start, conLineInfos[i].coorY_end)
				&& (((conLineInfos_buffer[j].coorX_start > conLineInfos[i].coorX_start) && (conLineInfos_buffer[j].getMaxValue() <= conLineInfos[i].getMaxValue()))
				|| ((conLineInfos_buffer[j].coorX_start < conLineInfos[i].coorX_start) && (conLineInfos_buffer[j].getMaxValue() >= conLineInfos[i].getMaxValue())))
				)
			{
				if (conLineInfos_buffer[j].coorX_start > conLineInfos[i].coorX_start)
					conLineInfos_buffer[j].coorX_start = conLineInfos[i].coorX_start;

				if (conLineInfos_buffer[j].coorX_end < conLineInfos[i].coorX_end)
					conLineInfos_buffer[j].coorX_end = conLineInfos[i].coorX_end;

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
			if (a.coorX_start == b.coorX_start &&
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
		if (conLineInfos[i].contours.size() >= 2)
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
	vector<bool> foundLineRelated;	// m_contourMaxBinImage_expectedLineInfo (이번에 찾은 라인이 참조되었는지에 대한 정보 )
	for (int i = 0; i < m_contourMaxBinImage_expectedLineInfo.size(); i++)
		foundLineRelated.push_back(false);

	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 1.
	{
		// 원인 : 살아있는 라인과 관련된 라인이 2개인데 (Y좌표 겹침) 하나는 만족하고, 다른하나는 만족되지 않음()
		// 검사될 라인에 관련(Y좌표 겹침)있는 것들을 뽑아서 머지 후 검사함	3379

		// Releation인 m_contourMaxBinImage_expectedLineInfo들 모아서, 머지한 후 처리
		vector<int> relatedIndex;

		for (int j = 0; j < m_contourMaxBinImage_expectedLineInfo.size(); j++)	// 이번에 프레임에 찾아낸 라인
		{
			bool isRelative = imageHandler::isRelation(m_expectedLineInfos[i].first.progress.coorY_start, m_expectedLineInfos[i].first.progress.coorY_end, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_start, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_end);
			if (isRelative)	// 이번프레임에도 좌표에 존재함 + 추가조건()
			{
				relatedIndex.push_back(j);
				foundLineRelated[j] = true;
			}
		}

		contourLineInfo mergedLineInfo;
		for (int j = 0; j < relatedIndex.size(); j++) // 이번에 프레임에 찾아낸 라인들 머지
		{
			if (j == 0)
				mergedLineInfo = m_contourMaxBinImage_expectedLineInfo[relatedIndex[j]].progress;
			else
			{
				contourLineInfo targetLine = m_contourMaxBinImage_expectedLineInfo[relatedIndex[j]].progress;
				if (mergedLineInfo.coorX_start > targetLine.coorX_start)
					mergedLineInfo.coorX_start = targetLine.coorX_start;

				if (mergedLineInfo.coorX_end < targetLine.coorX_end)
					mergedLineInfo.coorX_end = targetLine.coorX_end;

				if (mergedLineInfo.coorY_start > targetLine.coorY_start)
					mergedLineInfo.coorY_start = targetLine.coorY_start;

				if (mergedLineInfo.coorY_end < targetLine.coorY_end)
					mergedLineInfo.coorY_end = targetLine.coorY_end;

				for (int idx = 0; idx < targetLine.contours.size(); idx++)
				{
					mergedLineInfo.contours.push_back(targetLine.contours[idx]);
				}
				sort(mergedLineInfo.contours.begin(), mergedLineInfo.contours.end(), imageHandler::asc_contourInfo);
				mergedLineInfo.pixelCount = imageHandler::getContourLineInfoVolume(mergedLineInfo);
				mergedLineInfo.maxValue = getMaxValue(mergedLineInfo);
				Mat mergedMat;
				bitwise_or(mergedLineInfo.weightMat.binImage, targetLine.weightMat.binImage, mergedMat);
				mergedLineInfo.weightMat = WeightMat(mergedMat, mergedLineInfo.weightMat.frameNum);
				//mergedLineInfo.weightMat_Unprint = WeightMat(targetLine.weightMat_Unprint.binImage, targetLine.weightMat_Unprint.frameNum);
			}
		}

		if (relatedIndex.size() == 0)
		{
			m_expectedLineInfos[i].second++;
			;	// 카운트 증가 : Count++;
		}
		else // 머지된 라인과 검사 진행
		{
			if (false==lineValidCheck(m_expectedLineInfos[i].first.progress, mergedLineInfo))
			{
				m_expectedLineInfos[i].second++;
			}
//			m_expectedLineInfos[i].first.progress.weightMat.binImage;
//			if (m_expectedLineInfos[i].first.progress.pixelCount / 2 > mergedLineInfo.pixelCount)
//			{	//
//				m_expectedLineInfos[i].second++;	// 존재하지만 픽셀수가 절반이하로 줄어듦
//			}
//			//else if (m_expectedLineInfos[i].first.progress.maxValue > mergedLineInfo.maxValue)	// maxvalue가 이전보다 낮음 -> 이 조건 없애거나 OR maxValue는 가장 왼쪽 점의 값으로 하기?
//			//{
//			//	m_expectedLineInfos[i].second++;
//			//}
//			else if(imageHandler::getLeftistWhitePixel_value(m_expectedLineInfos[i].first.progress.weightMat.binImage) > 
//				imageHandler::getLeftistWhitePixel_value(mergedLineInfo.weightMat.binImage))
//			{
//				m_expectedLineInfos[i].second++;
//			}
			else
			{
				if (m_expectedLineInfos[i].second == 0)	// 연속으로 0일때만 정보 업데이트
				{
					//int pixelCount = m_expectedLineInfos[i].first.maximum.pixelCount; //weightMat_maximum.binImage);	// 픽셀수가 더 많다면 이미지 유지
					//if (pixelCount > mergedLineInfo.pixelCount)	// 맥시멈이 progress보다 큼 -> 맥시멈은 유지
					int pixelCount = imageHandler::getSumOfBinImageValues(m_expectedLineInfos[i].first.maximum.weightMat.binImage); //m_expectedLineInfos[i].first.maximum.pixelCount; //weightMat_maximum.binImage);
					// weight 합이 이전게 더 크면 maximum만 업데이트 
					if (pixelCount > imageHandler::getSumOfBinImageValues(mergedLineInfo.weightMat.binImage))	// 맥시멈이 progress보다 큼 -> 맥시멈은 유지
					{
						m_expectedLineInfos[i].first.progress = mergedLineInfo;	// 맥시멈만 따로 하는 이유:  라인이 Fade-out일 경우임 (픽셀수가 가장 많음, && )
					}
					else // 맥시멈, 프로그래스 둘다 업데이트
					{
						m_expectedLineInfos[i].first = contourLineInfoSet(mergedLineInfo);
					}
					//
				}
				m_expectedLineInfos[i].second = 0;	// 카운트 초기화
			}
		}

		// foundLineRelated 에 있는 라인들 머지 


		////////////////////////////////////////////////

		//bool isFind = false;
		//for (int j = 0; j < m_contourMaxBinImage_expectedLineInfo.size(); j++)	// 이번에 프레임에 찾아낸 라인
		//{

		//	bool isRelative = imageHandler::isRelation(m_expectedLineInfos[i].first.progress.coorY_start, m_expectedLineInfos[i].first.progress.coorY_end, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_start, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_end);
		//	if (isRelative)	// 이번프레임에도 좌표에 존재함
		//	{
		//		isFind = true;
		//		if (m_expectedLineInfos[i].first.progress.pixelCount / 2 > m_contourMaxBinImage_expectedLineInfo[j].progress.pixelCount)
		//		{	//
		//			m_expectedLineInfos[i].second++;	// 존재하지만 픽셀수가 절반이하로 줄어듦
		//		}
		//		else if (m_expectedLineInfos[i].first.progress.maxValue > m_contourMaxBinImage_expectedLineInfo[j].progress.maxValue)	// maxvalue가 이전보다 낮음
		//		{
		//			m_expectedLineInfos[i].second++;
		//		}
		//		else 
		//		{
		//			if (m_expectedLineInfos[i].second == 0)	// 연속으로 0일때만 정보 업데이트
		//			{
		//				int pixelCount = m_expectedLineInfos[i].first.maximum.pixelCount; //weightMat_maximum.binImage);	// 픽셀수가 더 많다면 이미지 유지
		//				if (pixelCount > m_contourMaxBinImage_expectedLineInfo[j].progress.pixelCount)	// 맥시멈이 progress보다 큼 -> 맥시멈은 유지
		//				{
		//					m_expectedLineInfos[i].first.progress = m_expectedLineInfos[i].first.progress;	
		//				}
		//				else // 맥시멈, 프로그래스 둘다 업데이트
		//				{
		//					m_expectedLineInfos[i].first = m_contourMaxBinImage_expectedLineInfo[j];	
		//				}
		//				//
		//			}
		//			m_expectedLineInfos[i].second = 0;	// 카운트 초기화
		//		}
		//		;
		//	}
		//}
		//if (isFind != true)	// 이번프레임에 없음
		//{
		//	m_expectedLineInfos[i].second++;
		//	;	// 카운트 증가 : Count++;
		//}
		////////////////////////////////////////////////

	}

	// 2. 중복제거
	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 
	{
		m_expectedLineInfos[i].first.progress.coorY_start;
		m_expectedLineInfos[i].first.progress.coorY_end;
		m_expectedLineInfos[i].first.progress.pixelCount;
		// ***** Sorting 추가 -> coorY_start
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

	for (int i = 0; i < m_contourMaxBinImage_expectedLineInfo.size(); i++)	// 3. 신규 라인 찾아 추가 
	{
		//bool isFind = false;
		//for (int j = 0; j < m_expectedLineInfos.size(); j++)
		//{
		//	bool isRelative = imageHandler::isRelation(m_contourMaxBinImage_expectedLineInfo[i].progress.coorY_start, m_contourMaxBinImage_expectedLineInfo[i].progress.coorY_end, m_expectedLineInfos[j].first.progress.coorY_start, m_expectedLineInfos[j].first.progress.coorY_end);
		//	if (isRelative)	// 
		//	{
		//		isFind = true;
		//		break;
		//	}
		//}
		//if (isFind != true)	// 못찾았을 경우 = 새로운 라인
		//{
		//	m_expectedLineInfos.push_back(make_pair(m_contourMaxBinImage_expectedLineInfo[i], 0));
		//	printf(" [Line Add] ");
		//	; // 라인 추가 : addLine()
		//}

		if (foundLineRelated[i] == false)
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

bool PeakFinder::lineValidCheck(contourLineInfo managedLine, contourLineInfo checkLine)
{
	bool isValid = true;

	// lineValidCheck (contourLineInfo managedLine, contourLineInfo checkLine)
	//  - check_lineEnd_pixelCount
	//  - check_lineEnd_maxValue
	// managedLine.contours[0] 의 값 == max값
	// managedLine.contours[0]의 범위를 마스크로 사용하여 mergedLineInfo.weightMat.binImage 마스킹 한 값에서 max값 얻어냄
	// 비교
	//Mat test1;
	//Mat test2;
	//inRange(managedLine.weightMat.binImage, 1, 255, test1);
	//inRange(checkLine.weightMat.binImage, 1, 255, test2);

	// pixelCount -> weightCount ??
	uint managedLineWeightSum = imageHandler::getSumOfBinImageValues(managedLine.weightMat.binImage);
	uint checkLineWeightSum = imageHandler::getSumOfBinImageValues(checkLine.weightMat.binImage);

	//if (managedLine.pixelCount / 2 > checkLine.pixelCount)
	if (managedLineWeightSum / 2 > checkLineWeightSum)
	{	
		isValid = false;
		return isValid;
	}

	for (int i = 0; i < managedLine.contours.size() / 2 + 1; i++)
	{
		// contour/2+1 개까지 검사하면서
		// 증가추세인것이 있어야함
		// 없다면 false한다.
		int managedLineMaxValue = 0;
		int checkLineMaxValue = 0;
		Mat mask = Mat::zeros(managedLine.weightMat.binImage.rows, managedLine.weightMat.binImage.cols, CV_8U);
		mask = imageHandler::getWhiteMaskImage(mask, managedLine.contours[i].coorX_start, managedLine.contours[i].coorY_start, managedLine.contours[i].coorX_end - managedLine.contours[i].coorX_start, managedLine.contours[i].coorY_end - managedLine.contours[i].coorY_start);

		Mat maskedMat;
		bitwise_and(mask, managedLine.weightMat.binImage, maskedMat);
		managedLineMaxValue = imageHandler::getMaximumValue(maskedMat);
		bitwise_and(mask, checkLine.weightMat.binImage, maskedMat);
		checkLineMaxValue = imageHandler::getMaximumValue(maskedMat);


		if (managedLineMaxValue == 0 && checkLineMaxValue == 0)
			continue;

		if (managedLineMaxValue < checkLineMaxValue)
			return true;
			
	}



	return false;
}

contourLineInfo PeakFinder::removeLeftNoise(contourLineInfo linInfo)
{
	int maxValue = linInfo.getMaxValue();
	
	contourInfo contourOfMaximum;
	int max = 0;
	for (int i = 0; i < linInfo.contours.size(); i++)
	{
		int val = linInfo.contours[i].getMaxValue();
		if (max < val)
		{
			max = val;
			contourOfMaximum = linInfo.contours[i];	// 최고점을 갖고있는 컨투어
		}
	}
	// 가장 큰점위치구함.
	// 가장큰점 위치 왼쪽 값 삭제 (정확히는 가장큰점이 존재하는 컨투어 제외 왼쪽점.)
	int width = linInfo.weightMat.binImage.cols - contourOfMaximum.coorX_start;
	int height = linInfo.weightMat.binImage.rows;
	Mat mask = Mat::zeros(linInfo.weightMat.binImage.rows, linInfo.weightMat.binImage.cols, CV_8U);
	mask = imageHandler::getWhiteMaskImage(mask, contourOfMaximum.coorX_start, 0, width, height);

	Mat maskedImage;
	bitwise_and(mask, linInfo.weightMat.binImage, maskedImage);

	linInfo.weightMat.binImage = maskedImage;
	return linInfo;
}


int PeakFinder::getMaxValue(contourLineInfo lineInfo)
{
	int maxValue = 0;
	for (int i = 0; i < lineInfo.contours.size(); i++)
	{
		int val = lineInfo.contours[i].getMaxValue();
		if (val > maxValue)
			maxValue = val;
	}
	return maxValue;
}


