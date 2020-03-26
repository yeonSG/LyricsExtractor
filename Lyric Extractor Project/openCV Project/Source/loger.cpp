//#include "loger.h"
//
//void loger::LogerTest1()
//{
//	using namespace logging::trivial;
//
//	logging::sources::severity_logger< severity_level > lg;
//
//	BOOST_LOG_TRIVIAL(trace) << "A trace severity message"; 
//	BOOST_LOG_TRIVIAL(debug) << "A debug severity message"; 
//	BOOST_LOG_TRIVIAL(info) << "An informational severity message"; 
//	BOOST_LOG_TRIVIAL(warning) << "A warning severity message"; 
//	BOOST_LOG_TRIVIAL(error) << "An error severity message"; 
//	BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";
//
//	BOOST_LOG_SEV(lg, trace) << "A trace severity message";
//	BOOST_LOG_SEV(lg, debug) << "A debug severity message";
//	BOOST_LOG_SEV(lg, info) << "An informational severity message";
//	BOOST_LOG_SEV(lg, warning) << "A warning severity message";
//	BOOST_LOG_SEV(lg, error) << "An error severity message";
//	BOOST_LOG_SEV(lg, fatal) << "A fatal severity message";
//}
//
//void loger::LogerInit()
//{
//	using namespace logging::trivial;
//	using namespace logging::keywords;
//
//	//boost::log::add_file_log("sample.log");
//	logging::add_common_attributes();
//
//	//BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
//
//	logging::add_file_log
//	(
//		logging::keywords::file_name = "sample_%N.log",                                        /*< file name pattern >*/
//		logging::keywords::rotation_size = 10 * 1024 * 1024,                                   /*< rotate files every 10 MiB... >*/
//		logging::keywords::time_based_rotation = logging::sinks::file::rotation_at_time_point(0, 0, 0), /*< ...or at midnight >*/
//		logging::keywords::format = "[%TimeStamp%]: %Message%"                                 /*< log record format >*/
//	);
//
//	//core::get()->set_filter(trivial::severity >= trivial::info);	// 로그 래밸 설정 , info래밸부터 출력
//
//}
//
//
//
#include "loger.h"
#include <boost/log/support/date_time.hpp>


void _logging(std::string msg, severity_level logType)
{
    src::severity_logger< severity_level > slg;

    printf("%s", msg);
    BOOST_LOG_SEV(slg, logType) << msg;
}

src::severity_logger< severity_level > getSL()
{
    src::severity_logger< severity_level > slg;
    return slg;
}


void logging_function()
{
    src::severity_logger< severity_level > slg;

    BOOST_LOG_SEV(slg, normal) << "A regular message";
    BOOST_LOG_SEV(slg, warning) << "Something bad is going on but I can handle it";
    BOOST_LOG_SEV(slg, critical) << "Everything crumbles, shoot me now!";
}

//[ example_tutorial_attributes_named_scope
void named_scope_logging()
{
    BOOST_LOG_NAMED_SCOPE("named_scope_logging");

    src::severity_logger< severity_level > slg;

    BOOST_LOG_SEV(slg, normal) << "Hello from the function named_scope_logging!";
}
//]

//[ example_tutorial_attributes_tagged_logging
void tagged_logging()
{
    src::severity_logger< severity_level > slg;
    slg.add_attribute("Tag", attrs::constant< std::string >("My tag value"));

    BOOST_LOG_SEV(slg, normal) << "Here goes the tagged record";
}
//]

//[ example_tutorial_attributes_timed_logging
void timed_logging()
{
    BOOST_LOG_SCOPED_THREAD_ATTR("Timeline", attrs::timer());

    src::severity_logger< severity_level > slg;
    BOOST_LOG_SEV(slg, normal) << "Starting to time nested functions";

    logging_function();

    BOOST_LOG_SEV(slg, normal) << "Stopping to time nested functions";
}
//]

void log_timer_start()
{
    BOOST_LOG_SCOPED_THREAD_ATTR("Timeline", attrs::timer());
}

// The operator puts a human-friendly representation of the severity level to the stream
std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    static const char* strings[] =
    {
        "normal",
        "notification",
        "warning",
        "error",
        "critical"
    };

    if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast<int>(level);

    return strm;
}

void loger_init(std::string savePath)
{
 
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

    sink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >(savePath+"\\log.txt")); 

    sink->set_formatter(expr::format("[%1%] : %2%") 
        //% expr::attr< unsigned int >("RecordID") 
        % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%e") 
        //% severity
        % expr::message);
    
    sink->flush();

    logging::core::get()->remove_all_sinks();
    logging::core::get()->add_sink(sink);


    // Add attributes
    logging::add_common_attributes();
    logging::core::get()->add_global_attribute("Scope", attrs::named_scope());
       
}
