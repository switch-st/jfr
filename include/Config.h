#ifndef JFR_CONFIG_H
#define JFR_CONFIG_H


#include <iostream>
#include <string>
#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_utils.hpp>
#include <boost/serialization/singleton.hpp>
#include <Daemonize/Daemonize.hpp>
#include "Common.h"


OPEN_NAMESPACE_JFR


using namespace std;
using namespace rapidxml;


#define JFR_DEFAULT_CONFIG_VERSION					MAIN_NAME"/v0.1.5"
#define JFR_DEFAULT_CONFIG_DATE						__DATE__" "__TIME__
#define JFR_DEFAULT_CONFIG_VERSIONSTRING			"Version    : "JFR_DEFAULT_CONFIG_VERSION
#define JFR_DEFAULT_CONFIG_DATESTRING				"Built Date : "JFR_DEFAULT_CONFIG_DATE
#define JFR_DEFAULT_CONFIG_BASEDIR					"./"
#define JFR_DEFAULT_CONFIG_CFGFILENAME				MAIN_NAME".xml"
#define JFR_DEFAULT_CONFIG_FLOWFILENAME				FLOW_NAME".xml"
#define JFR_DEFAULT_CONFIG_LOGPREFIX				MAIN_NAME
#define JFR_DEFAULT_CONFIG_MAXRUNLINES				5
#define JFR_DEFAULT_CONFIG_MAXTHREADSIZE			10
#define JFR_DEFAULT_CONFIG_MAXJOBSIZE				20
#define JFR_DEFAULT_CONFIG_DEBUGLEVEL_MIN			0
#define JFR_DEFAULT_CONFIG_DEBUGLEVEL_MAX			3
#define JFR_DEFAULT_CONFIG_DEBUGLEVEL				JFR_DEFAULT_CONFIG_DEBUGLEVEL_MIN
#define JFR_DEFAULT_CONFIG_DEAMON					false
#define JFR_DEFAULT_CONFIG_LOG2TERM					false
#define JFR_DEFAULT_CONFIG_CFGCHECK					false
#define JFR_DEFAULT_CONFIG_VERSIONFLAG				false


/// 配置信息
/// 配置优先级顺序：cmd > 配置文件 > 默认
class Config : public boost::serialization::singleton< Config >
{
public:
	int SetBaseDir(const string& sBaseDir);
	int SetCfgFileName(const string& sCfgFileName);
	int SetFlowFileName(const string& sFlowFileName);
	int SetLogPrefix(const string& sLogPrefix);
	int SetMaxRunLines(const int max);
	int SetMaxThreadSize(const int max);
	int SetMaxJobSize(const int max);
	int SetDebugLevel(const int level);
	int SetDaemonFlag(const bool flag);
	int SetLog2TermFlag(const bool flag);
	int SetCfgCheckFlag(const bool flag);
	int SetVersionFlag(const bool flag);
	const string& GetBaseDir(void) const;
	const string& GetCfgFileName(void) const;
	const string& GetFlowFileName(void) const;
	const string& GetLogPrefix(void) const;
	const int GetMaxRunLines(void) const;
	const int GetMaxThreadSize(void) const;
	const int GetMaxJobSize(void) const;
	const int GetDebugLevel(void) const;
	const bool GetDaemonFlag(void) const;
	const bool GetLog2TermFlag(void) const;
	const bool GetCfgCheckFlag(void) const;
	const string& GetVersion(void) const;
	const string& GetBuiltDate(void) const;
	const bool GetVersionFlag(void) const;

	int ParseConfigFile(void);

protected:
	Config(void);
	~Config(void);
private:
	string						m_sBaseDir;
	string						m_sCfgFileName;
	string						m_sFlowFileName;
	string						m_sLogPrefix;
	int							m_nMaxRunLines;
	int 						m_nMaxThreadSize;
	int							m_nMaxJobSize;
	int							m_nDebugLevel;
	bool						m_bDaemon;
	bool						m_bCfgCheck;
	bool						m_bLog2Term;
	string						m_sVersion;
	string						m_sBuiltDate;
	bool						m_bVersion;
};


CLOSE_NAMESPACE_JFR


#endif // JFR_CONFIG_H
