#include "Line.h"

/// <summary>
/// Line.text�� ������ words�� �ִ´�. �����ڴ� " "
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
/// Line.maskImage���� ���� ������ �Ǻ��س�
/// </summary>
void Line::getSpacingWordsFromMaskImage()
{
	vector<int> vecProjectionData =  imageHandler::getVerticalProjectionData(maskImage);
	Mat mask = maskImage;	// for debug
	int spacePixelCount = 0;
	int spacePixelStartPoint = 0;
	bool started = false;

	// ���� -> ���� : 
	// ���� -> ����
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
			if (spacePixelCount >= 13)	// �����̽��� �Ǵ��� �� �Ȼ��� �� 10������ �ִ� �κ��� Separation���� ��
			{
				int spacingWordXCoodination = (spacePixelCount / 2) + spacePixelStartPoint;	// �߰���
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
/// Line ��ä�� word ��ä�� ����
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

