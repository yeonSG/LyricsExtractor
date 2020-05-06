#pragma once

#include <Windows.h>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "defines.h"
#include "videoHandler.h"
#include "fileManager.h"
#include "imageHandler.h"
#include <boost/filesystem/path.hpp>
#include "Line.h"
#include "lyric.h"
#include "Json.h"
#include "MVInformation.h"
#include "LineInfoFinder.h"

using json = nlohmann::json;

using namespace std;

class OCRHandler
{
public:

public:

    void runOCR(string targetImage, string outFileName);
    wstring stringToWstring(const std::string& s);
};