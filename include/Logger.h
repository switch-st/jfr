#ifndef JFR_LOGGER_H
#define JFR_LOGGER_H

#include <iostream>
#include <string>
#include <sstream>
#include <boost/serialization/singleton.hpp>
#include "Common.h"


OPEN_NAMESPACE_JFR


using namespace std;


#define JFR_LOGGER_LEVEL_FATAL						"FATAL"
#define JFR_LOGGER_LEVEL_ERROR						"ERROR"
#define JFR_LOGGER_LEVEL_WARNING					"WARNING"
#define JFR_LOGGER_LEVEL_INFO						"INFO"
#define JFR_LOGGER_LEVEL_DEBUG_1					"DEBUG_1"
#define JFR_LOGGER_LEVEL_DEBUG_2					"DEBUG_2"
#define JFR_LOGGER_LEVEL_DEBUG_3					"DEBUG_3"
#define FATAL										JFR_LOGGER_LEVEL_FATAL
#define ERROR										JFR_LOGGER_LEVEL_ERROR
#define WARNING										JFR_LOGGER_LEVEL_WARNING
#define INFO										JFR_LOGGER_LEVEL_INFO
#define DEBUG_1										JFR_LOGGER_LEVEL_DEBUG_1
#define DEBUG_2										JFR_LOGGER_LEVEL_DEBUG_2
#define DEBUG_3										JFR_LOGGER_LEVEL_DEBUG_3

#define LOG_FUNC_STRING								"[%s(%s:%d)] "
#define LOG_FUNC_VALUE 								__PRETTY_FUNCTION__, __FILE__, __LINE__


class LoggerSingleton : private Switch::Tool::Logger, public boost::serialization::singleton< LoggerSingleton >
{
public:
	int Init(int nBaseLevel, const string& sFilePath, const string& sLogPrefix, bool bIsDev = false);
	void SetChecker(void);
	int LogWrite(const std::string& level, const std::string& module, const std::string& format, ...);
	int LogWrite(const std::string& level, const std::string& module, const std::string& format, va_list ap);

protected:
	LoggerSingleton(void) {}
	~LoggerSingleton(void) {}

private:
	static string LogLevels[];
};


CLOSE_NAMESPACE_JFR


#endif // JFR_LOGGER_H
