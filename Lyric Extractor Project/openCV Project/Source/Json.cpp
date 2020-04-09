#include "json.h"

void Json::nlohmannJson1()
{
    using json = nlohmann::json;

    //std::string s = R"({"authority":1,"category":6,"category_id":"0","delete_by":"","delete_time":"","ext":"","ext_link":0,"is_delete":0,"is_file":0,"is_share":0,"lv_creator":"","lv_id":1,"lv_size":0,"lv_time":"","name":"回收站","x_id":6,"x_pid":1})";
    //std::string s = R"({"authority":1, "name":"回收站"})";
    //std::string s = R"({"authority":1, "name":"ฟหก"})";
    //json j = json::parse(s);
    //std::cout << std::setw(2) << j << std::endl;
    //std::ofstream o("pretty.json");
    //o << std::setw(4) << j << std::endl;

    /*
    json j;
    //j["a"] = 1;
    //j["b"] = false;
    //j["c"] = "HEE";
    //j["d"] = L"한글";
    string s = "ฟหก";
    j["e"] = s;
    //s = "中文";
    //j["f"] = s;

    
    std::ofstream o;// ("pretty.json");
    o.open("pretty.json", std::ios::out | std::ios::app);

    std::locale utf8_locale(std::locale(), new utf8cvt<false>);
    o.imbue(utf8_locale);

    std::cout << std::setw(2) << j << std::endl;
    o << std::setw(4) << j << std::endl;
    */

	string lineFileName = "Line.txt";
	string ocredText;
	fileManager::readLine(lineFileName, ocredText);
    cout << "Read text: " << ocredText;


    std::ofstream writeFile;
    writeFile.open("encode.txt");

    json j;
    j["thai"] = ocredText;
    //j["thai"] = "นะมุวิคิ ต้นไม้แห่งความรู้ที่ทุกท่านปลูก";
    //j["thai"] = "한국어";

    string s = j.dump();
    
    char* cc = new char[s.size() + 1];
    //char c[s.size() + 1];
    strcpy_s(cc, s.size()+1, s.c_str());
    
    cout << s.c_str() << endl;

    if (writeFile.is_open())
    {
        writeFile.write(cc, s.size());
    }
    writeFile.close();

}

void Json::writeDoc(json& j)
{
    //string path = fileManager::getSavePath() + "output.json";

    //std::ofstream writeFile;
    //writeFile.open(path);

    //string s = j.dump();
    //char* cc = new char[s.size() + 1];
    ////char c[s.size() + 1];
    //strcpy_s(cc, s.size() + 1, s.c_str());

    //cout << s.c_str() << endl;

    //if (writeFile.is_open())
    //{
    //    writeFile.write(cc, s.size());
    //}
    //writeFile.close();

    string path = fileManager::getSavePath() + "output.json";
    std::ofstream o(path);
    o << std::setw(4) << j << std::endl;
}

void Json::makeJson(Lyric lyric)
{
    json j;
    j["composer"] = m_composer;
    j["copyright"] = m_copyright;
    j["genre"] = m_genre;
    j["key"] = m_key;

    // line
    json json_lineObject;
    {
        json_lineObject = getObject_line(lyric);
    }
    j["line"] = json_lineObject;
    j["national"] = m_national;
    j["optionType"] = m_optionType;
    j["singer"] = m_singer;
    j["supportvocal"] = m_supportvocal;
    j["title"] = m_title;
    j["version"] = m_version;
    j["vocaldelay"] = m_vocaldelay;
    j["writer"] = m_writer;

    writeDoc(j);
}

json Json::getObject_line(Lyric lyric)
{
    json json_lineObject = json::array();

    json_lineObject.push_back(getObject_eventInterludeObject(5000));
    json_lineObject.push_back(getObject_eventCountObject(5000));

    // lyric
    for (int i = 0; i < lyric.getLinesSize(); i++)
    {
        json_lineObject.push_back(getObject_eventLyricObject(*lyric.getLine(i)));
    }

    return json();
}

json Json::getObject_eventObject(eType_object_name evnetType)
{
    json objValue;

    switch (evnetType)
    {
    case eType_object_name::interlude:
    {
        objValue["event"] = "interlude";
        objValue["start"] = 123;
    }
    break;
    case eType_object_name::count:
    {
        objValue["count"] = 4;
        objValue["countms"] = 500;
        objValue["event"] = "count";
        objValue["start"] = 123;
    }
    break;
    case eType_object_name::lyrics:
    {
        // getObject_eventLyricObject 
    }
    break;
    default:
        break;
    }

    return objValue;
}

json Json::getObject_eventInterludeObject(int startms)
{
    json objValue;
    objValue["event"] = "interlude";
    objValue["start"] = startms;
    return objValue;
}

json Json::getObject_eventCountObject(int startms)
{
    json objValue;
    objValue["count"] = 4;
    objValue["countms"] = 500;
    objValue["event"] = "count";
    objValue["start"] = startms;
    return objValue;
}

json Json::getObject_eventLyricObject(Line line)
{
    json objValue;

    objValue["event"]= "lyrics";
    // get object arr
    json json_lineObject;
    json_lineObject = getObject_linePartArr(line);

    objValue["linepart"] = json_lineObject;
    objValue["part"] = "N";
    if(line.words.size()>0)
        if(line.words[0].startFrame - 500 >0)
            objValue["start"] = line.words[0].startFrame-500;
        else
            objValue["start"] = line.words[0].startFrame;
    else 
        objValue["start"] = line.words[0].startFrame;

    return objValue;
}

json Json::getObject_linePartArr(Line line)
{
    json linepartArray = json::array();

    for (int i = 0; i < line.words.size(); i++)
    {
        json arrObject;
        arrObject["end"] = line.words[i].endFrame;
        arrObject["start"] = line.words[i].startFrame;
        arrObject["text"] = line.words[i].text;

        linepartArray.push_back(arrObject);
    }

    return linepartArray;
}
