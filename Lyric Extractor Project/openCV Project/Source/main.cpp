#include <Windows.h>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <direct.h>
#include <zlib.h>
#include "testClass.h"
#include "analyzer.h"
#include "fileManager.h"
#include "loger.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/log/trivial.hpp>

using namespace std;
using namespace cv;


int main(int argc, char* argv[])
{
#ifndef _DEBUG
	if (argc != 2)
	{
		std::cout << "usage: " << argv[0] << " [video file path or directory path].";
		return 0;
	}

	boost::filesystem::path path(argv[1]);
	// fileManager::videoName = p.filename().string();
	if (!boost::filesystem::exists(path))
	{
		cout << path << " is not exists.";
		return 0;
	}
	if (boost::filesystem::is_regular_file(path))
	{
		analyzer ana;
		ana.startVideoAnalization(path.string());

	}
	else if (boost::filesystem::is_directory(path))
	{
		// do all files
		BOOST_FOREACH(const boost::filesystem::path & p, std::make_pair(boost::filesystem::directory_iterator(path), boost::filesystem::directory_iterator()))
		{
			//if (!fs::is_directory(p))
			//{
			//	std::cout << p.filename() << std::endl;
			//}
			analyzer ana;
			ana.startVideoAnalization( path.string()+"\\"+p.filename().string() );
		}
	}

#else
	/*
	vector<string> fname;
	fname.push_back("15062_cap.png");
	fname.push_back("41875.mpg_2600.png");
	fname.push_back("47795.mpeg_2500.png");
	fname.push_back("49686.mpg_2500.png");
	fname.push_back("53772.mpg_2600.png");
	fname.push_back("baboon.png");
	fname.push_back("butterfly.png");
	fname.push_back("comic.png");
	//string fname = "15062_cap.png";
	for (int i = 0; i < fname.size(); i++)
	{
		Mat img = imread("¿øº»\\" + fname[i]);
		resize(img, img, cv::Size(img.cols * 4, img.rows * 4), 0, 0, cv::INTER_CUBIC);
		imwrite(fname[i], img);
	}
	*/
	//system("python deblur.py debug_2_upscale.png");
	testClass testClass;
	//testClass.test_Video_captureFrame("53772_out.mp4");
	//testClass.test_Video_captureFrame("47795.mpeg");
	//testClass.test_Video_captureFrame("42041.mpg");
	//testClass.test_Video_captureFrame("41875.mpg");
	//testClass.test_Video_captureFrame("49686.mpg");
	//testClass.test_Video_captureFrame("53772.mpg");

	//testClass.test_Image();
	//testClass.test_Image2();
	//testClass.test_Image4();
	//testClass.test_Video3();
	//testClass.test_Video_GetContourMask2("40009.mp4");

	testClass.test_Video("movie1.mp4");

	//testClass.test_Video4("40009.mp4");
	//testClass.test_Video4("40011_forTest.mp4");
	//testClass.test_Video4("40011.mp4");
	//testClass.test_Video4("movie1_forTest.mp4");
	//testClass.test_Video4("movie1_forTest2.mp4");
	//testClass.test_Video4("movie.mp4");
	//testClass.test_Video4("movie1.mp4");

	analyzer ana;
	//ana.videoAnalization1("movie1.mp4");
	//ana.videoAnalization1("40009.mp4");

	//ana.startVideoAnalization("40009_forDebug.mp4");

	ana.startVideoAnalization("40009_forDebug.mp4");	// *shortist test movie
	//ana.startVideoAnalization("movie1_forTest2.mp4");
	//ana.startVideoAnalization("movie1_forTest2.mp4");
	//ana.startVideoAnalization("40011_forTest.mp4");
	//ana.startVideoAnalization("40011_forTest_upscale.mp4");

	//ana.startVideoAnalization("movie.mp4");
	//ana.startVideoAnalization("movie1.mp4");
	//ana.startVideoAnalization("40009.mp4");
	//ana.startVideoAnalization("40011.mp4");
	//ana.startVideoAnalization("40006.mp4");
	//ana.startVideoAnalization("40003.mp4");
	//	  
	//ana.startVideoAnalization("gramy_vol8_4.mp4");
	//ana.startVideoAnalization("gramy_vol8_6.mp4");
	

	
	//BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "nProcess Successed : " <<(clock() - startClock) / CLOCKS_PER_SEC <<"Sec";
	//printf("\r\nProcess Successed : %0.1fSec\r\n", (float)(clock() - startClock) / CLOCKS_PER_SEC);

#endif


	return 1;
}