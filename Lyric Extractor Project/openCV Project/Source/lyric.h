#pragma once

#include "videoAnalyzer.h"
#include "Line.h"


using namespace std;

class Lyric
{
public:
	void setLines(vector<Line> lines);
	void addLine(Line line);
	Line* getLine(int index);
	int getLinesSize();
	void init();

	void cleanupInvalidLines();

	void getTimeDataFromframeNum(VideoCapture* videoCapture);
	
	void writeLyricFile(VideoCapture* videoCapture);
	void writeLyric_withWordFile(VideoCapture* videoCapture);

	void sortingLine();

	void saveBinaryImage(string videoPath);		// catpureBinaryImageOfLinesEnd()

private:
	vector<Line> lines;
	vector<int> vecWhiteCount;
};