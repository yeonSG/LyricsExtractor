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

// ������ �ƽ� �̹��� ������ ������ ���� ����
void PeakFinder::makeContourMaxBinImageAndContourInfos()
{
	Mat outImage = Mat::zeros(stackBinImage.rows, stackBinImage.cols, CV_8U);
	contourMaxBinImage_contourInfos.clear();

	vector<vector<Point>> contours;
	findContours(stackBinImage, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	Mat binImage;
	inRange(stackBinImage, 1, 255, binImage);
	binImage = imageHandler::getDustRemovedImage(binImage);	// ���ǰ� 3���� ������ ������

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(stackBinImage.rows, stackBinImage.cols, CV_8U);;
		// ����� ����ũ
		vector<vector<Point>> contours_picked;
		contours_picked.push_back(contours[i]);
		fillPoly(contourMask, contours_picked, 255);
		vector<Point> indices;		// ������ ������ ����
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
	
		;	// contourLineInfo ���� ��ƾ
			// 2.1 ��ä��ȸ�Ͽ� ��� ���� ���� (��� ������ ����)
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

				for (int j = i + 1; j < contourInfos.size(); j++)	// ���� ������� Ȯ��
				{
					// Y start~end �ȿ� ���Եȴٸ� �߰�
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
		if (idx == 0)	// �ʱ�ȭ
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

	conLineInfo = getLineInfoFromContourInfos(contourMaxBinImage_contourInfos);		// ����� �������� ���⸸ ��

	contourMaxBinImage_expectedLineInfo = expectedLineInfoAfterProcess(conLineInfo);	// ���� ��ó��(����)
}

vector<contourLineInfo> PeakFinder::expectedLineInfoAfterProcess(vector<contourLineInfo> conLineInfos)
{

	for (int i = 0; i < conLineInfos.size(); i++)	// 1.
	{
		for (int j = 0; j < conLineInfos[i].contours.size(); j++)
		{
			if (conLineInfos[i].coorY_start > conLineInfos[i].contours[j].coorY_start)
				conLineInfos[i].coorY_start = conLineInfos[i].contours[j].coorY_start;	//�ּҰ�

			if (conLineInfos[i].coorY_end < conLineInfos[i].contours[j].coorY_end)
				conLineInfos[i].coorY_end = conLineInfos[i].contours[j].coorY_end;	// �ִ밪

			if (conLineInfos[i].coorX_start > conLineInfos[i].contours[j].coorX_start)
				conLineInfos[i].coorX_start = conLineInfos[i].contours[j].coorX_start;	//�ּҰ�

			if (conLineInfos[i].coorX_end < conLineInfos[i].contours[j].coorX_end)
				conLineInfos[i].coorX_end = conLineInfos[i].contours[j].coorX_end;	// �ִ밪
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
		}	  // �ߺ�����	 
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

	// >> ���� �����ӿ��� ��������� ã�Ƴ�
	// 1. contourLineInfo�� coorY_start�� coorY_end�� ��� ����� ���� �ִ�-�ּҰ����� �ٲ�
	// 2. vecContours.Size()�� 5 �̻��� �͸� �츲 & �ȼ�ī��Ʈ�� 100 �̻��ΰ͸� �츲 & �ִ� weight�� 5 �̻��ΰ͸� �츲
	// 3. contourLineInfo���� Y���� ��ġ�� �κ��� ������ ��ġ��(merge) vecContours �ٽ� ���� -> Ŭ�����͸� �� �͸� ����
	// 4. contours�ߺ� ����
	// 5. ���� ��� & ���� �̹��� ���� & �ִ밪 �Է�
	// 2. (�ѹ���)vecContours.Size()�� 5 �̻��� �͸� �츲 & �ȼ�ī��Ʈ�� 100 �̻��ΰ͸� �츲 & �ִ� weight�� 5 �̻��ΰ͸� �츲
	// 6. �ش� �� ���� ( �ʿ��ϴٸ� ���� �߰� �� �����Ұ�!)

}

vector<contourLineInfo> PeakFinder::getJudgeLineByFrameFlow()
{
	vector<contourLineInfo> judgedLine;

	for (int i = 0; i < expectedLineInfos.size(); i++)	// 1.
	{
		bool isFind = false;
		for (int j = 0; j < contourMaxBinImage_expectedLineInfo.size(); j++)	// �̹��� �����ӿ� ã�Ƴ� ����
		{
			bool isRelative = imageHandler::isRelation(expectedLineInfos[i].first.coorY_start, expectedLineInfos[i].first.coorY_end, contourMaxBinImage_expectedLineInfo[j].coorY_start, contourMaxBinImage_expectedLineInfo[j].coorY_end);
			if (isRelative)	// �̹������ӿ��� ��ǥ�� ������
			{
				isFind = true;
				if (expectedLineInfos[i].first.pixelCount / 2 > contourMaxBinImage_expectedLineInfo[j].pixelCount)
				{	//
					expectedLineInfos[i].second++;	// ���������� �ȼ����� �������Ϸ� �پ��
				}
				else if (expectedLineInfos[i].first.maxValue > contourMaxBinImage_expectedLineInfo[j].maxValue)	// maxvalue�� �������� ����
				{
					expectedLineInfos[i].second++;
				}
				else 
				{
					if (expectedLineInfos[i].second == 0)	// �������� 0�϶��� ���� ������Ʈ
					{
						int pixelCount = imageHandler::getWhitePixelCount(expectedLineInfos[i].first.weightMat_maximum.binImage);	// �ȼ����� �� ���ٸ� �̹��� ����
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
				;// ī��Ʈ �ʱ�ȭ : Count = 0;
			}
		}
		if (isFind != true)	// �̹������ӿ� ����
		{
			expectedLineInfos[i].second++;
			;	// ī��Ʈ ���� : Count++;
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
		if (isFind != true)	// ��ã���� ���  
		{
			expectedLineInfos.push_back(make_pair(contourMaxBinImage_expectedLineInfo[i], 0));
			printf(" [Line Add] ");
			; // ���� �߰� : addLine()
		}
	}

	vector<pair< contourLineInfo, int>> expectedLineInfos_temp;
	for (int i = 0; i < expectedLineInfos.size(); i++)	// 4. 
	{
		if (expectedLineInfos[i].second >= PeakFinder::JUDGE_TIMEOUT)	// timeOut�� 5�̻��̸� �������� ó�� 
		{
			judgedLine.push_back(expectedLineInfos[i].first);
			; // expectedLineInfos Ȯ��				
		}
		else
		{
			expectedLineInfos_temp.push_back(expectedLineInfos[i]);
		}
	}
	expectedLineInfos = expectedLineInfos_temp;

	// 5. �ߺ� ����
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

	// >> frame �帧�� �־ �������� �Ǻ�
	// 1. �������������� ���� ���ο� �ִٸ� ������Ʈ and ������ ���� count++
	// 2. ������Ʈ �� ���� �� �ߺ��� ������ �ִٸ� ����  (�ΰ��� ������ ����Ǵ� �ϳ��� �������⵵ ��.)
	// 3. �ű� ���� ã�� �߰� 
	// 4. timeOut count�� 5 �̻��ΰ͵� �������� �����ǰ� (������ �� �������� ��������) + ����(MaxValue, etc..)
	// 5. �ߺ� ���� 

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


