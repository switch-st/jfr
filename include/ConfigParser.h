#ifndef JFR_CONFIG_PARSER_H
#define JFR_CONFIG_PARSER_H


#include <string>
#include <vector>
#include <set>
#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_utils.hpp>
#include <boost/serialization/singleton.hpp>
#include "Common.h"
#include "Logger.h"


using namespace rapidxml;

OPEN_NAMESPACE_JFR


struct ConfigModule_st
{
    string 				m_sName;				// 模块名称
    string		 		m_sType;				// 模块类型
    string				m_sMain;				// 入口函数
    string				m_sFileName;			// 文件
    string 				m_sDesc;				// 描述
    string				m_sInput;				// 入参
    string				m_sOutput;				// 出参
    vector< pair< string, RetValue_t > >	m_vRequirement;		// 必要条件
    vector< string >	m_vEquivalent;			// 等效条件
};

struct ConfigMainLine_st
{
    string				m_sName;				// 主线名称
    string				m_sDesc;				// 描述
    ConfigTrigger_t		m_oTrigger;				// 触发器
    vector< ConfigModule_t >	m_vModules;		// 模块
};

/// 配置文件解析
/// 不支持多线程
class ConfigParser : public boost::serialization::singleton< ConfigParser >
{
public:
	int Init(const string& basedir, const string& filename);
	int Parse(void);
	const string& GetFileName(void) const;
	const vector< ConfigModule_t* >& GetModules(void) const;
	const vector< ConfigTrigger_t* >& GetTriggers(void) const;
	const vector< ConfigStaticModule_t* >& GetStaticModules(void) const;
	const vector< ConfigMainLine_t* >& GetMainlines(void) const;

protected:
	ConfigParser(void);
	~ConfigParser(void);

private:
	int ParseModules(xml_node<>* pRoot);
	int ParseTriggers(xml_node<>* pRoot);
	int ParseStaticModules(xml_node<>* pRoot);
	int ParseMainlines(xml_node<>* pRoot);
	int NameUniqCheck(void);
	inline void FileExpand(const char* in, string& out);
	inline bool IsInited(void);
	inline void Clear(void);

private:
	string							m_sBaseDir;
	string							m_sFilename;
	vector< ConfigModule_t* >		m_vModules;
	vector< ConfigTrigger_t* >		m_vTriggers;
	vector< ConfigStaticModule_t* >	m_vStaticModules;
	vector< ConfigMainLine_t* >		m_vMainlines;
	jfr::LoggerSingleton*			m_pLogger;
	bool							m_bInited;

};


CLOSE_NAMESPACE_JFR


#endif // JFR_CONFIG_PARSER_H
