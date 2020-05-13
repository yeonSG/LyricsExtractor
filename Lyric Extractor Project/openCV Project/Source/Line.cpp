#include "Line.h"

/// <summary>
/// Line.text의 내용을 words에 넣는다. 구분자는 " "
/// </summary>
void Line::splitLineTextToWord()
{
	vector<string> tokens;

	boost::split(tokens, text, boost::is_any_of(" "));
	if (tokens.size() != words.size())
		printf("Not same : OCR_tokens.size(), words.size() ,%d ,%d\r\n", tokens.size(), words.size());
	for (int i = 0; i < words.size(); i++)
	{
		if (tokens.size() > i)
		{
			words[i].text = tokens[i];
		}
	}

}

/// <summary>
/// Line.maskImage에서 띄어쓰기 갯수를 판별해냄
/// </summary>
void Line::getSpacingWordsFromMaskImage()
{
	vector<int> vecProjectionData =  imageHandler::getVerticalProjectionData(maskImage);
	Mat mask = maskImage;	// for debug
	int spacePixelCount = 0;
	int spacePixelStartPoint = 0;
	bool started = false;

	// 흰점 -> 검점 : 
	// 검점 -> 흰점
	for (int i = 0; i < vecProjectionData.size(); i++)
	{		
		if (started == false)
		{
			if (vecProjectionData[i] != 0)
				started = true;
			else
				continue;
		}

		if (vecProjectionData[i] != 0)
		{
			if (spacePixelCount >= 13)	// 스페이스로 판단함 빈 픽샐이 약 10개정도 있는 부분을 Separation으로 함
			{
				int spacingWordXCoodination = (spacePixelCount / 2) + spacePixelStartPoint;	// 중간점
				spacingWords.push_back(spacingWordXCoodination);
			}
			spacePixelCount = 0;
			spacePixelStartPoint = 0;
		}
		else
		{
			spacePixelCount++;
			if (spacePixelStartPoint == 0)
				spacePixelStartPoint = i;
		}
	}

}

/// <summary>
/// Line 객채로 word 객채를 생성
/// </summary>
/// <param name="line">The line.</param>
/// <returns></returns>
Word Line::lineToWord(Line line)
{
	Word word;
	word.startFrame = line.startFrame;
	word.endFrame = line.endFrame;
	word.text = line.text;

	return word;
}

bool Line::desc_Line(Line a, Line b)
{
	return a.endFrame > b.endFrame;
}

bool Line::asc_Line(Line a, Line b)
{
	return a.endFrame < b.endFrame;
}

