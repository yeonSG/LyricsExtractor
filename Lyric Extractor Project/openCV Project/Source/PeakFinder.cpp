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
		inRange(refUnprintImage, 0, 2, bin_refUnprintImage);// �ֱ� 3�������� ����̾�����

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

// x������ Ȯ���Ǹ� Ȯ���� �̹������� ������ ū ���� ������ ���� �������־� 
// ������ Ȯ���Ǹ� �ش��ϴ� y���� binImage�� 0���� ����.
// �� �� ���� �Ǻ��� �� �տ� ���α��� ��ƸԴ� ������ �����ϱ� ����
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
			if (yPtr_corr[x] > yPtr_valid[x] && yPtr_valid[x]!=0)	// stackBin[x]�� ���� vaild[x]���� ũ�� vaild[x]�� 0�� �ƴ� �� ������
			{
				int temp = yPtr_corr[x] - (yPtr_valid[x] + JUDGE_TIMEOUT);
				if (temp < 0)	// ���� ���� - �̸� ������������
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
			// ������
			// ������ ���� -> 0ó��
			// ������ ��� and Unprint�� ����̾����� -> 1ó�� (����)
			// �����̹����� �̹� ĥ���� �����鼭 �����ΰ�

			if (yPtr_pattern[x] == 0)	// 0�ΰ�  On ����
			{
				yPtr_stack[x] = 0;
			}
			else if (yPtr_pattern[x] != 0 && yPtr_stack[x] != 0)	// �����̸鼭,
			{
				yPtr_stack[x] += 1;
			}
			else if (yPtr_pattern[x] != 0 && yPtr_refUnprint[x] != 0)	// �����̸鼭 ����̾����� (��������)
			{
				yPtr_stack[x] = 1;
			}

		}
	}
	return stackBinImage;
}

// ������ �ƽ� �̹��� ������ ������ ���� ����
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
	binTempImage = imageHandler::getDustRemovedImage(binTempImage);	// ���ǰ� 3���� ������ ������

	vector<vector<Point>> contours;
	findContours(binTempImage, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(m_stackBinImage.rows, m_stackBinImage.cols, CV_8U);;
		// ����� ����ũ
		vector<vector<Point>> contours_picked;
		contours_picked.push_back(contours[i]);
		fillPoly(contourMask, contours_picked, 255);
		vector<Point> indices;		// ������ ������ ����
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
			if (diff <= 10)	// p�� np���� ũ�ų� �������� ��
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

	conLineInfo = getLineInfoFromContourInfos(m_contourMaxBinImage_contourInfos);		// ����� �������� ���⸸ ��

	if (conLineInfo.size() != 0)
	{
		m_contourMaxBinImage_expectedLineInfo = expectedLineInfoAfterProcess(conLineInfo);	// ���� ��ó��(����)
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

	// >> ���� �����ӿ��� ��������� ã�Ƴ�
	// 1. contourLineInfo�� coorY_start�� coorY_end�� ��� ����� ���� �ִ�-�ּҰ����� �ٲ�
	// 2. vecContours.Size()�� 5 �̻��� �͸� �츲 & �ȼ�ī��Ʈ�� 100 �̻��ΰ͸� �츲 & �ִ� weight�� 5 �̻��ΰ͸� �츲
	// 3. contourLineInfo���� Y���� ��ġ�� �κ��� ������ ��ġ��(merge) vecContours �ٽ� ���� -> Ŭ�����͸� �� �͸� ����
	// 4. contours�ߺ� ����
	// 5. ���� ��� & ���� �̹��� ���� & �ִ밪 �Է�
	// 2. (�ѹ���)vecContours.Size()�� 5 �̻��� �͸� �츲 & �ȼ�ī��Ʈ�� 100 �̻��ΰ͸� �츲 & �ִ� weight�� 5 �̻��ΰ͸� �츲
	// 6. �ش� �� ���� ( �ʿ��ϴٸ� ���� �߰� �� �����Ұ�!)

}

vector<contourLineInfoSet> PeakFinder::getJudgeLineByFrameFlow()
{
	vector<contourLineInfoSet> judgedLine;

	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 1.
	{
		bool isFind = false;
		for (int j = 0; j < m_contourMaxBinImage_expectedLineInfo.size(); j++)	// �̹��� �����ӿ� ã�Ƴ� ����
		{
			bool isRelative = imageHandler::isRelation(m_expectedLineInfos[i].first.progress.coorY_start, m_expectedLineInfos[i].first.progress.coorY_end, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_start, m_contourMaxBinImage_expectedLineInfo[j].progress.coorY_end);
			if (isRelative)	// �̹������ӿ��� ��ǥ�� ������
			{
				isFind = true;
				if (m_expectedLineInfos[i].first.progress.pixelCount / 2 > m_contourMaxBinImage_expectedLineInfo[j].progress.pixelCount)
				{	//
					m_expectedLineInfos[i].second++;	// ���������� �ȼ����� �������Ϸ� �پ��
				}
				else if (m_expectedLineInfos[i].first.progress.maxValue > m_contourMaxBinImage_expectedLineInfo[j].progress.maxValue)	// maxvalue�� �������� ����
				{
					m_expectedLineInfos[i].second++;
				}
				else 
				{
					if (m_expectedLineInfos[i].second == 0)	// �������� 0�϶��� ���� ������Ʈ
					{
						int pixelCount = m_expectedLineInfos[i].first.maximum.pixelCount; //weightMat_maximum.binImage);	// �ȼ����� �� ���ٸ� �̹��� ����
						if (pixelCount > m_contourMaxBinImage_expectedLineInfo[j].progress.pixelCount)	// �ƽø��� progress���� ŭ -> �ƽø��� ����
						{
							m_expectedLineInfos[i].first.progress = m_expectedLineInfos[i].first.progress;
						}
						else // �ƽø�, ���α׷��� �Ѵ� ������Ʈ
						{
							m_expectedLineInfos[i].first = m_contourMaxBinImage_expectedLineInfo[j];	
						}
						//
					}
					m_expectedLineInfos[i].second = 0;	// ī��Ʈ �ʱ�ȭ
				}
				;
			}
		}
		if (isFind != true)	// �̹������ӿ� ����
		{
			m_expectedLineInfos[i].second++;
			;	// ī��Ʈ ���� : Count++;
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
		if (isFind != true)	// ��ã���� ��� = ���ο� ����
		{
			m_expectedLineInfos.push_back(make_pair(m_contourMaxBinImage_expectedLineInfo[i], 0));
			printf(" [Line Add] ");
			; // ���� �߰� : addLine()
		}
	}

	vector<pair< contourLineInfoSet, int>> expectedLineInfos_temp;
	for (int i = 0; i < m_expectedLineInfos.size(); i++)	// 4. 
	{
		if (m_expectedLineInfos[i].second >= PeakFinder::JUDGE_TIMEOUT)	// timeOut�� 5�̻��̸� �������� ó�� 
		{
			judgedLine.push_back(m_expectedLineInfos[i].first);
			; // expectedLineInfos Ȯ��				
		}
		else
		{
			expectedLineInfos_temp.push_back(m_expectedLineInfos[i]);
		}
	}
	m_expectedLineInfos = expectedLineInfos_temp;

	// 5. �ߺ� ����
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


