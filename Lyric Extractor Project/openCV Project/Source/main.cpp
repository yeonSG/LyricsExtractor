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
#include "Json.h"

using namespace std;
using namespace cv;

static void
usage(const char* argv0) {
	fprintf(stderr, "Usage: %s [options] <input filenames | foldernames...>\n", argv0);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -r --re-analyzation <\"true\" | \"false\">: 이미 분석한 것이 있다면 다시 분석함 \"Lyric.json\"파일 유무로 확인. (Default: false)\n");
}

int main(int argc, char* argv[])
{
	bool reAnalyzation = false;
	int i;

#ifndef _DEBUG
	for (i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-h") == 0 ||
			strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return 0;
			continue;
		}

		if (strcmp(argv[i], "-r") == 0 ||
			strcmp(argv[i], "--re-analyzation") == 0) {
			i++;
			if (argc == i)
			{
				usage(argv[0]);
				return 0;
			}

			if (strcmp(argv[i], "true") == 0)
			{
				reAnalyzation = true;
				continue;
			}
			else if (strcmp(argv[i], "false") == 0)
			{
				reAnalyzation = false;
				continue;
			}
			usage(argv[0]);
			return 0;
		}

		break;
	}

	if (i == argc) {    // 옵션들은 다 입력됬는데, 파일명이 입력안된경우
		fprintf(stderr, "No filename given\n\n");
		usage(argv[0]);
		return 0;
	}

	while (i < argc) {
		boost::filesystem::path path(argv[i]);
		// fileManager::videoName = p.filename().string();
		if (!boost::filesystem::exists(path))
		{
			cout << path << " input file is not exists." << endl;
			i++;
			continue;//return 0;
		}

		if (boost::filesystem::is_regular_file(path))
		{
			printf("is regular file. \r\n");
			if (reAnalyzation == false)	// 
			{
				// is regular file. dbug ".\Output\MV_karaoke_20200416\\movie1.mp4\Lyric.json"
				boost::filesystem::path jsonPath = ".\\Output\\" + path.filename().string() + "\\" + "Lyric.json";

				if (boost::filesystem::exists(jsonPath) == true)	// path.json 파일이 있다면 작업 하지 않음
				{
					cout << path << " is already analyzed." << endl;
					i++;
					continue;
				}
			}
			analyzer ana;
			ana.startVideoAnalization(path.string());

		}
		else if (boost::filesystem::is_directory(path))
		{
			printf("is_directory. \r\n");
			// do all files
			BOOST_FOREACH(const boost::filesystem::path & p, std::make_pair(boost::filesystem::directory_iterator(path), boost::filesystem::directory_iterator()))
			{
				//if (!fs::is_directory(p))
				//{
				//	std::cout << p.filename() << std::endl;
				//}

				if (reAnalyzation == false)	// 
				{
					// is regular file. dbug ".\Output\MV_karaoke_20200416\\movie1.mp4\Lyric.json"
					boost::filesystem::path jsonPath = ".\\Output\\" + p.filename().string() + "\\" + "Lyric.json";

					if (boost::filesystem::exists(jsonPath) == true)	// path.json 파일이 있다면 작업 하지 않음
					{
						cout << p.filename().string() << " is already analyzed." << endl;
						i++;
						continue;
					}
				}

				analyzer ana;
				ana.startVideoAnalization(path.string() + "\\" + p.filename().string());
			}
		}

		i++;
		continue;
	}

	// @@ 버그 : 파일쓰기 맛탱이감
	/*	TODO Progress
		0. 케이스 따내는 루틴 생성 : 
			0.0. 색칠전색 - 색칠후색  (+Duet일때 2가지 패턴) 
			0.1. 2라인 or 1라인
			0.2. 시작 전 카운팅 심볼

		1. 라인 수 만큼 Peak 찾기 
		
	*/
	/* 케이스 따내는 루틴
	 방법 : 색칠되는 부분을 따냄 ()
	
	*/
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
		Mat img = imread("원본\\" + fname[i]);
		resize(img, img, cv::Size(img.cols * 4, img.rows * 4), 0, 0, cv::INTER_CUBIC);
		imwrite(fname[i], img);
	}
	*/
	//system("python deblur.py debug_2_upscale.png");
	testClass testClass;
	//testClass.test_Image4();
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

	testClass.test_Video("Input\\51815_0~35Lines_Duet_StartGray.mp4");
	//testClass.test_Video("40003.mp4");

	//testClass.test_Video4("40009.mp4");
	//testClass.test_Video4("40011_forTest.mp4");
	//testClass.test_Video4("40011.mp4");
	//testClass.test_Video4("movie1_forTest.mp4");
	//testClass.test_Video4("movie1_forTest2.mp4");
	//testClass.test_Video4("movie.mp4");
	//testClass.test_whiteCountImage("gramy_vol8_4.mp4");
	//testClass.test_whiteCountImage("movie1.mp4");
	//testClass.test_whiteCountImage("51781_0~73Lines_Duet_ColorWeired.mp4");	// 하늘색 P
	//testClass.test_whiteCountImage("52704_0~39Lines_Duet_StartGray.mp4");	

	//testClass.test_getTypeRoutin("51781_0~73Lines_Duet_ColorWeired.mp4");	// 하늘색 P
	//testClass.test_getTypeRoutin2("54326_0~15Lines_TwinLine_Unprint색다름.mp4");	// 오랜지색 UP
	//testClass.test_getTypeRoutin2("movie1.mp4");	
	
	//testClass.test_getTypeRoutin2("52704_0~39Lines_Duet_StartGray.mp4");
	//testClass.test_getTypeRoutin2("movie.mp4");
	//testClass.test_getTypeRoutin2("50123_0~31Lines_TwinLine_BadQuality.mp4");
	
	

	analyzer ana;
	ana.startVideoAnalization("Input\\53338_0~27Lines.mp4");		// 0~62
	
	//BOOST_LOG_SEV(my_logger::get(), severity_level::normal) << "nProcess Successed : " <<(clock() - startClock) / CLOCKS_PER_SEC <<"Sec";
	//printf("\r\nProcess Successed : %0.1fSec\r\n", (float)(clock() - startClock) / CLOCKS_PER_SEC);

#endif


	return 1;
}

// 머지 알고리즘 손볼것 (51781_0~73Lines_Duet_ColorWeire)
// 퍼플 노이즈 처리 알고리즘 추가 (53337_0~29Lines.mp4)