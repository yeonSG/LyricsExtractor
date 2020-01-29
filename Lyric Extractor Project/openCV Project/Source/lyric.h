#pragma once

#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "line.h"

using namespace std;

class lyric
{
public:
	void addLine(Line line);
	Line* getLine(int index);
	int getLinesSize();
	void init();

	void removeInvalidLines();

	void writeLyricFile();

private:
	vector<Line> lines;
	;
};