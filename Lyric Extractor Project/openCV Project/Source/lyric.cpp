#include "lyric.h"

void lyric::addLine(Line line)
{
	lines.push_back(line);
}

Line* lyric::getLine(int index)
{
	if (index < getLinesSize())
		return &lines.at(index);
	else 
		return nullptr;
}

int lyric::getLinesSize()
{
	return lines.size();
}

void lyric::init()
{
	if(!lines.empty())
		lines.clear();
}

void lyric::removeInvalidLines()
{
	int lineCount = 0;
	Mat maskImage;
	for (vector<Line>::iterator it = lines.begin(); it != lines.end(); /*it++*/)
	{
		printf("Line%d : %d - %d\r\n", lineCount, it->startFrame, it->endFrame);
		if (it->isValid == false)
		{
			printf("Line%d remove(line Invalid).\r\n\r\n", lineCount);
			it = lines.erase(it);
			lineCount++;
			continue;
		}
		++it;
		lineCount++;
	}
}
