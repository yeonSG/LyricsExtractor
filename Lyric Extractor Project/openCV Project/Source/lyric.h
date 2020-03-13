#pragma once

#include "videoAnalyzer.h"
#include "Line.h"


using namespace std;

class lyric
{
public:
	void setLines(vector<Line> lines);
	void addLine(Line line);
	Line* getLine(int index);
	int getLinesSize();
	void init();

	void cleanupInvalidLines();
	
	void writeLyricFile(VideoCapture* videoCapture);
	void writeLyric_withWordFile(VideoCapture* videoCapture);

private:
	vector<Line> lines;
	vector<int> vecWhiteCount;
};