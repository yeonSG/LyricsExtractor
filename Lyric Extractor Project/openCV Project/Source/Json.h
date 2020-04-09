#pragma once

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include "lyric.h"
#include "fileManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include "videoAnalyzer.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

using namespace std;

 enum class eType_object_name {
    interlude,
    count,
    lyrics
};

 enum class eType_doc_member {
     composer,
     copyright,
     genre,
     key,
     line,
     national,
     optional,
     singer,
     supportvocal,
     title,
     version,
     vocaldelay,
     writer
 };

 // national °ª
#define NATION_KOR "KOR"
#define NATION_JPN "JPN"
#define NATION_USA "USA"
#define NATION_CHI "CHI"
#define NATION_BRA "BRA"
#define NATION_VIE "VIE"
#define NATION_THA "THA"
#define NATION_TUR "TUR"
#define NATION_MMR "MMR"    // ¹Ì¾á¸¶
#define NATION_FRA "FRA"
#define NATION_ESP "ESP"
#define NATION_IND "IND"
#define NATION_PHL "PHL"

 // part °ª
#define PART_NORMAL "N" // ÀÏ¹Ý
#define PART_MALE   "M" // ³²¼º
#define PART_FEMALE "F" // ¿©¼º
#define PART_DUET   "D" // µà¿§
#define PART_CHORUS "C" // ±¸È£
#define PART_SCRIPT "S" // ´ë»ç
#define PART_RAP    "R" // ·¦

#define INTERLUDE_JUDGETIME_MS 10000 //1000 * 10; 10sec


class Json
{
public:
    string m_composer = "";
    string m_copyright = "";
    string m_genre = "";
    string m_key = "";
    string m_national = NATION_THA;
    int m_optionType = 0;
    string m_singer = "";
    bool m_supportvocal = true;
    string m_title = "";
    int m_version = 0;
    int m_vocaldelay = 1500;
    string m_writer = "";

public:
    
    void nlohmannJson1();

    // nlohmannJson
    //void printDoc(static Document& doc);
    void writeDoc(static json& j);

    void makeJson(Lyric lyric);

    json getObject_line(Lyric lyric);
    json getObject_eventObject(eType_object_name evnetType);
    json getObject_eventInterludeObject(int startms);
    json getObject_eventCountObject(int startms);
    json getObject_eventLyricObject(Line line);
    json getObject_linePartArr(Line line);
};