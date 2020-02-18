#include "videoHandler.h"

VideoCapture* videoHandler::videoCapture = nullptr;

double videoHandler::getVideoFPS()
{
	if (videoCapture->isOpened())
	{
		double fps = videoCapture->get(CAP_PROP_FPS);
		return fps;
	}
	else
		return 0;
}

int videoHandler::getVideoFrameCount()
{
	if (videoCapture->isOpened())
	{
		double fps = videoCapture->get(CAP_PROP_FRAME_COUNT);
		return fps;
	}
	else
		return 0;
}

int videoHandler::getVideoWidth()
{
	if (videoCapture->isOpened())
	{
		double fps = videoCapture->get(CAP_PROP_FRAME_WIDTH);
		return fps;
	}
	else
		return 0;
}

int videoHandler::getVideoHeight()
{
	if (videoCapture->isOpened())
	{
		double fps = videoCapture->get(CAP_PROP_FRAME_HEIGHT);
		return fps;
	}
	else
		return 0;
}

void videoHandler::printVideoSpec()
{
	printf("Video fps			: %f \r\n", getVideoFPS());
	printf("Video frameCount	: %d \r\n", getVideoFrameCount());
	printf("Video frameWidth	: %d \r\n", getVideoWidth());
	printf("Video frameHeight	: %d \r\n", getVideoHeight());
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

VideoCapture* videoHandler::getVideoCapture()
{
	if (videoCapture == nullptr)
	{
		printf("videoCapture is Null\r\n");
		return nullptr;
	}
	else if(!videoCapture->isOpened())
	{
		printf("videoCapture is notOpened\r\n");
		return nullptr;
	}
	else if (videoCapture->isOpened())
		return videoCapture;
}

bool videoHandler::setVideo(string videoPath)
{
	videoCapture = new VideoCapture(videoPath);
	if (!videoCapture->isOpened())
	{
		cout << "fail to open the video" << endl;
		return false;
	}
	boost::filesystem::path p(videoPath);
	fileManager::videoName = p.filename().string();

	printVideoSpec();

	return true;
}

void videoHandler::closeVideo()
{
	if (videoCapture == nullptr)
		;
	else if (videoCapture->isOpened())
		videoCapture->release();

	delete videoCapture;
}
