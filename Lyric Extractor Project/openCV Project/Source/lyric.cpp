#include "Lyric.h"
#include "loger.h"

void Lyric::setLines(vector<Line> lines)
{
	this->lines = lines;
}

void Lyric::addLine(Line line)
{
	lines.push_back(line);
}

Line* Lyric::getLine(int index)
{
	if (index < getLinesSize())
		return &lines.at(index);
	else 
		return nullptr;
}

int Lyric::getLinesSize()
{
	return lines.size();
}

void Lyric::init()
{
	if(!lines.empty())
		lines.clear();
}

void Lyric::cleanupInvalidLines()
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "cleanupInvalidLines()";
	int lineCount = 0;
	Mat maskImage;
	for (vector<Line>::iterator it = lines.begin(); it != lines.end(); /*it++*/)
	{
		printf("Line%d : %d - %d\r\n", lineCount, it->startFrame, it->endFrame);
		BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line" << lineCount << " : " << it->startFrame <<"-"<< it->endFrame;
		if (it->isValid == false)
		{
			printf("Line%d remove(line Invalid).\r\n\r\n", lineCount);
			BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Line" << lineCount << "remove(line Invalid).";
			it = lines.erase(it);
			lineCount++;
			continue;
		}
		++it;
		lineCount++;
	}
}

void Lyric::getTimeDataFromframeNum(VideoCapture* videoCapture)
{
	for (int i = 0; i < getLinesSize(); i++)
	{
		Line* line = getLine(i);
		line->startFrame_ms = videoHandler::frameToMs(line->startFrame, *videoCapture);
		line->endFrame_ms = videoHandler::frameToMs(line->endFrame, *videoCapture);
		
		for (int j = 0; j < line->words.size(); j++)
		{
			line->words[j].startFrame_ms = videoHandler::frameToMs(line->words[j].startFrame, *videoCapture);
			line->words[j].endFrame_ms = videoHandler::frameToMs(line->words[j].endFrame, *videoCapture);			
		}
	}
}

void Lyric::writeLyricFile(VideoCapture* videoCapture)
{
	vector<string> vecLyricLine;
	vector<string> vecLyricLine_debug;
	for (int i = 0; i < getLinesSize(); i++)
	{
		Line* line = getLine(i);

		String startTime = videoHandler::frameToTime(line->startFrame, *videoCapture);
		String endTime = videoHandler::frameToTime(line->endFrame, *videoCapture);

		string lyricLine = "[" + startTime + "]\t" + line->text + "\t[" + endTime + "]";

		vecLyricLine.push_back(lyricLine);

		startTime = to_string(line->startFrame);
		endTime = to_string(line->endFrame);
		lyricLine = "[" + startTime + "]\t" + line->text + "\t[" + endTime + "]";
		vecLyricLine_debug.push_back(lyricLine);
	}

	string filename = "Lyrics.txt";
	fileManager::writeVector(filename, vecLyricLine);

	filename = "Lyrics_debug.txt";
	fileManager::writeVector(filename, vecLyricLine_debug);
}

void Lyric::writeLyric_withWordFile(VideoCapture* videoCapture)
{
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "save _withWord file (Lines:"<< getLinesSize() << ")";
	printf("save _withWord file (%dLines)\r\n", getLinesSize());

	vector<string> vecLyricLine;
	vector<string> vecLyricLine_debug;
	for (int i = 0; i < getLinesSize(); i++)
	{
		Line* line = getLine(i);
		for (int j = 0; j < line->words.size(); j++)
		{
			String startTime = to_string(line->words[j].startFrame);
			String endTime = to_string(line->words[j].endFrame);
			string stringLine = to_string(i) + "\t" + to_string(j) + "\t[" + startTime + "]\t[" + endTime + "]\t" + line->words[j].text;
			vecLyricLine_debug.push_back(stringLine);

			startTime = videoHandler::frameToTime(line->words[j].startFrame, *videoCapture);
			endTime = videoHandler::frameToTime(line->words[j].endFrame, *videoCapture);
			stringLine = to_string(i) + "\t" + to_string(j) + "\t[" + startTime + "]\t[" + endTime + "]\t" + line->words[j].text;
			vecLyricLine.push_back(stringLine);
		}
	}
	string filename = "Lyrics_withWord_debug.txt";
	fileManager::writeVector(filename, vecLyricLine_debug);

	filename = "Lyrics_withWord.txt";
	fileManager::writeVector(filename, vecLyricLine);
}
