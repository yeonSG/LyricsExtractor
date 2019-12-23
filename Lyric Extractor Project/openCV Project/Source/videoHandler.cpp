#include "videoHandler.h"

void videoHandler::printVideoSpec(VideoCapture vc)
{
	if (!vc.isOpened())
	{
		printf("video is not opened\r\n");
		return;
	}
	double fps = vc.get(CAP_PROP_FPS);
	int frameCount = vc.get(CAP_PROP_FRAME_COUNT);
	int frameWidth = vc.get(CAP_PROP_FRAME_WIDTH);
	int frameHeight = vc.get(CAP_PROP_FRAME_HEIGHT);

	printf("Video fps			: %f \r\n", fps);
	printf("Video frameCount	: %d \r\n", frameCount);
	printf("Video frameWidth	: %d \r\n", frameWidth);
	printf("Video frameHeight	: %d \r\n", frameHeight);
	printf("Video expected length : %d \r\n", frameCount);
}

void videoHandler::printCurrentFrameSpec(VideoCapture vc)
{
	if (!vc.isOpened())
	{
		printf("video is not opened\r\n");
		return;
	}
	double curMsec = vc.get(CAP_PROP_POS_MSEC);
	int curSec = (int)(curMsec / 1000);
	int curMin = curSec / 60;
	int curFrame = (int)vc.get(CAP_PROP_POS_FRAMES);

	printf("current time(msec)	: %02d:%02d.%03d(%.0f) \r\n", curMin, curSec%60, ((int)curMsec) % 1000, curMsec);
	printf("current frame	: %.0d \r\n", curFrame);


	char buff[100];
	snprintf(buff, sizeof(buff), "%02d:%02d.%03d", curMin, curSec % 60, ((int)curMsec) % 1000);
	String buffasStrt = buff;
}

String videoHandler::frameToTime(int frame, VideoCapture& vc)
{
	if (!vc.isOpened())
	{
		printf("video is not opened\r\n");
		return "";
	}
	vc.set(CAP_PROP_POS_FRAMES, (double)frame);
	Mat startImage;
	vc.read(startImage);
	double sourceMsec = vc.get(CAP_PROP_POS_MSEC);
	
	int curMsec = (int)sourceMsec % 1000;
	int curSec = ((int)sourceMsec/1000) % 60;
	int curMin = ((int)sourceMsec/1000) /60;

	char buff[100];
	snprintf(buff, sizeof(buff), "%02d:%02d.%03d", curMin, curSec, curMsec);
	String buffAsString = buff;

	return buffAsString;
}
