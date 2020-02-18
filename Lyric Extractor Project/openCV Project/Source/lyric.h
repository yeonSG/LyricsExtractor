#pragma once

#include "videoAnalyzer.h"
#include "Line.h"


using namespace std;

class lyric
{
public:
	void addLine(Line line);
	Line* getLine(int index);
	int getLinesSize();
	void init();

	void removeInvalidLines();
	
	void writeLyricFile(VideoCapture* videoCapture);
	void writeLyric_withWordFile(VideoCapture* videoCapture);

private:
	vector<Line> lines;
	vector<int> vecWhiteCount;
};