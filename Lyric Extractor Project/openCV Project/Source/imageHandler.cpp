#include "imageHandler.h"


bool desc(contourInfo a, contourInfo b)
{
	return a.coorX_start > b.coorX_start;
}
bool asc(contourInfo a, contourInfo b)
{
	return a.coorX_start < b.coorX_start;
}

int imageHandler::getContourLineInfoVolume(contourLineInfo lineInfo)
{
	int sum = 0;
	for (int i = 0; i < lineInfo.contours.size(); i++)
	{
		sum += lineInfo.contours[i].pixelCount;
	}
	return sum;
}

bool imageHandler::desc_contourInfo(contourInfo a, contourInfo b)
{
	return a.coorX_start > b.coorX_start;
}
bool imageHandler::asc_contourInfo(contourInfo a, contourInfo b)
{
	return a.coorX_start < b.coorX_start;
}

bool imageHandler::desc_contourLineInfo(contourLineInfoSet a, contourLineInfoSet b)
{
	return a.progress.coorX_start > b.progress.coorX_start;
}
bool imageHandler::asc_contourLineInfo(contourLineInfoSet a, contourLineInfoSet b)
{
	return a.progress.coorX_start < b.progress.coorX_start;
}

bool imageHandler::isRelation(int a_start, int a_end, int b_start, int b_end)
{
	if (a_start >= b_end || a_end <= b_start)
		return false;
	else
		return true;
}

Mat imageHandler::resizeImageToAnalize(Mat& sourceImage)
{
	Mat resizedImage;

	if (sourceImage.rows != 720)
	{
		float size = 720 / (float)sourceImage.rows;
		resize(sourceImage, resizedImage, cv::Size(sourceImage.cols * size, 720), 0, 0, cv::INTER_CUBIC);
	}
	else
	{
		resizedImage = sourceImage;
	}
	return resizedImage;
}

/// <summary>
/// 이미지의 가사 부분만 잘라 반환함.
/// 자를 부분은 고정 값이기 때문에 주의해야함 (만약 자를 부분이 동적으로 변한다면 자를 부분 찾을 알고리즘 구현 필요)
/// </summary>
/// <param name="sourceImage">The source image.</param>
/// <returns>가사 부분만 자른 이미지</returns>
Mat imageHandler::getSubtitleImage(Mat& sourceImage)
{
	Rect subRect(0, SUBTITLEAREA_Y, sourceImage.cols, sourceImage.rows - SUBTITLEAREA_Y);//SUBTITLEAREA_LENGTH);	// sub_start_y, sub_length
	Mat subImage = sourceImage(subRect);
	return subImage;
}

Mat imageHandler::getResizeAndSubtitleImage(Mat& sourceImage)
{
	Mat resizedMat = resizeImageToAnalize(sourceImage);
	Mat subImage = getSubtitleImage(resizedMat);
	return subImage;
}

/// <summary>
/// thresold 함수로 이진화 한 이미지 반환.
/// </summary>
/// <param name="sourceImage">3체널 이미지(컬러).</param>
/// <returns></returns>
Mat imageHandler::getBinaryImage(Mat& sourceImage)
{
	Mat image_gray;
	cvtColor(sourceImage, image_gray, COLOR_BGR2GRAY);
	Mat image_gray_bin;
	threshold(image_gray, image_gray_bin, 146, 255, THRESH_BINARY);	// 146이 테스트로 얻은 최적값
	return image_gray_bin;
}

Mat imageHandler::getBlueColorFilteredBinaryImage(Mat& sourceImage)
{
	// 3-1. Blue(HSV)의 이진화
	Mat hsvImage;
	cvtColor(sourceImage, hsvImage, COLOR_BGR2HSV);
	// 3. 이진화
	Mat image_getBlue;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
	inRange(hsvImage, Scalar(115, 100, 100), Scalar(140, 255, 255), image_getBlue);	// HSV값은 테스트로 얻은 값임.

	return image_getBlue;
}

/// <summary>
/// 모플로지 연산 수행 (Noise 제거)
/// </summary>
/// <param name="sourceImage">이진화 된 이미지.</param>
/// <returns></returns>
Mat imageHandler::getMorphImage(Mat& sourceImage, cv::MorphTypes type)
{
	Mat element(3, 3, CV_8U, Scalar(1));
	element = getStructuringElement(MORPH_ELLIPSE, Point(3, 3));
	//element = { {0, 1, 0},{1, 1,0},

	Mat image_morp;

	switch (type)
	{
	case MORPH_DILATE:
		dilate(sourceImage, image_morp, element);	// 흰색 넓힘
		break;
	case MORPH_ERODE:
		erode(sourceImage, image_morp, element);	// 흰색 깍음
		break;
	case MORPH_CLOSE:
		morphologyEx(sourceImage, image_morp, MORPH_CLOSE, element);
		break;
	case MORPH_OPEN:
		morphologyEx(sourceImage, image_morp, MORPH_OPEN, element);
		break;
	}
	//erode()	// 침식연산

	return image_morp;
}


Mat imageHandler::getCannyImageWithBinaryImage(Mat& binImage)
{
	Mat image_canny;
	Canny(binImage, image_canny, 250, 255);
	return image_canny;
}

/// <summary>
/// 바이너리 이미지의 태두리에 floodFill(그림판의 패인트통) 연산을 한 이미지 반환(Noise 제거)
/// </summary>
/// <param name="binaryMat">이진화 이미지</param>
/// <param name="toBlack">true면 태두리를 검정으로 함</param>
/// <returns>테두리에 floodFill 연산의 결과 이미지</returns>
Mat imageHandler::getBorderFloodFilledImage(Mat& binaryMat, bool toBlack)
{
	Mat outImage = binaryMat.clone();
	int nRows = outImage.rows;
	int nCols = outImage.cols;

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;


	// 상측 
	for (int i = 0; i < nCols; i++)
		if (outImage.at<uchar>(0, i) != color)
			floodFill(outImage, Point(i, 0), color);

	// 좌측
	for (int i = 0; i < nRows; i++)
		if (outImage.at<uchar>(i, 30) != color)
			floodFill(outImage, Point(30, i), color);

	// 우측
	for (int i = 0; i < nRows; i++)
		if (outImage.at<uchar>(i, nCols - 30) != color)
			floodFill(outImage, Point(nCols - 30, i), color);

	// 아래측
	for (int i = 0; i < nCols; i++)
		if (outImage.at<uchar>(nRows - 1, i) != color)
			floodFill(outImage, Point(i, nRows - 1), color);

	return outImage;
}

Mat imageHandler::getBorderFloodFilledImageForColor(Mat& rgbImage, bool toBlack)
{
	int nRows = rgbImage.rows;
	int nCols = rgbImage.cols;

	Vec3b color;
	if (toBlack == true)
	{
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
	}
	else
	{
		color[0] = 255;
		color[1] = 255;
		color[2] = 255;
	}

	// 상측 
	for (int i = 0; i < nCols; i++)
		if (rgbImage.at<Vec3b>(2, i) != color)
			floodFill(rgbImage, Point(i, 2), color);

	// 좌측
	for (int i = 0; i < nRows; i++)
		if (rgbImage.at<Vec3b>(i, 30) != color)
			floodFill(rgbImage, Point(30, i), color);

	// 우측
	for (int i = 0; i < nRows; i++)
		if (rgbImage.at<Vec3b>(i, nCols - 30) != color)
			floodFill(rgbImage, Point(nCols - 30, i), color);

	// 아래측
	for (int i = 0; i < nCols; i++)
		if (rgbImage.at<Vec3b>(nRows - 1, i) != color)
			floodFill(rgbImage, Point(i, nRows - 1), color);

	return rgbImage;
}


// 이미지에서 흰색인 가사 아닌것 제거(20x20정사각형인곳 or width가 20이상인것 )
Mat imageHandler::removeNotLyricwhiteArea(static Mat& binaryMat)
{
	int height = binaryMat.rows;
	int width = binaryMat.cols;

	Mat outImage;
	outImage = binaryMat.clone();

	// width가 0이상인 흰줄에 floodfill
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y); //
		int continueCount = 0;

		for (int x = 0; x < width; x++)
		{
			bool isRight = false;	// 조건만족?

			if (yPtr[x] == 255)
				continueCount++;
			else
				continueCount = 0;

			if (continueCount >= 100)
				floodFill(outImage, Point(x, y), 0); // do floodfill to black
		}
	}

	// 20x20이상 흰색 박스인 곳에floodfill
	int boxSize = 20;
	for (int y = 0; y < height - boxSize; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y); //

		for (int x = 0; x < width - boxSize; x++)
		{
			bool isBox = true;
			for (int boxY = 0; boxY < boxSize; boxY++)
			{
				uchar* yPtr_box = outImage.ptr<uchar>(y + boxY);
				for (int boxX = 0; boxX < boxSize; boxX++)
				{
					if ((yPtr_box[x + boxX]) == 255)
						;
					else
					{
						isBox = false;
						break;
					}
				}
				if (isBox != true)
					break;
			}
			if (isBox)	// 
				floodFill(outImage, Point(x, y), 0); // do floodfill to black

		}
	}

	return outImage;
}


/// <summary>
/// 필터링을 거친 Contours들을 가로 라인으로 뚫었을 때 가장 많이 걸린 개수.
/// </summary>
/// <param name="binImage">The bin image.</param>
/// <returns></returns>
int imageHandler::getAlinedContoursCount(Mat& binImage)
{
	Mat image_debug = binImage.clone();

	// 5. contour 검출
	vector<vector<Point>> contours;
	findContours(binImage, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

	vector<vector<Point>> contours_poly(contours.size());
	vector<Rect> boundRect;
	vector<Rect> boundRect_filtered;

	for (int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);	// contour -> contourPolyDP
		boundRect.push_back(boundingRect(Mat(contours_poly[i])));

		rectangle(image_debug, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0));	// forDebug
	}
	boundRect_filtered = getFillteredContours(boundRect);

	//drawContours(orgImage, contours_poly, -1, Scalar(0, 255, 255), 1);

	// => Blue에서 가장 많은 contur가 걸리는 선x를 구한다.
	// => Red 에서 가장 많은 contur가 걸리는 선x를 구한다.
	// ==> 더 많은 contur가 걸린 컬러영역이 Painted_Lyric이 된다.
	int biggestCount = 0;
	int biggestCountCol = 0;
	for (int col = 5; col < binImage.cols; col += 3)
	{
		int count = 0;
		for (int j = 0; j < boundRect_filtered.size(); j++)
		{
			int rectY = boundRect_filtered[j].y;
			int rectHeight = boundRect_filtered[j].height;
			if (rectY < col && rectY + rectHeight > col)	// Rect안에 
				count++;
		}
		if (biggestCount < count)
		{
			biggestCount = count;
			biggestCountCol = col;
		}
	}

	return biggestCount;
}

/// <summary>
/// Gets the filltered contours.
/// </summary>
/// <param name="contoursRect">The contours rect.</param>
/// <returns></returns>
vector<Rect> imageHandler::getFillteredContours(vector<Rect> contoursRect)
{
	vector<Rect> boundRect_filtered;

	for (int i = 0; i < contoursRect.size(); i++)
	{
		if (contoursRect[i].width < 6)
			continue;	// fillter : 너비가 6 이하인 것
		else if (contoursRect[i].height < 6)
			continue;	// fillter : 높이가 6 이하인 것

		int lower, bigger;
		if (contoursRect[i].height > contoursRect[i].width)
		{
			bigger = contoursRect[i].height;
			lower = contoursRect[i].width;
		}
		else
		{
			bigger = contoursRect[i].width;
			lower = contoursRect[i].height;
		}

		if (bigger / lower >= 6)	// 가로새로 비율이 6 이상인 것 
			continue;

		boundRect_filtered.push_back(contoursRect[i]);
	}

	return boundRect_filtered;
}

/// <summary>
/// 깔끔한 MaskImage 얻기 위한 것, 
///  - 가로로 길게 뻗어있는 Object를 삭제
///  - 해당하는 영역 row에 최대 최소 x값을 구해 중앙기준으로 비율이 비슷하다면 노이즈 아닌걸로
/// </summary>
/// <param name="binaryMat">The binary mat.</param>
/// <param name="toBlack">if set to <c>true</c> [to black].</param>
/// <returns></returns>
Mat imageHandler::getNoiseRemovedImage(Mat& binaryMat, bool toBlack)
{
	int nRows = binaryMat.rows;
	int nCols = binaryMat.cols;
	int rightistPoint = getRightistWhitePixel_x(binaryMat);
	int leftistPoint = getLeftistWhitePixel_x(binaryMat);

	threshold(binaryMat, binaryMat, 128, 255, THRESH_BINARY);

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;

	for (int row = 0; row < nRows; row++) {
		int count = 0;
		uchar* rowPtr = binaryMat.ptr<uchar>(row);
		for (int i = 0; i < nCols; i++) {
			if (rowPtr[i] != color)
				count++;
			else
				count = 0;

			if (count > 60)	// 60pixel
			{
				Mat rowMat = Mat::zeros(1, nCols, CV_8U);
				uchar* rowMatPtr = rowMat.ptr<uchar>(0);	// copy row
				for (int c = 0; c < nCols; c++) {
					rowMatPtr[c] = rowPtr[c];
				}
				int rowRightistPoint = getRightistWhitePixel_x(rowMat);
				int rowLeftistPoint = getLeftistWhitePixel_x(rowMat);

				if ((rightistPoint + 100 < rowRightistPoint || rowRightistPoint < rightistPoint - 100) ||
					(leftistPoint + 100 < rowLeftistPoint || rowLeftistPoint < leftistPoint - 100))
					floodFill(binaryMat, Point(i, row), color);

				count = 0;
			}
		}
	}

	Mat removedMat = getDotRemovedImage(binaryMat, 10, toBlack);

	return removedMat;
}

/// <summary>
/// 컨투어해서 면적이 x이하인 것 삭제
/// </summary>
/// <param name="binaryMat">The binary mat.</param>
/// <param name="toBlack">if set to <c>true</c> [to black].</param>
/// <returns></returns>
Mat imageHandler::getDotRemovedImage(Mat& binaryMat, int dotSize, bool toBlack)
{
	threshold(binaryMat, binaryMat, 128, 255, THRESH_BINARY);

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;

	vector<vector<Point>> contours;
	findContours(binaryMat, contours, RETR_LIST, CHAIN_APPROX_NONE);
	vector<Moments> mu(contours.size());
	for (unsigned int i = 0; i < contours.size(); i++)
		mu[i] = moments(contours[i]);

	/*
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		std::cout << "# of contour points: " << contours[i].size() << " " << contours[i][0] << std::endl;
		mu[i] = moments(contours[i]);

		for (unsigned int j = 0; j < contours[i].size(); j++)
		{
			//std::cout << "Point(x,y)=" << contours[i][j] << std::endl;
		}
		std::cout << " Area: " << contourArea(contours[i]) << std::endl;
		std::cout << mu[i].m10 / mu[i].m00 << ", " << mu[i].m01 / mu[i].m00 << std::endl;	// Center of Contour
	}
	*/

	Mat removedMat = binaryMat.clone();
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		int conArea = contourArea(contours[i]);
		if (conArea == 0)
			floodFill(removedMat, contours[i][0], color);
		else if (contourArea(contours[i]) < dotSize)
		{
			Point middlePoint = Point((int)((mu[i].m10 / mu[i].m00) + 0.5), (int)((mu[i].m01 / mu[i].m00) + 0.5));
			floodFill(removedMat, middlePoint, color);
		}
		//floodFill(removedMat, contours[i][0], color);
	}

	return removedMat;
}

Mat imageHandler::getDustRemovedImage(Mat& binaryMat, bool toBlack)
{
	threshold(binaryMat, binaryMat, 128, 255, THRESH_BINARY);

	int color;
	if (toBlack == true)
		color = 0;
	else
		color = 255;

	vector<vector<Point>> contours;
	findContours(binaryMat, contours, RETR_LIST, CHAIN_APPROX_NONE);

	Mat removedMat = binaryMat.clone();
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() <= 2)
		{
			for (int j = 0; j < contours[i].size(); j++)
				floodFill(removedMat, contours[i][j], color);
		}
		//floodFill(removedMat, contours[i][0], color);
	}

	return removedMat;
}

// 컨투어에 해당하는 영역의 컬러를 해당 컨투어의 평균 값으로 칠하는 함수
/*
 1. 컨투어 영역 mask 땀
 2. 평균값을 구한 후 fludfill

 1. inrange()하여 0 or 255 이진화
 2.
*/
Mat imageHandler::getMaxColorContoursImage(Mat& binaryMat, vector<contourLineInfo>& expectedLineInfos_curFrame)
{
	Mat outImage = Mat::zeros(binaryMat.rows, binaryMat.cols, CV_8U);

	vector<vector<Point>> contours;
	findContours(binaryMat, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
	vector<contourInfo> contourInfos;

	Mat binImage;
	inRange(binaryMat, 1, 255, binImage);
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(binaryMat.rows, binaryMat.cols, CV_8U);;
		// 컨투어별 마스크
		vector<vector<Point>> contours_picked;
		contours_picked.push_back(contours[i]);
		fillPoly(contourMask, contours_picked, 255);
		vector<Point> indices;		// 내부의 점들을 얻음
		bitwise_and(contourMask, binImage, contourMask);
		findNonZero(contourMask, indices);

		int sum = 0;
		int max = 0;
		for (int idx = 0; idx < indices.size(); idx++)
		{
			uchar* yPtr = binaryMat.ptr<uchar>(indices[idx].y);	//in
			sum += yPtr[indices[idx].x];
			if (max < yPtr[indices[idx].x])
				max = yPtr[indices[idx].x];
		}
		int avgContourColor = sum / indices.size();

		for (int idx = 0; idx < indices.size(); idx++)
		{
			uchar* yPtr = outImage.ptr<uchar>(indices[idx].y);	//in
			//yPtr[indices[idx].x] = avgContourColor;
			yPtr[indices[idx].x] = max;
		}

		contourInfo conInfo;
		for (int idx = 0; idx < indices.size(); idx++)
		{
			if (idx == 0)	// 초기화
			{
				conInfo.coorX_start = indices[idx].x;
				conInfo.coorX_end = indices[idx].x;
				conInfo.coorY_start = indices[idx].y;
				conInfo.coorY_end = indices[idx].y;
				conInfo.pixelCount = indices.size();
			}

			if (indices[idx].x < conInfo.coorX_start)
				conInfo.coorX_start = indices[idx].x;
			if (indices[idx].x > conInfo.coorX_end)
				conInfo.coorX_end = indices[idx].x;
			if (indices[idx].y < conInfo.coorY_start)
				conInfo.coorY_start = indices[idx].y;
			if (indices[idx].y > conInfo.coorY_end)
				conInfo.coorY_end = indices[idx].y;
		}
		contourInfos.push_back(conInfo);
	}

	// 1. 컨투어인포 x좌표 기준으로 소팅
	// 2. contourLineInfo 생성 
	sort(contourInfos.begin(), contourInfos.end(), asc);

	vector<contourLineInfo> conLineInfos;	// Expect contour Line
	{
		;	// contourLineInfo 생성 루틴
			// 2.1 전채순회하여 모든 연결 구함 (모든 컨투어 진행)
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
				for (int j = i + 1; j < contourInfos.size(); j++)	// 남은 컨투어들 확인
				{
					// Y start~end 안에 포함된다면 추가
					if (isRelation(contourLineInfo.coorY_start, contourLineInfo.coorY_end, contourInfos[j].coorY_start, contourInfos[j].coorY_end))
					{
						contourLineInfo.contours.push_back(contourInfos[j]);
						contourInfos[j].isRefed = true;
					}
				contourLineInfo.pixelCount = getContourLineInfoVolume(contourLineInfo);
				}
				conLineInfos.push_back(contourLineInfo);
			}
		}
	}

	for (int i = 0; i < conLineInfos.size(); i++)	// 1.
	{
		for (int j = 0; j < conLineInfos[i].contours.size(); j++)
		{
			if (conLineInfos[i].coorY_start > conLineInfos[i].contours[j].coorY_start)
				conLineInfos[i].coorY_start = conLineInfos[i].contours[j].coorY_start;	//최소값

			if (conLineInfos[i].coorY_end < conLineInfos[i].contours[j].coorY_end)
				conLineInfos[i].coorY_end = conLineInfos[i].contours[j].coorY_end;	// 최대값
		}
	}

	vector<contourLineInfo> conLineInfos_buffer;
	for (int i = 0; i < conLineInfos.size(); i++)	// 2.
	{
		bool isPassOnSize = false;
		bool isPassOnVolume = false;
		if (conLineInfos[i].contours.size() >= 5)
			isPassOnSize = true;
		if (conLineInfos[i].pixelCount >= 100)
			isPassOnVolume = true;

		if(isPassOnSize && isPassOnVolume)
			conLineInfos_buffer.push_back(conLineInfos[i]);
	}
	conLineInfos = conLineInfos_buffer;
	conLineInfos_buffer.clear();

	for (int i = 0; i < conLineInfos.size(); i++)	// 3. 
	{
		bool isMerged = false;
		for (int j = 0; j < conLineInfos_buffer.size(); j++)
		{
			if (isRelation(conLineInfos_buffer[j].coorY_start, conLineInfos_buffer[j].coorY_end, conLineInfos[i].coorY_start, conLineInfos[i].coorY_end))
			{
				if (conLineInfos_buffer[j].coorY_start > conLineInfos[i].coorY_start)
					conLineInfos_buffer[j].coorY_start = conLineInfos[i].coorY_start;

				if (conLineInfos_buffer[j].coorY_end < conLineInfos[i].coorY_end)
					conLineInfos_buffer[j].coorY_end = conLineInfos[i].coorY_end;

				for (int idx = 0; idx < conLineInfos[i].contours.size(); idx++)
				{
					conLineInfos_buffer[j].contours.push_back(conLineInfos[i].contours[idx]);
				}
				sort(conLineInfos_buffer[j].contours.begin(), conLineInfos_buffer[j].contours.end(), asc);

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
		conLineInfos[i].pixelCount = getContourLineInfoVolume(conLineInfos[i]);
	}

	expectedLineInfos_curFrame = conLineInfos;	// for return

	// >> 현제 프래임에서 예상라인을 찾아냄
	// 1. contourLineInfo의 coorY_start와 coorY_end를 모든 컨투어에 대한 최대-최소값으로 바꿈
	// 2. vecContours.Size()가 5 이상인 것만 살림 & 픽셀카운트가 100 이상인것만 살림
	// 3. contourLineInfo끼리 Y값이 겹치는 부분이 있으면 합치고(merge) vecContours 다시 소팅 -> 클러스터링 된 것만 남음
	// 4. contours중복 제거
	// 5. 부피 계산
	// 6. 해당 값 리턴 ( 필요하다면 조건 추가 및 변경할것!)

	// 0. 밖에서) 해당 값과 이후 프레임에서 얻어지는 값과 비교진행 (Y좌표 기준으로 겹처 존제하던게 사라짐)
	// 0.1 Y좌표 관련있는것 && 픽셀수 크면 업데이트
	// 0.2 없어지면 라인으로

	return outImage;
}
// 예상라인들 반환 ( List<pair<int start_Y, int end_Y>> detectedLine)
// 이전 라인들과 비교
// 만약 예상라인이 없어졌다면 없어진라인 분석 (weight로 분석)
// (분석 : 이전프레임에 Unprint 색이 있는지에대한.. etc)


/// <summary>
/// 잡영 회피 목적의 흰색 커튼이미지 생성
/// </summary>
/// <param name="cols">The cols.</param>
/// <param name="rows">The rows.</param>
/// <param name="maskLength">커튼의 길이</param>
/// <param name="targetColum">타깃 좌표.</param>
/// <returns></returns>
Mat imageHandler::getColumMaskImage(int cols, int rows, int maskLength, int targetColum)
{
	Mat maskImage = Mat::zeros(rows, cols, CV_8UC1);
	if (maskLength / 2 > targetColum)
		targetColum = maskLength / 2;
	else if (cols - (maskLength / 2) < targetColum)
		targetColum = cols - (maskLength / 2);

	for (int r = 0; r < rows; r++)
	{
		uchar* yPtr = maskImage.ptr<uchar>(r);
		for (int c = targetColum - (maskLength / 2); c < targetColum + (maskLength / 2); c++)
		{
			yPtr[c] = 255;
		}

	}

	return maskImage;
}

Mat imageHandler::getWhiteMaskImage(Mat mask, int x, int y, int width, int height)
{
	Mat mask_out = mask.clone();
	for (int row = 0; row < height; row++)
	{
		uchar* yPtr = mask_out.ptr<uchar>(row + y);
		for (int col = 0; col < width; col++)
		{
			yPtr[col + x] = 255;
		}
	}
	return mask_out;
}


/// <summary>
/// hsv와 rgb로 필터링 된 이진 이미지를 반환함
/// ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV)) 
/// </summary>
/// <param name="subImage">The subtitle image.</param>
/// <returns>이진화 된 이미지</returns>
Mat imageHandler::getCompositeBinaryImages(Mat& subImage)
{
	Mat image_merged;
	Mat image_binIR_Red = getCompositeBinaryImagesRed(subImage);
	Mat image_binIR_Blue = getCompositeBinaryImagesBlue(subImage);
	bitwise_or(image_binIR_Blue, image_binIR_Red, image_merged);
	return image_merged;
}

/// <summary>
/// 자막의 빨간색의 hsv와 rgb로 필터링 된 이진 이미지를 반환함
/// ((Red_RGB)AND(Red_HSV))
/// </summary>
/// <param name="subImage">The sub image.</param>
/// <returns>빨간색으로 이진화 된 이미지</returns>
Mat imageHandler::getCompositeBinaryImagesRed(Mat& subImage)
{
	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

	Mat image_binIR_RGB_R;
	inRange(subImage, Scalar(0, 0, 130), Scalar(50, 50, 255), image_binIR_RGB_R);	// binarize by rgb

	Mat image_binIR_HSV_R_0;
	Mat image_binIR_HSV_R_1;
	Mat image_binIR_HSV_R;
	inRange(subImage_hsv, Scalar(0, 140, 180), Scalar(1, 255, 255), image_binIR_HSV_R_0); // binarize by hsv
	inRange(subImage_hsv, Scalar(160, 140, 180), Scalar(179, 255, 255), image_binIR_HSV_R_1); // binarize by hsv
	bitwise_or(image_binIR_HSV_R_0, image_binIR_HSV_R_1, image_binIR_HSV_R);	// 총 빨강색 범위 hue = (0~1 + 160~179)

	Mat image_binIR_HSV_R_Filterd = getBorderFloodFilledImage(image_binIR_HSV_R);
	Mat image_binIR_RGB_R_Filterd = getBorderFloodFilledImage(image_binIR_RGB_R);

	Mat image_binIR_Red;
	bitwise_and(image_binIR_RGB_R_Filterd, image_binIR_HSV_R_Filterd, image_binIR_Red);

	return image_binIR_Red;
}

/// <summary>
/// 자막의 파란색의 hsv와 rgb로 필터링 된 이진 이미지를 반환함
/// ((Blue_RGB)AND(Blue_HSV))
/// </summary>
/// <param name="subImage">The sub image.</param>
/// <returns>파란색의 이진화 된 이미지</returns>
Mat imageHandler::getCompositeBinaryImagesBlue(Mat& subImage)
{
	Mat subImage_hsv;	// Scalar (H=색조(180'), S=채도(255), V=명도(255))
	cvtColor(subImage, subImage_hsv, COLOR_BGR2HSV);

	Mat image_binIR_RGB_B;
	inRange(subImage, Scalar(140, 0, 0), Scalar(255, 40, 50), image_binIR_RGB_B);	// binarize by rgb

	Mat image_binIR_HSV_B;
	inRange(subImage_hsv, Scalar(118, 230, 100), Scalar(140, 255, 255), image_binIR_HSV_B);	// binarize by hsv

	Mat image_binIR_HSV_B_Filterd = getBorderFloodFilledImage(image_binIR_HSV_B);
	Mat image_binIR_RGB_B_Filterd = getBorderFloodFilledImage(image_binIR_RGB_B);
	Mat image_binIR_Blue;
	bitwise_and(image_binIR_RGB_B_Filterd, image_binIR_HSV_B_Filterd, image_binIR_Blue);

	return image_binIR_Blue;
}

/// <summary>
/// 입력된 두개의 이진 이미지의 차이점에 대한 이미지 반환
///	A	B	=	C
///	0	0	=	0
///	0	1	=	1
///	1	0	=	1
///	1	1	=	0
/// </summary>
/// <param name="binImageA">The bin image A.</param>
/// <param name="binImageB">The bin image B.</param>
/// <returns></returns>
Mat imageHandler::getDifferenceImage(Mat& binImageA, Mat& binImageB)
{
	Mat xorImage;
	bitwise_xor(binImageA, binImageB, xorImage);

	return xorImage;
}

///	A	B		C		// 0 == black , 1 == white
///	0	0	=	0
///	0	1	=	0
///	1	0	=	1
///	1	1	=	0
Mat imageHandler::getWhiteToBlackImage(static Mat& beforeImage, static Mat& afterImage)
{
	Mat andImage;

	Mat afterImage_not;
	bitwise_not(afterImage, afterImage_not);
	bitwise_and(beforeImage, afterImage_not, andImage);

	return andImage;
}

///	A	B		C		// 0 == black , 1 == white
///	0	0	=	0
///	0	1	=	1
///	1	0	=	0
///	1	1	=	0
Mat imageHandler::getBlackToWhiteImage(Mat& beforeImage, Mat& afterImage)
{
	Mat andImage;

	Mat beforeImage_not;
	bitwise_not(beforeImage, beforeImage_not);
	bitwise_and(afterImage, beforeImage_not, andImage);

	return andImage;
}

/*
	rgbImage에서 mask에 255인 영역만 살림
*/
Mat imageHandler::getMaskedImage(Mat& rgbImage, Mat& mask)
{
	int height = rgbImage.rows;
	int width = rgbImage.cols;

	Mat outImage_masked = rgbImage.clone();
	Vec3b blackColor = { 0,0,0 };
	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_mask = mask.ptr<uchar>(y);	//in
		Vec3b* yPtr_out = outImage_masked.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if (yPtr_mask[x] != 255)
			{
				yPtr_out[x] = blackColor;
			}
		}
	}

	return outImage_masked;
}

// mask의 흰색부분만 살림
Mat imageHandler::getMaskedImage_binImage(static Mat& binImage, static Mat& mask)
{
	int height = binImage.rows;
	int width = binImage.cols;

	Mat outImage_masked = binImage.clone();
	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_mask = mask.ptr<uchar>(y);	//in
		uchar* yPtr_out = outImage_masked.ptr<uchar>(y); //
		for (int x = 0; x < width; x++)
		{
			if (yPtr_mask[x] != 255)
			{
				yPtr_out[x] = 0;
			}
		}
	}

	return outImage_masked;
}

Mat imageHandler::getColorToBlackImage(static Mat& mask, Scalar removeColor)
{
	int height = mask.rows;
	int width = mask.cols;

	Mat outImage_masked = mask.clone();
	Vec3b blackColor = { 0,0,0 };
	// 행연산
	for (int y = 0; y < height; y++)
	{
		Vec3b* yPtr_out = outImage_masked.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if (yPtr_out[x][0] == removeColor[0] &&
				yPtr_out[x][1] == removeColor[1] &&
				yPtr_out[x][2] == removeColor[2])
			{
				yPtr_out[x] = blackColor;
			}
		}
	}

	return outImage_masked;
}

// targetColor로 이루어진 부분을 FCImage에서 찾은 이미지 반환
Mat imageHandler::getBinImageTargetColored(Mat FCImage, Scalar targetColor)
{
	int height = FCImage.rows;
	int width = FCImage.cols;
	Mat outImage;
	outImage = Mat::zeros(FCImage.rows, FCImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = FCImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			if (yPtr_FCImage[x][0] == targetColor[0] &&
				yPtr_FCImage[x][1] == targetColor[1] &&
				yPtr_FCImage[x][2] == targetColor[2])
				yPtr[x] = 255;
		}
	}

	return outImage;
}



/// <summary>
/// 색칠된 부분찾아 흑백으로 표시한 이미지
/// 흰점 : 단순화된 이미지(FullyContrast)에서 흰점 기준 좌우상하 검사함
/// 조건 : 흰점기준으로 한쪽은 빨강or파랑이 2점 이상 연속되고, 반대쪽은 검정이 2점 이상 연속됨 
/// </summary>
/// <param name="srcImage">The source image.</param>
/// <returns></returns>
Mat imageHandler::getPaintedBinImage(Mat& rgbImage)
{
	Mat fullyContrastImage = getFullyContrastImage(rgbImage);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage;
	//cvtColor(fullyContrastImage, outImage, COLOR_BGR2GRAY);
	outImage = Mat::zeros(rgbImage.rows, rgbImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int whiteStart = x;
			int whiteEnd = x;
			bool isRight = false;	// 조건만족?
			if (isWhite(yPtr_FCImage[x]))////if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	
			{
				int color_m = -1; // black==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (whiteEnd < width - 1)	// 어디까지 흰색이냐
				{
					Vec3b v3p = yPtr_FCImage[whiteEnd + 1];
					if (isWhite(v3p))
						whiteEnd++;
					else
						break;
				}

				if (whiteStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[whiteStart - 1];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_m = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteStart - 2]; //Vec3b v3p_ = yPtr_FCImage[whiteEnd + 2]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_m = 2;	// Black
						}
					}
				}

				if (whiteEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[whiteEnd + 1];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteEnd + 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_p = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[whiteEnd + 2]; //FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_p = 2;	// Black
						}
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족

				if (isRight)	// 조건에 만족함
				{
					for (int i = whiteStart; i <= whiteEnd; i++)
					{
						if ((i + 4 > whiteEnd) && (i - 4 < whiteStart))
							yPtr[i] = 255;
					}
				}
				x = whiteEnd;
			}
		}
	}


	// 열연산
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//if (isWhite(outImage.ptr<uchar>(y)[x]))	// 이미 흰색인곳
			//	continue;

			bool isRight = false;	// 조건만족?
			if (isWhite(fullyContrastImage.ptr<Vec3b>(y)[x]))
			{
				int whiteStart = y;
				int whiteEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (whiteEnd < height - 1)	// 어디까지 흰색이냐
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(whiteEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					if (isWhite(v3p))
						whiteEnd++;
					else
						break;
				}

				if (whiteStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(whiteStart - 1)[x];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_m = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteStart - 2)[x];
						if (isBlack(v3p_))
						{
							color_m = 2;	// Black
						}
					}
				}

				if (whiteEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(whiteEnd + 1)[x];
					if (isBlue(v3p) || isRed(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlue(v3p_) || isRed(v3p_))
						{
							color_p = 1;	// B or R
						}
					}
					else if (isBlack(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(whiteEnd + 2)[x];
						if (isBlack(v3p_))
						{
							color_p = 2;	// Black
						}
					}
				}

				if ((color_p == 2 && color_m == 1) || (color_p == 1 && color_m == 2))
					isRight = true;	// 조건만족

				if (isRight)	// 조건에 만족함
				{
					for (int i = whiteStart; i <= whiteEnd; i++)
					{
						if ((i + 4 > whiteEnd) && (i - 4 < whiteStart))
							outImage.ptr<uchar>(i)[x] = 255;
					}
				}
				y = whiteEnd;
			}
		}
	}

	return outImage;
}

/*
	흰 파(빨) 흰 조합인 곳을 찾아냄
*/
Mat imageHandler::getPaintedBinImage_inner(Mat& rgbImage, bool isBluePaint)
{
	Mat fullyContrastImage = getFullyContrastImage(rgbImage);
	//fullyContrastImage = getBorderFloodFilledImageForColor(fullyContrastImage);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage_painted;
	outImage_painted = Mat::zeros(rgbImage.rows, rgbImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_painted = outImage_painted.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if ((isBlue(yPtr_FCImage[x]) && isBluePaint) || (isRed(yPtr_FCImage[x]) && !isBluePaint))
			{
				int color_m = -1; /// black==2, white==1, other==-1
				int color_p = -1;
				while (colorEnd < width - 1)
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if ((isBlue(v3p) && isBluePaint) || (isRed(v3p) && !isBluePaint))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorStart - 1];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_m = 1;	// W
						}
					}
				}

				if (colorEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorEnd + 2];
						if (isWhite(v3p_))
						{
							color_p = 1;	// W
						}
					}
				}
				if ((color_p == 1 && color_m == 1))	// 한쪽이 흰색, 반대쪽도 흰색
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
						yPtr_painted[i] = 255;
				}
				x = colorEnd;
			}
		}
	}

	// 열연산
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//if (isWhite(outImage.ptr<uchar>(y)[x]))	// 이미 흰색인곳
			//	continue;

			bool isRight = false;	// 조건만족?
			if ((isBlue(fullyContrastImage.ptr<Vec3b>(y)[x]) && isBluePaint) || (isRed(fullyContrastImage.ptr<Vec3b>(y)[x]) && !isBluePaint))
			{
				int colorStart = y;
				int colorEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (colorEnd < height - 1)	// 어디까지 컬러인가
				{
					//Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];
					if ((isBlue(v3p) && isBluePaint) || (isRed(v3p) && !isBluePaint))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorStart - 1)[x];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(colorStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_m = 1;	// W
						}
					}
				}

				if (colorEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(colorEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_p = 1;	// W
						}
					}
				}

				if ((color_p == 1 && color_m == 1))	// 한쪽이 흰색, 반대쪽도 흰색
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
					{
						outImage_painted.ptr<uchar>(i)[x] = 255;
					}
				}
				y = colorEnd;
			}
		}
	}

	threshold(outImage_painted, outImage_painted, 200, 255, THRESH_BINARY);

	return outImage_painted;
}


/*
	흰 컬러 흰  조합인 곳을 찾아냄
	bool ifBothHV : 가로 새로 둘다 참인곳만 찾아냄
*/
Mat imageHandler::getPaintedPattern(Mat& rgbImage, Scalar pattenColor, bool ifBothHV)
{
	//Mat fullyContrastImage = getFullyContrastImage(rgbImage);
	Mat fullyContrastImage = imageHandler::getFullyContrast_withDilate(rgbImage, pattenColor);

	int height = fullyContrastImage.rows;
	int width = fullyContrastImage.cols;
	Mat outImage_painted;
	outImage_painted = Mat::zeros(rgbImage.rows, rgbImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_painted = outImage_painted.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = fullyContrastImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if ((isSameColor(pattenColor, yPtr_FCImage[x])))
			{
				int color_m = -1; /// black==2, white==1, other==-1
				int color_p = -1;
				while (colorEnd < width - 1)
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isSameColor(pattenColor, v3p))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorStart - 1];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_m = 1;	// W
						}
					}
				}

				if (colorEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorEnd + 2];
						if (isWhite(v3p_))
						{
							color_p = 1;	// W
						}
					}
				}
				if ((color_p == 1 && color_m == 1))	// 한쪽이 흰색, 반대쪽도 흰색
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
						yPtr_painted[i] = 128;
				}
				x = colorEnd;
			}
		}
	}

	// 열연산
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//if (isWhite(outImage.ptr<uchar>(y)[x]))	// 이미 흰색인곳
			//	continue;

			bool isRight = false;	// 조건만족?
			if (isSameColor(pattenColor, fullyContrastImage.ptr<Vec3b>(y)[x]))
			{
				int colorStart = y;
				int colorEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (colorEnd < height - 1)	// 어디까지 컬러인가
				{
					//Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isSameColor(pattenColor, v3p))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorStart - 1)[x];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(colorStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_m = 1;	// W
						}
					}
				}

				if (colorEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isWhite(v3p))
					{
						Vec3b v3p_ = fullyContrastImage.ptr<Vec3b>(colorEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isWhite(v3p_))
						{
							color_p = 1;	// W
						}
					}
				}

				if ((color_p == 1 && color_m == 1))	// 한쪽이 흰색, 반대쪽도 흰색
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
					{
						outImage_painted.ptr<uchar>(i)[x] += 127;	// 128 or 127 or 255
					}
				}
				y = colorEnd;
			}
		}
	}

	if (ifBothHV)
		threshold(outImage_painted, outImage_painted, 200, 255, THRESH_BINARY);
	else
		threshold(outImage_painted, outImage_painted, 100, 255, THRESH_BINARY);

	return outImage_painted;
}

Mat imageHandler::getFullyContrastImage(Mat rgbImage)
{
	// RGB 분리
	Mat bgr[3];
	split(rgbImage, bgr);
	// 각각의 128 임계로 한 이미지 얻음
	threshold(bgr[0], bgr[0], 127, 255, THRESH_BINARY);
	threshold(bgr[1], bgr[1], 127, 255, THRESH_BINARY);
	threshold(bgr[2], bgr[2], 127, 255, THRESH_BINARY);
	// RGB 합침
	Mat mergedImage;
	merge(bgr, 3, mergedImage);
	return mergedImage;
}
//408(163, 104, 140) : (40, 25, 34)	
/*
	픽셀의 RGB 비율에 따른 ContrastImage 생성
	<조건들>
	검정	: RGB중 가장 큰 값이 50 이하 AND
	흰색	: RGB값 모두 128 이상
	나머지 색에서 색 구분 진행
	파랑(255 0 0)	:
	빨강(0 0 255)	:
	초록(0 255 0)	:
	노랑(0 255 255)	:
	퍼플(255 0 255)	:
	하늘(255 255 0)	:
	3개 색에 대한 비율 구함. ( B/RGB합*100 : G/RGB합*100  : R/RGB합*100 )
*/	// 패턴 찾아낼 때 사용(색처리에 대해 관대함)
Mat imageHandler::getPixelContrastImage(Mat rgbImage)
{
	int height = rgbImage.rows;
	int width = rgbImage.cols;
	Mat outImage = rgbImage.clone();

	Scalar color_white = Scalar(255, 255, 255);
	Scalar color_black = Scalar(0, 0, 0);

	Scalar color_red = Scalar(0, 0, 255);
	Scalar color_green = Scalar(0, 255, 0);
	Scalar color_blue = Scalar(255, 0, 0);
	Scalar color_purple = Scalar(255, 0, 255);
	Scalar color_yellow = Scalar(0, 255, 255);
	Scalar color_sky = Scalar(255, 255, 0);

	vector<Scalar> otherColor;
	otherColor.push_back(color_red);
	otherColor.push_back(color_green);
	otherColor.push_back(color_blue);
	otherColor.push_back(color_purple);
	otherColor.push_back(color_yellow);
	otherColor.push_back(color_sky);
	//Scalar otherColor[] = { color_red, color_green, color_blue, color_purple, color_yellow, color_sky};

	// 행연산
	for (int y = 0; y < height; y++)
	{
		//uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = outImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			// white
			if (yPtr_FCImage[x][0] > 127 &&
				yPtr_FCImage[x][1] > 127 &&
				yPtr_FCImage[x][2] > 127)
			{
				yPtr_FCImage[x][0] = color_white[0];
				yPtr_FCImage[x][1] = color_white[1];
				yPtr_FCImage[x][2] = color_white[2];
				continue;
			}

			// black
			int maxValue = yPtr_FCImage[x][0];
			for (int i = 0; i < 3; i++)
				if (yPtr_FCImage[x][i] > maxValue)
					maxValue = yPtr_FCImage[x][i];
			if (maxValue < 50)
			{
				yPtr_FCImage[x][0] = color_black[0];
				yPtr_FCImage[x][1] = color_black[1];
				yPtr_FCImage[x][2] = color_black[2];
				continue;
			}

			// other colors
			Scalar imagePixel = yPtr_FCImage[x];
			int minValue = 255 * 3; // 최저 값 
			for (int i = 0; i < otherColor.size(); i++)
			{
				int value = abs(otherColor[i][0] - imagePixel[0]) +
					abs(otherColor[i][1] - imagePixel[1]) +
					abs(otherColor[i][2] - imagePixel[2]);
				if (value < minValue)	// 색 업데이트 : 가장 낮은 값을 갖는 Scalar계산이 가장 가까운 색
				{
					yPtr_FCImage[x][0] = otherColor[i][0];
					yPtr_FCImage[x][1] = otherColor[i][1];
					yPtr_FCImage[x][2] = otherColor[i][2];
					minValue = value;
				}
			}

		}
	}

	return outImage;
}

/*
	위 알고리즘과 비슷하지만 변형버전
	<조건들>
	검정	: RGB중 가장 큰 값이 50 이하 AND
	흰색	: RGB값 모두 128 이상
	나머지 색에서 색 구분 진행
	파랑: B값 퍼센테이지 50% 이상
	빨강: R값 퍼센테이지 50% 이상
	초록: G값 퍼센테이지 50% 이상
	노랑: GR값 각 퍼센테이지 35% 이상
	퍼플: BR값 각 퍼센테이지 35% 이상
	하늘: BG값 각 퍼센테이지 35% 이상
*/	// 패턴위치에 flodfill 할때 사용 (컬러)
Mat imageHandler::getPixelContrastImage_byPercent(Mat rgbImage)
{
	int height = rgbImage.rows;
	int width = rgbImage.cols;
	Mat outImage = rgbImage.clone();

	Scalar color_white = Scalar(255, 255, 255);
	Scalar color_black = Scalar(0, 0, 0);
	Vec3b color_red = Vec3b(0, 0, 255);
	Vec3b color_green = Vec3b(0, 255, 0);
	Vec3b color_blue = Vec3b(255, 0, 0);
	Vec3b color_purple = Vec3b(255, 0, 255);
	Vec3b color_yellow = Vec3b(0, 255, 255);
	Vec3b color_sky = Vec3b(255, 255, 0);

	//Scalar otherColor[] = { color_red, color_green, color_blue, color_purple, color_yellow, color_sky};

	// 행연산
	for (int y = 0; y < height; y++)
	{
		//uchar* yPtr = outImage.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = outImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			// white
			if (yPtr_FCImage[x][0] > 127 &&
				yPtr_FCImage[x][1] > 127 &&
				yPtr_FCImage[x][2] > 127)
			{
				yPtr_FCImage[x][0] = color_white[0];
				yPtr_FCImage[x][1] = color_white[1];
				yPtr_FCImage[x][2] = color_white[2];
				continue;
			}

			// black
			int maxValue = yPtr_FCImage[x][0];
			for (int i = 0; i < 3; i++)
				if (yPtr_FCImage[x][i] > maxValue)
					maxValue = yPtr_FCImage[x][i];
			if (maxValue < 50)
			{
				yPtr_FCImage[x][0] = color_black[0];
				yPtr_FCImage[x][1] = color_black[1];
				yPtr_FCImage[x][2] = color_black[2];
				continue;
			}

			// other colors
			Scalar imagePixel = yPtr_FCImage[x];
			int SumPixelValue = imagePixel[0] + imagePixel[1] + imagePixel[2];
			float percentR = imagePixel[2] / (float)SumPixelValue;
			float percentG = imagePixel[1] / (float)SumPixelValue;
			float percentB = imagePixel[0] / (float)SumPixelValue;

			if (percentB > 0.5)
				yPtr_FCImage[x] = color_blue;
			else if (percentR > 0.5)
				yPtr_FCImage[x] = color_red;
			else if (percentG > 0.5)
				yPtr_FCImage[x] = color_green;
			else if (percentR > 0.35 && percentB > 0.35)	//purple
				yPtr_FCImage[x] = color_purple;
			else if (percentR > 0.35 && percentB > 0.35)	//yellow
				yPtr_FCImage[x] = color_yellow;
			else if (percentR > 0.35 && percentB > 0.35)	//sky
				yPtr_FCImage[x] = color_sky;
			else // 검정처리
				yPtr_FCImage[x] = { 0, 0, 0 };

		}
	}

	return outImage;
}

Mat imageHandler::getFullyContrast_withDilate(Mat rgbImage, Scalar color)
{
	//Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);
	Mat fullyContrastImage = imageHandler::getPixelContrastImage(rgbImage);

	Scalar targetColorMin = color;
	for (int i = 0; i < 3; i++)
	{
		if (targetColorMin[i] != 0)
			targetColorMin[i] = targetColorMin[i] - 1;
	}

	Mat colorMat;
	//inRange(fullyContrastImage, Scalar(254, 0, 0), Scalar(255, 0, 0), blue);	// binarize by rgb
	inRange(fullyContrastImage, targetColorMin, color, colorMat);

	Mat colorMat_dilate = imageHandler::getMorphImage(colorMat, MORPH_DILATE);

	int height = rgbImage.rows;
	int width = rgbImage.cols;

	for (int y = 0; y < height; y++)
	{
		Vec3b* yPtr_FC = fullyContrastImage.ptr<Vec3b>(y);
		uchar* yPtr_colorMat = colorMat_dilate.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr_colorMat[x] == 255)
			{
				if ((!isWhite(yPtr_FC[x])) && (!isBlack(yPtr_FC[x])))	// 흑 백도 아닌곳만 적용
				{
					Vec3b& ptr = fullyContrastImage.at<Vec3b>(y, x);
					//ptr[0] = 255;	// B
					//ptr[1] = 0;	// G
					//ptr[2] = 0;	// R

					ptr[0] = color[0];
					ptr[1] = color[1];
					ptr[2] = color[2];

				}

			}
		}
	}
	return fullyContrastImage;
}

Mat imageHandler::getSharpenAndContrastImage(Mat rgbImage)
{			// 빵구매꾸기 위해 두번 함
	Mat sharpenImage;
	sharpenImage = getSharpenImage(rgbImage);
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

Mat imageHandler::getSharpenImage(Mat srcImage)
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

/// <summary>
/// ATImage에 compositeImage의 흰색점인 좌표에 floodfill 연산을 하고 나온 결과물을 반환
/// </summary>
/// <param name="ATImage">AdoptedThresold()의 결과이미지.</param>
/// <param name="compositeImage"> ((Red_RGB)AND(Red_HSV)) OR ((Blue_RGB)AND(Blue_HSV))의 결과이미지.</param>
/// <returns></returns>
Mat imageHandler::getBinImageByFloodfillAlgorism(Mat ATImage, Mat compositeImage)
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

/*
 1. 원본이미지에서 흰색만 남김
 2.
*/
// orgimage의 흰색영역에 floodfill한 결과
Mat imageHandler::getFloodfillImage(Mat orgImage, Mat toWhiteTarget)
{
	//Mat filteredImage_BGR = orgImage.clone();
	Mat filteredImage_BGR;
	inRange(orgImage, Scalar(254, 254, 254), Scalar(255, 255, 255), filteredImage_BGR);
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
			if (toWhiteTarget.at<uchar>(Point(x, y)) != 0 && color == whiteColor)	// 좌표색이 흰색이면
				floodFill(filteredImage_BGR, Point(x, y), redColor);
		}
	}
	// 2. 빨간색으로 inRange() 하여 다시 흰색 이미지를 얻음
	Mat filteredImage;
	inRange(filteredImage_BGR, Scalar(0, 0, 254), Scalar(0, 0, 255), filteredImage);	// binarize by rgb

	return filteredImage;
}

// BinImage의 최소, 최대값 구한 후 해당 값 +-10 Vertical까지 자름
Mat imageHandler::cutColumByHistorgram(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);
	int highPoint = 0;
	int lowPoint = projection.size();
	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)
		{
			highPoint = i;
			break;
		}
	}
	for (int i = projection.size() - 1; i >= 0; i--)
	{
		if (projection[i] != 0)
		{
			lowPoint = i;
			break;
		}
	}

	if (highPoint == 0)
		return binImage;
	else
	{
		if (highPoint - 10 > 0)
			highPoint = highPoint - 10;
		else
			highPoint = 0;

		if (lowPoint + 10 < binImage.rows)
			lowPoint = lowPoint + 10;
		else
			lowPoint = binImage.rows;

		Rect subRect(0, highPoint, binImage.cols, lowPoint - highPoint);
		Mat subImage = binImage(subRect);
		return subImage;

	}
}

// 이진 이미지에서 발음 부분을 지우기 위한 함수
Mat imageHandler::removeSubLyricLine(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);

	vector<pair<int, int>> islands;
	vector<int> islandsVolum;

	// 섬의 갯수 구함, 
	// 섬이 2개 이상이면서, 가장 아래섬이 흰점수 2등 이면
	// 보조 lyric으로 판별하여 2등 제거
	/*	or 가장 큰 volume의 Lyric만 남김 */

	bool blackZone = false;
	int startPoint = 0;
	int endPoint = 0;
	int volume = 0;

	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)	// Not blackZone
		{
			if (blackZone == true)	// blackZone -> Not BlackZone
			{
				startPoint = i;
			}

			volume += projection[i];
			blackZone = false;
		}
		else // blackZone
		{
			if (blackZone == false)	// Not blackZone -> blackZone
			{	// save island
				islands.push_back(make_pair(startPoint, i));
				islandsVolum.push_back(volume);
				volume = 0;
			}
			//
			blackZone = true;
		}
	}

	Mat maskingImage = binImage.clone();

	if (islands.size() >= 2)
	{
		vector<int> islandsVolum_sort;
		islandsVolum_sort.assign(islandsVolum.begin(), islandsVolum.end());	// copy vector

		sort(islandsVolum_sort.begin(), islandsVolum_sort.end(), greater<int>());

		if (islandsVolum_sort[1] == islandsVolum[islandsVolum.size() - 1])	// 가장 아래섬이 흰점수 2등
		{
			int startPoint = islands[islands.size() - 1].first;
			int finishPoint = islands[islands.size() - 1].second;

			for (int i = startPoint; i <= finishPoint; i++)
			{
				for (int c = 0; c < maskingImage.cols; c++)
				{
					if (maskingImage.at<uchar>(i, c) != 0)	// (rows, cols)
						floodFill(maskingImage, Point(c, i), 0);	// (cols, rows)
				}
			}
			return maskingImage;
		}
	}

	return maskingImage;
}

Mat imageHandler::removeSubLyricLine2(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);

	vector<pair<int, int>> islands;
	vector<int> islandsVolum;

	// 섬의 갯수 구함, 
	// 섬의 개수가 2개 이상이면 가장 부피가 큰 섬 기준으로 아래섬 삭제

	bool blackZone = false;
	int startPoint = 0;
	int endPoint = 0;
	int volume = 0;

	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)	// Not blackZone
		{
			if (blackZone == true)	// blackZone -> Not BlackZone
			{
				startPoint = i;
			}

			volume += projection[i];
			blackZone = false;
		}
		else // blackZone
		{
			if (blackZone == false)	// Not blackZone -> blackZone
			{	// save island
				islands.push_back(make_pair(startPoint, i));
				islandsVolum.push_back(volume);
				volume = 0;
			}
			//
			blackZone = true;
		}
	}

	int maximumVolumeIslandVolume = 0;
	int maximumVolumeIslandIndex = 0;	// 가장 부피가 큰 섬 indxe
	for (int i = 0; i < islandsVolum.size(); i++)
	{
		if (islandsVolum[i] > maximumVolumeIslandVolume)
		{
			maximumVolumeIslandVolume = islandsVolum[i];
			maximumVolumeIslandIndex = i;
		}
	}

	Mat maskingImage = Mat::zeros(binImage.rows, binImage.cols, CV_8U); 
	{	// 가장 부피가 큰 섬 endY 좌표까지의 마스크 생성
		// bitwiseand 연산수행
		maskingImage = getWhiteMaskImage(maskingImage, 0, 0, binImage.cols, islands[maximumVolumeIslandIndex].second);
		bitwise_and(maskingImage, binImage, maskingImage);
	}

	return maskingImage;
}

Mat imageHandler::removeNotPrimeryLyricLine(Mat binImage)
{
	vector<int> projection = getHorizontalProjectionData(binImage);

	vector<pair<int, int>> islands;
	vector<int> islandsVolum;
	vector<int> islandsHeight;

	// 섬의 갯수 구함, 
	// 섬이 2개 이상이면서, 가장 아래섬이 흰점수 2등 이면
	// 보조 lyric으로 판별하여 2등 제거

	bool blackZone = false;
	int startPoint = 0;
	int endPoint = 0;
	int volume = 0;

	for (int i = 0; i < projection.size(); i++)
	{
		if (projection[i] != 0)	// Not blackZone
		{
			if (blackZone == true)	// blackZone -> Not BlackZone
			{
				startPoint = i;
			}

			volume += projection[i];
			blackZone = false;
		}
		else // blackZone
		{
			if (blackZone == false)	// Not blackZone -> blackZone
			{	// save island
				islands.push_back(make_pair(startPoint, i));
				islandsVolum.push_back(volume);
				islandsHeight.push_back(i- startPoint);
				volume = 0;
			}
			//
			blackZone = true;
		}
	}

	Mat maskingImage = binImage.clone();

	if (islands.size() > 1)
	{
		/*
		int biggistIsland = 0;
		int biggistIslandVolume = 0;
		for (int i = 0; i < islandsVolum.size(); i++)
		{
			if (biggistIslandVolume < islandsVolum[i])
			{
				biggistIsland = i;
				biggistIslandVolume = islandsVolum[i];
			}
		}		
		*/

		int biggistIsland = 0;
		int biggistIslandHeight = 0;
		for (int i = 0; i < islandsHeight.size(); i++)
		{
			if (biggistIslandHeight < islandsHeight[i])
			{
				biggistIsland = i;
				biggistIslandHeight = islandsHeight[i];
			}
		}

		int primaryLyricStartPoint = islands[biggistIsland].first;
		int primaryLyricEndPoint = islands[biggistIsland].second;
		for (int i = 0; i < projection.size(); i++)
		{
			if (i<primaryLyricStartPoint || i> primaryLyricEndPoint)
			{
				for (int c = 0; c < maskingImage.cols; c++)
				{
					if (maskingImage.at<uchar>(i, c) != 0)	// (rows, cols)
						floodFill(maskingImage, Point(c, i), 0);	// (cols, rows)
				}
			}
		}
	}

	return maskingImage;
}

bool imageHandler::isWhite(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 255 && ptr[2] == 255)
		return true;
	return false;
}

bool imageHandler::isBlack(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool imageHandler::isBlue(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 255 && ptr[1] == 0 && ptr[2] == 0)
		return true;
	return false;
}

bool imageHandler::isRed(const Vec3b& ptr)
{	// BGR 
	if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 255)
		return true;
	return false;
}

bool imageHandler::isSameColor(const Scalar& sColor, const Vec3b& vColor)
{
	if (sColor[0] == vColor[0] &&
		sColor[1] == vColor[1] &&
		sColor[2] == vColor[2])
		return true;
	return false;
}

/// <summary>
/// 이진 이미지의 Vertical projection 데이터로 변환 
/// </summary>
/// <param name="binImage">변환할 이진 이미지.</param>
/// <returns>Vertical projection 데이터</returns>
vector<int> imageHandler::getVerticalProjectionData(Mat binImage)
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

Mat imageHandler::getVerticalProjection(Mat binMat)
{
	vector<int> counts;
	counts = getVerticalProjectionData(binMat);

	double sum = 0;
	for (int i = 0; i < counts.size(); i++)
		sum += counts[i];
	int avg = 0;
	if (counts.size() != 0)
		avg = sum / counts.size();

	int nRows = binMat.rows;
	int nCols = binMat.cols;

	Mat outImage;
	binMat.copyTo(outImage);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			outImage.at<uchar>(j, i) = 0;	// init;

			if (counts[i] > j)
			{
				if (j > avg)
					outImage.at<uchar>(j, i) = 255;
				else
					outImage.at<uchar>(j, i) = 255 / 2;
			}
		}
	}

	return outImage;
}

int imageHandler::getVerticalAvgPoint(Mat binMat)
{
	vector<int> counts;
	counts = getVerticalProjectionData(binMat);

	double sum = 0;
	for (int i = 0; i < counts.size(); i++)
		sum += counts[i];
	int avg = 0;
	if (counts.size() != 0)
		avg = sum / counts.size();
	return avg;
}

/// <summary>
/// 이진 이미지의 Horizontal projection 데이터로 변환 
/// </summary>
/// <param name="binImage">변환할 이진 이미지.</param>
/// <returns>Horizontal projection 데이터</returns>
vector<int> imageHandler::getHorizontalProjectionData(Mat binImage)
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

Mat imageHandler::getHorizontalProjection(Mat binMat)
{
	vector<int> counts;
	counts = getHorizontalProjectionData(binMat);

	double sum = 0;
	for (int i = 0; i < counts.size(); i++)
		sum += counts[i];
	int avg = 0;
	if (counts.size() != 0)
		avg = sum / counts.size();

	int nRows = binMat.rows;
	int nCols = binMat.cols;

	Mat outimage;
	binMat.copyTo(outimage);

	for (int j = 0; j < nRows; j++) {
		for (int i = 0; i < nCols; i++) {
			outimage.at<uchar>(j, i) = 0;	// init;

			if (counts[j] > i)
			{
				if (i > avg)
					outimage.at<uchar>(j, i) = 255;
				else
					outimage.at<uchar>(j, i) = 255 / 2;
			}
		}
	}

	return outimage;
}

int imageHandler::getHorizontalAvgPoint(Mat binMat)
{
	vector<int> counts;
	counts = getHorizontalProjectionData(binMat);

	double sum = 0;
	for (int i = 0; i < counts.size(); i++)
		sum += counts[i];
	int avg = 0;
	if (counts.size() != 0)
		avg = sum / counts.size();
	return avg;

}

/// <summary>
/// Mat의 흰색 점의 개수를 반환함
/// </summary>
/// <param name="binImage">The binary mat.</param>
/// <returns>흰색 점의 개수</returns>
int imageHandler::getWhitePixelCount(Mat binImage)
{
	int height = binImage.rows;
	int width = binImage.cols;
	int whiteCount = 0;
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// 흑색이 아닌경우
				whiteCount++;
	}
	// printf("whiteCount:%d\r\n", whiteCount);
	return whiteCount;
}

// bin image의 흰점들의 평균점을 구함
int imageHandler::getWhitePixelAvgCoordinate(Mat binImage, bool isXcoordinate)
{
	vector<pair<int, int>> whitePixels;
	whitePixels = getWhitePixels(binImage);
	int avg = 0;
	int sum = 0;
	for (int i = 0; i < whitePixels.size(); i++)
	{
		if (isXcoordinate)
			sum += whitePixels[i].first;
		else
			sum += whitePixels[i].second;
	}
	if (whitePixels.size() != 0)
		avg = sum / whitePixels.size();
	else
		avg = 0;

	return avg;
}

// binImage의 흰점들의 평균 값을 반환
int imageHandler::getWhitePixelAvgValue(Mat binImage)
{
	vector<pair<int, int>> whitePixels;
	whitePixels = getWhitePixels(binImage);
	if (whitePixels.size() == 0)
		return 0;

	int sum = 0;
	for (int i = 0; i < whitePixels.size(); i++)
	{
		uchar* yPtr = binImage.ptr<uchar>(whitePixels[i].second);
		sum += yPtr[whitePixels[i].first];
	}

	return sum / whitePixels.size();
}

// 흰점의 좌표들을 얻어냄
vector<pair<int, int>> imageHandler::getWhitePixels(Mat binImage)
{
	vector<pair<int, int>> whitePixels;

	int height = binImage.rows;
	int width = binImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
			if (yPtr[x] != 0)	// 흑색이 아닌경우
				whitePixels.push_back(make_pair(x, y));
	}

	return whitePixels;
}

int imageHandler::getLeftistWhitePixel_x(Mat binImage)
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

int imageHandler::getRightistWhitePixel_x(Mat binImage)
{
	int rightist_x = 0;

	//int height = binImage.rows;
	//int width = binImage.cols;

	for (int width = binImage.cols - 1; width > 0; width--)
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
int imageHandler::getRightistWhitePixel_x(Mat binImage, int targetStartX, int range, int threshold)
{
	// get Vertical Projection
	// 대상 x 부터 x+range 까지 검사한 것 중 thresold보다 큰 값,
	// 만약 최대 x가 결과라면 그 앞쪽까지 더 확인?
	vector<int> vecVProjection = imageHandler::getVerticalProjectionData(binImage);

	// 뒤에서부터 검사
	for (int i = targetStartX + range; i > targetStartX; i--)
	{
		if (vecVProjection.size() <= i)
			break;

		if (vecVProjection[i] >= threshold)
		{
			if (targetStartX + range == i)	//
				i = getRightistWhitePixel_x(binImage, targetStartX + range, range, threshold);
			return i;
		}
	}

	return targetStartX;	// 못찾으면 값 유지 
}

uchar imageHandler::getLeftistWhitePixel_value(Mat binImage)
{
	uchar leftist_value = 0;

	for (int width = 0; width < binImage.cols; width++)
	{
		for (int hight = 0; hight < binImage.rows; hight++)
		{
			if (binImage.at<uchar>(hight, width) != 0)
				return binImage.at<uchar>(hight, width);
		}
	}
	return leftist_value;
}

// 편중된 색상에 대한 흑백 이미지를 얻음
Mat imageHandler::getBiasedColorImage(Mat rgbImage, Color biasedColor)
{
	int height = rgbImage.rows;
	int width = rgbImage.cols;

	Mat outImage_painted;
	outImage_painted = Mat::zeros(rgbImage.rows, rgbImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		Vec3b* yPtr_RGBImage = rgbImage.ptr<Vec3b>(y); //in	BGR [0~2]
		uchar* yPtr_painted = outImage_painted.ptr<uchar>(y);	//

		for (int x = 0; x < width; x++)
		{
			if (yPtr_RGBImage[x][biasedColor] > 10)
			{
				float ratio = 1;
				switch (biasedColor)
				{
				case Color::BLUE:
					ratio = (yPtr_RGBImage[x][1] + yPtr_RGBImage[x][2]) / yPtr_RGBImage[x][0];	// b g r   0에 가까울수록 더 불루;
					break;
				case Color::GREEN:
					ratio = (yPtr_RGBImage[x][0] + yPtr_RGBImage[x][2]) / yPtr_RGBImage[x][1];
					break;
				case Color::RED:
					ratio = (yPtr_RGBImage[x][0] + yPtr_RGBImage[x][1]) / yPtr_RGBImage[x][2];
					break;
				}
				if (ratio < 1)
					yPtr_painted[x] = 255;
			}
		}
	}

	return outImage_painted;
}

Mat imageHandler::getBWBPatternImage(Mat FCImage)
{
	int height = FCImage.rows;
	int width = FCImage.cols;

	Mat outImage_painted;
	outImage_painted = Mat::zeros(FCImage.rows, FCImage.cols, CV_8U);

	// 행연산
	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_painted = outImage_painted.ptr<uchar>(y);	//in
		Vec3b* yPtr_FCImage = FCImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			int colorStart = x;
			int colorEnd = x;
			bool isRight = false;	// 조건만족?

			if (isWhite(yPtr_FCImage[x]))
			{
				int color_m = -1; /// black==2, white==1, other==-1
				int color_p = -1;
				while (colorEnd < width - 1)
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isWhite(v3p))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorStart - 1];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorStart - 2]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_m = 2;	// B
						}
					}
				}

				if (colorEnd + 2 < width)	// 연속되는지 확인
				{
					Vec3b v3p = yPtr_FCImage[colorEnd + 1];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = yPtr_FCImage[colorEnd + 2];
						if (isBlack(v3p_))
						{
							color_p = 2;	// B
						}
					}
				}
				if ((color_p == 2 && color_m == 2))	// 검-흰-검 패턴
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
						yPtr_painted[i] = 255;
				}
				x = colorEnd;
			}
		}
	}

	// 열연산
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			//if (isWhite(outImage.ptr<uchar>(y)[x]))	// 이미 흰색인곳
			//	continue;

			bool isRight = false;	// 조건만족?
			if (isWhite(FCImage.ptr<Vec3b>(y)[x]))
			{
				int colorStart = y;
				int colorEnd = y;
				int color_m = -1; // lack==2, Blue,Red==1, other==-1
				int color_p = -1;

				while (colorEnd < height - 1)	// 어디까지 컬러인가
				{
					//Vec3b v3p = fullyContrastImage.ptr<Vec3b>(colorEnd + 1)[x];//yPtr_FCImage[whiteEnd + 1];
					Vec3b v3p = FCImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isWhite(v3p))
						colorEnd++;
					else
						break;
				}

				if (colorStart - 2 > 0)	// 연속되는지 확인
				{
					Vec3b v3p = FCImage.ptr<Vec3b>(colorStart - 1)[x];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = FCImage.ptr<Vec3b>(colorStart - 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_m = 2;	// Black
						}
					}
				}

				if (colorEnd + 2 < height)	// 연속되는지 확인
				{
					Vec3b v3p = FCImage.ptr<Vec3b>(colorEnd + 1)[x];
					if (isBlack(v3p))
					{
						Vec3b v3p_ = FCImage.ptr<Vec3b>(colorEnd + 2)[x]; // FCImage.at<cv::Vec3b>(y, x - (count + 1));
						if (isBlack(v3p_))
						{
							color_p = 2;	// B
						}
					}
				}

				if ((color_p == 2 && color_m == 2))	// 검-흰-검
					isRight = true;

				if (isRight)	// 조건에 만족함
				{
					for (int i = colorStart; i <= colorEnd; i++)
					{
						outImage_painted.ptr<uchar>(i)[x] = 255;
					}
				}
				y = colorEnd;
			}
		}
	}

	threshold(outImage_painted, outImage_painted, 200, 255, THRESH_BINARY);

	return outImage_painted;
}

void imageHandler::stackFCImage(Mat FCImage, Mat FCImage_before, Mat& stackBinImage, Mat maskImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = stackBinImage.ptr<uchar>(y);
		Vec3d* yPtr_FC = FCImage.ptr<Vec3d>(y);
		Vec3d* yPtr_FC_before = FCImage_before.ptr<Vec3d>(y);
		uchar* yPtr_Mask = maskImage.ptr<uchar>(y);
		for (int x = 0; x < width; x++)
		{
			/*printf("%d ", yPtr_FC_before[x][0]);
			printf("%d ", yPtr_FC_before[x][1]);
			printf("%d \r\n", yPtr_FC_before[x][2]);*/
			/*
			On << mask ==1, before==w, cur==color
			bin이 0일때

			*/

			// 누적법
			if (yPtr[x] == 0)	// 0인곳 
			{
				if (yPtr_Mask[x] != 255)	// 패턴에 부합하는곳
					yPtr[x] = 0;
				else if (isWhite(FCImage_before.at<cv::Vec3b>(y, x)) && isBlue(FCImage.at<cv::Vec3b>(y, x)))
				{
					//yPtr[x] = 10;
					yPtr[x] = 255;
				}
				else if (isWhite(FCImage_before.at<cv::Vec3b>(y, x)) && isRed(FCImage.at<cv::Vec3b>(y, x)))
				{
					//yPtr[x] = 10;
					yPtr[x] = 255;
				}
			}
			else
			{
				if (isBlue(FCImage.at<cv::Vec3b>(y, x)) || isRed(FCImage.at<cv::Vec3b>(y, x)))
				{
					//if (yPtr[x] <= 245)	// 증감
					//	yPtr[x] += 10;
					//else
					yPtr[x] = 255;
				}
				else
					yPtr[x] = 0;
			}
		}
	}
	;
}

void imageHandler::stackFCImage_BlackToWhite(Mat subImage, Mat subImage_before, Mat& stackBinImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	int newDot = 0;
	int delDot = 0;
	Mat subImage_black;
	Mat subImage_white;
	Mat subImageBefore_black;
	Mat subImageBefore_white;
	inRange(subImage, Scalar(0, 0, 0), Scalar(125, 125, 125), subImage_black);
	inRange(subImage, Scalar(126, 126, 126), Scalar(255, 255, 255), subImage_white);
	inRange(subImage_before, Scalar(0, 0, 0), Scalar(125, 125, 125), subImageBefore_black);
	inRange(subImage_before, Scalar(126, 126, 126), Scalar(255, 255, 255), subImageBefore_white);

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = stackBinImage.ptr<uchar>(y);
		//Vec3d* yPtr_FC = FCImage.ptr<Vec3d>(y);
		//Vec3d* yPtr_FC_before = FCImage_before.ptr<Vec3d>(y);
		uchar* yPtr_sub_black = subImage_black.ptr<uchar>(y);
		uchar* yPtr_sub_white = subImage_white.ptr<uchar>(y);
		uchar* yPtr_subBefore_black = subImageBefore_black.ptr<uchar>(y);
		uchar* yPtr_subBefore_white = subImageBefore_white.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{

			// 누적법
			if (yPtr[x] == 0)	// 0인곳  On 조건
			{
				if (yPtr_subBefore_black[x] == 0 && yPtr_sub_white[x] == 255)
					//if (isBlack(FCImage_before.at<cv::Vec3b>(y, x)) && isWhite(FCImage.at<cv::Vec3b>(y, x)))
				{
					yPtr[x] = 255;
					newDot++;
				}
			}
			else
			{
				if (yPtr_sub_white[x] == 255)
					//if (isWhite(FCImage.at<cv::Vec3b>(y, x)))	// 유지
				{
					yPtr[x] = 255;
				}
				else  // 0이 아닌곳 Off 함
				{
					yPtr[x] = 0;
					delDot++;
				}
			}

		}
	}
	cout << "newDot = " << newDot << " delDot = " << delDot;
	;
}

// black(0, 0, 0)이 아닌 점들의 컬러 평균을 출력 (bgr)
// black(0, 0, 0)을 제외한 가장 많은 컬러 출력
Vec3b imageHandler::getMostHaveRGB(static Mat rgbImage)
{
	vector<pair<Vec3b, int>> vecColorCount;

	int height = rgbImage.rows;
	int width = rgbImage.cols;

	Vec3b blackColor = { 0,0,0 };

	//행연산
	for (int y = 0; y < height; y++)
	{
		Vec3b* yPtr = rgbImage.ptr<Vec3b>(y); //
		for (int x = 0; x < width; x++)
		{
			if (!(isBlack(yPtr[x])))
			{
				Vec3b val = yPtr[x];

				bool isFind = false;
				for (int i = 0; i < vecColorCount.size(); i++)
				{
					if (vecColorCount[i].first[0] == val[0] &&
						vecColorCount[i].first[1] == val[1] &&
						vecColorCount[i].first[2] == val[2])
					{
						vecColorCount[i].second += 1;
						isFind = true;
						break;
					}
				}
				if (isFind != true)
				{
					vecColorCount.push_back(make_pair(val, 1));
				}

			}
		}
	}
	if (vecColorCount.size() < 1)
		return blackColor;

	Vec3b maxColor = vecColorCount[0].first;
	int maxCount = vecColorCount[0].second;
	for (int i = 0; i < vecColorCount.size(); i++)
	{
		//		printf("Color %d : (%d %d %d), %d \r\n", i, vecColorCount[i].first[0], vecColorCount[i].first[1], vecColorCount[i].first[2], vecColorCount[i].second);
		if (vecColorCount[i].second > maxCount)
		{
			maxCount = vecColorCount[i].second;
			maxColor = vecColorCount[i].first;
		}
	}

	return maxColor;
}

Vec3b imageHandler::getMostHaveRGB(vector<Vec3b> vecColor)
{
	Vec3b blackColor = { 0,0,0 };

	vector<pair<Vec3b, int>> vecColorCount;

	for (int i = 0; i < vecColor.size(); i++)
	{
		bool isFind = false;
		for (int j = 0; j < vecColorCount.size(); j++)
		{
			if (vecColorCount[j].first[0] == vecColor[i][0] &&
				vecColorCount[j].first[1] == vecColor[i][1] &&
				vecColorCount[j].first[2] == vecColor[i][2])
			{
				vecColorCount[j].second += 1;
				isFind = true;
				break;
			}
		}
		if (isFind != true)
		{
			vecColorCount.push_back(make_pair(vecColor[i], 1));
		}
	}

	if (vecColorCount.size() < 1)	// 없으면 흰색뱉음
		return { 255, 255, 255 };

	Vec3b maxColor = vecColorCount[0].first;
	int maxCount = vecColorCount[0].second;
	for (int i = 0; i < vecColorCount.size(); i++)
	{
		//		printf("Color %d : (%d %d %d), %d \r\n", i, vecColorCount[i].first[0], vecColorCount[i].first[1], vecColorCount[i].first[2], vecColorCount[i].second);
		if (vecColorCount[i].second > maxCount)
		{
			maxCount = vecColorCount[i].second;
			maxColor = vecColorCount[i].first;
		}
	}

	return maxColor;
}

Mat imageHandler::stackBinImage(Mat stackBinImage, Mat patternImage)
{
	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_stack = stackBinImage.ptr<uchar>(y);
		uchar* yPtr_pattern = patternImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			// 누적법
			if (yPtr_pattern[x] == 0)	// 0인곳  On 조건
			{
				yPtr_stack[x] = 0;
			}
			else
			{
				yPtr_stack[x] += 1;
			}

		}
	}
	return stackBinImage;
}

// 모든 좌표의 값들의 합
uint imageHandler::getSumOfBinImageValues(Mat stackBinImage)
{
	uint sum = 0;

	int height = stackBinImage.rows;
	int width = stackBinImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr_stack = stackBinImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			sum += yPtr_stack[x];
		}
	}

	return sum;
}

uint imageHandler::getMaximumValue(Mat binImage)
{
	uint max = 0;

	int height = binImage.rows;
	int width = binImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr[x] > max)
				max = yPtr[x];
		}
	}

	return max;
}

uint imageHandler::getMinimumValue(Mat binImage)
{
	uint min = 255;

	int height = binImage.rows;
	int width = binImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			if (yPtr[x] != 0)// 0 제외
			{
				if (yPtr[x] < min)
					min = yPtr[x];
			}
		}
	}

	return min;
}

vector<int> imageHandler::getValueArrWithSort(Mat binImage)
{
	vector<int> values;

	int height = binImage.rows;
	int width = binImage.cols;

	for (int y = 0; y < height; y++)
	{
		uchar* yPtr = binImage.ptr<uchar>(y);

		for (int x = 0; x < width; x++)
		{
			// 누적법
			if (yPtr[x] != 0)	// 0인곳  On 조건
			{
				values.push_back(yPtr[x]);
			}
		}
	}

	sort(values.begin(), values.end());
	return values;
}
WeightMat::WeightMat()
{
	//this->binImage = image.clone();
	//this->frameNum = frameNum;
}

WeightMat::WeightMat(Mat image, int frameNum)
{
	this->binImage = image.clone();
	this->frameNum = frameNum;
}

contourLineInfo::contourLineInfo()
{
	;
}

int contourLineInfo::getMaxValue()
{
	int max = 0;
	for (int i = 0; i < contours.size(); i++)
	{
		int val = contours[i].getMaxValue();
		if (max < val)
			max = val;
	}
	return max;
	return 0;
}

contourLineInfo contourLineInfo::getLineinfoFrombinMat(Mat binWeightMat)
{
	return contourLineInfo();
}


Mat imageHandler::getPatternFillImage(Mat rgbImage, Scalar targetColor)
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

Mat imageHandler::getPatternFillImage_2(Mat rgbImage, Scalar targetColor)
{
	Mat FC_Bin = getFillImage(rgbImage, targetColor);
	inRange(FC_Bin, Scalar(254, 254, 254), Scalar(255, 255, 255), FC_Bin);	// to 1 demend
	Mat PatternFullfill;
	PatternFullfill = imageHandler::getBorderFloodFilledImage(FC_Bin);
	Mat erodeImage_Denoise = imageHandler::removeNotLyricwhiteArea(PatternFullfill);	// 사각박스있는곳 제거
	//erodeImage_Denoise = imageHandler::getMorphImage(erodeImage_Denoise, MORPH_ERODE);	// 침식연산
	return erodeImage_Denoise;

	// return FC_Bin;
}

Mat imageHandler::getFillImage(Mat rgbImage, Scalar targetColor)
{
	//Mat fullyContrastImage = imageHandler::getFullyContrastImage(rgbImage);
	//Mat fullyContrastImage_pix = imageHandler::getPixelContrastImage(rgbImage);
	Mat fullyContrastImage_per = imageHandler::getPixelContrastImage_byPercent(rgbImage);
	//Mat fullyContrastImage = imageHandler::getPixelContrastImage(rgbImage);
	//imshow("fullyContrastImage_per", fullyContrastImage_per);
	//imshow("fullyContrastImage", fullyContrastImage);
	Mat FC_Bin;
	Scalar patternMin = targetColor;
	for (int i = 0; i < 3; i++)
		if (patternMin[i] != 0)
			patternMin[i] = patternMin[i] - 1;

	inRange(fullyContrastImage_per, patternMin, targetColor, FC_Bin);	// 파랑만이미지
	cvtColor(FC_Bin, FC_Bin, COLOR_GRAY2BGR);

	return FC_Bin;
}


Mat imageHandler::getFillImage_unPrint(Mat rgbImage, Scalar targetColor)
{
	Mat fullyContrastImage = imageHandler::getPixelContrastImage(rgbImage);
	//imshow("fullyContrastImage_per", fullyContrastImage_per);
	//imshow("fullyContrastImage", fullyContrastImage);
	Mat FC_Bin;
	Scalar patternMin = targetColor;
	for (int i = 0; i < 3; i++)
		if (patternMin[i] != 0)
			patternMin[i] = patternMin[i] - 1;

	inRange(fullyContrastImage, patternMin, targetColor, FC_Bin);	// 파랑만이미지
	cvtColor(FC_Bin, FC_Bin, COLOR_GRAY2BGR);

	return FC_Bin;
}

int contourInfo::getMaxValue()
{
	int max = 0;
	for (int i = 0; i < includeValues.size(); i++)
	{
		if (max < includeValues[i])
			max = includeValues[i];
	}
	return max;
}

vector<int> contourInfo::includeValuesDeduplicate(vector<int> includeValues)
{
	sort(includeValues.begin(), includeValues.end(), greater<int>());
	includeValues.erase(
		unique(includeValues.begin(), includeValues.end(),
			[](const int& a, const int& b) {
		if (a == b)
			return true;
		else
			return false;
	}), includeValues.end());

	return includeValues;
}

vector<contourInfo> contourInfo::getContourInfosFromBinImage(Mat binImage, Mat &outImage)	// m_unPrintWeight 이미지도 사용해서 뽑자..
{
	outImage = Mat::zeros(binImage.rows, binImage.cols, CV_8U);
	vector<contourInfo> outInfos;
	Mat binTempImage;
	inRange(binImage, 1, 255, binTempImage);
	binTempImage = imageHandler::getDustRemovedImage(binTempImage);	// 부피가 3보다 작은것 삭제함

	/*
		unp이미지 사용
		unp
	*/

	vector<vector<Point>> contours;
	findContours(binTempImage, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		Mat contourMask = Mat::zeros(binImage.rows, binImage.cols, CV_8U);;
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
			uchar* yPtr = binImage.ptr<uchar>(indices[idx].y);	//in
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

		contourInfo conInfo = contourInfo:: getContourInfoFromPixels(indices);
		outInfos.push_back(conInfo);
	}
	sort(outInfos.begin(), outInfos.end(), imageHandler::asc_contourInfo);

	return outInfos;
}

contourInfo contourInfo::getContourInfoFromPixels(vector<Point> pixels)
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

contourLineInfoSet::contourLineInfoSet(contourLineInfo InitLineInfo)
{
	this->progress = InitLineInfo;
	this->maximum = InitLineInfo;
}

// patternFill 에서 뎁스가 1이상인 곳 삭제 (O 안에 노이즈가 있을 때 문제 발생가능.. )
Mat imageHandler::getDepthContourRemovedMat(Mat binImage)
{
	Mat removedMat = binImage.clone();
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;	// [Next, Privious, First Child, Parent] -> Parent있는에들 삭제하면됨

	findContours(binImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);
	// 뎁스가 0이 아닌 에들은 floodfill 해버림

	vector<int> firstGeneration;
	vector<int> secondGeneration;
	vector<int> firstSecondGeneration;
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		if (hierarchy[i][3] == -1)
			firstGeneration.push_back(i);
	}
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		for (int j = 0; j < firstGeneration.size(); j++)
		{
			if (hierarchy[i][3] == firstGeneration[j])// firstGen 배열에 있는 애가 부모인 경우
			{
				secondGeneration.push_back(i);
				break;	
			}
		}
	}
	// first, second 부모의 합
	firstSecondGeneration = firstGeneration;	
	for (int i = 0 ; i < secondGeneration.size(); i++)
	{
		firstSecondGeneration.push_back(secondGeneration[i]);
	}

	// 부모가 firstGen, secondGen 둘다 아닌경우 3세대 이상으로 보고 삭제함.
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		bool isFSGeneration = false;
		for (int j = 0; j < firstSecondGeneration.size(); j++)
		{
			if (i == firstSecondGeneration[j])	// 1,2 세대임
			{
				isFSGeneration = true;
				break;
			}
		}
		
		if (isFSGeneration == false)
		{
			Mat contourMask = Mat::zeros(binImage.rows, binImage.cols, CV_8U);
			vector<vector<Point>> contours_picked;
			contours_picked.push_back(contours[i]);
			fillPoly(contourMask, contours_picked, 255);

			bitwise_not(contourMask, contourMask);
			bitwise_and(contourMask, removedMat, removedMat);
		}

	}


	//for (unsigned int i = 0; i < contours.size(); i++)
	//{
	//	if (hierarchy[i][3] != -1) // 부모가 있는 애들 && 
	//	{
	//		Mat contourMask = Mat::zeros(binImage.rows, binImage.cols, CV_8U);;
	//		vector<vector<Point>> contours_picked;
	//		contours_picked.push_back(contours[i]);
	//		fillPoly(contourMask, contours_picked, 255);

	//		bitwise_not(contourMask, contourMask);
	//		bitwise_and(contourMask, removedMat, removedMat);
	//	}
	//}
	return removedMat;
}

int imageHandler::getAvgContourVolume(Mat binMImage)
{
	inRange(binMImage, 1, 255, binMImage);	// binarize 
	binMImage = imageHandler::getDustRemovedImage(binMImage);

	vector<vector<Point>> contours;
	vector<int> contoursSize;
	vector<Vec4i> hierarchy;
	findContours(binMImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);

	for (unsigned int i = 0; i < contours.size(); i++)
	{
		if (hierarchy[i][3] == -1) // 부모가 없는 에들 (첫 계층애들)
		{
			contoursSize.push_back(contours[i].size());
		}
	}
	sort(contoursSize.begin(), contoursSize.end(), greater<int>());

	if (contoursSize.size() < 2)	// for error: divide zero
		return 0;

	int sum = 0;
	for (int i = 0; i < contoursSize.size() / 2; i++) // 상위 절반만 측정함
	{
		sum += contoursSize[i];
	}

	int avg = sum / (contoursSize.size() / 2);
	return avg;
}
