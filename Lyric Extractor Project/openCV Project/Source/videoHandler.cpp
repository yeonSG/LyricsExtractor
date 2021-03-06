#include "videoHandler.h"
#include "loger.h"

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

	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "***Video SPEC***";
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Video fps		: " << getVideoFPS();
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Video frameCount	: " << getVideoFrameCount();
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Video frameWidth	: " << getVideoWidth();
	BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "Video frameHeight	: " << getVideoHeight();
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
int videoHandler::frameToMs(int frame, VideoCapture& vc)
{
	if (!vc.isOpened())
	{
		printf("video is not opened\r\n");
		return 0;
	}
	vc.set(CAP_PROP_POS_FRAMES, (double)frame);
	Mat startImage;
	vc.read(startImage);
	double sourceMsec = vc.get(CAP_PROP_POS_MSEC);
	
	return sourceMsec;
}

String videoHandler::frameToTime(int frame, VideoCapture& vc)
{
	if (!vc.isOpened())
	{
		printf("video is not opened\r\n");
		return "";
	}
	
	int sourceMsec = frameToMs(frame, vc);
	
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
		return false;

	boost::filesystem::path p(videoPath);
	fileManager::videoName = p.filename().string();

	return true;
}

void videoHandler::closeVideo()
{
	if (videoCapture == nullptr)
		return;
	else if (videoCapture->isOpened())
		videoCapture->release();

	delete videoCapture;
	videoCapture = nullptr;
}
