#ifndef JFR_MAINLINE_MANAGER_H
#define JFR_MAINLINE_MANAGER_H


#include <string>
#include <map>
#include <vector>
#include <boost/serialization/singleton.hpp>
#include "Common.h"
#include "ModuleManager.h"
#include "Logger.h"


OPEN_NAMESPACE_JFR


using namespace std;


/// 主线触发器结构
struct LineTrigger_st
{
    Trigger_t*				m_pTrigger;			// 触发器指针
    vector< ModArg_t* >		m_vInputArgs;		// 入参
    vector< ModArg_t* >		m_vOutputArgs;		// 出参

    LineTrigger_st(void)
    {
    	m_pTrigger = NULL;
    }
    ~LineTrigger_st(void)
    {
    	Clear();
    }
    void Clear()
    {
        m_pTrigger = NULL;
        m_vInputArgs.clear();
        m_vOutputArgs.clear();
    }
};

/// 主线模块结构
struct LineModule_st
{
    Module_t*								m_pModule;				// 模块指针
    vector< ModArg_t* >						m_vInputArgs;			// 入参
    vector< ModArg_t* >						m_vOutputArgs;			// 出参
    vector< pair< Module_t*, RetValue_t > >	m_vRequirement;			// 必要条件
    vector< Module_t* >						m_vEquivalent;			// 等效条件

    LineModule_st(void)
    {
    	m_pModule = NULL;
    }
    ~LineModule_st(void)
    {
    	Clear();
    }
    void Clear(void)
    {
    	m_pModule = NULL;
    	m_vInputArgs.clear();
    	m_vOutputArgs.clear();
    	m_vRequirement.clear();
    	m_vEquivalent.clear();
    }
};

/// 静态模块
struct LineStaticModule_st
{
	string 						m_sName;				// 静态模块名称
	LineModule_t 				m_oModule;				// 结构
	vector< ModArg_t* >			m_vArgs;				// 所有参数

	LineStaticModule_st(void)
	{
		m_sName = "";
	}
	~LineStaticModule_st(void)
	{
		Clear();
	}
	void Clear(void)
	{
		m_sName = "";
		m_oModule.Clear();
		for (size_t i = 0; i < m_vArgs.size(); ++i)
		{
			delete m_vArgs[i];
		}
		m_vArgs.clear();
	}
};

/// 主线结构
struct Line_st
{
    string						m_sName;				// 主线名称
    string						m_sDesc;				// 描述
    LineTrigger_t				m_oTrigger;				// 触发器
    vector< LineModule_t* >		m_vModules;				// 模块
    LineModule_t*				m_pEnd;					// 结束条件
    vector< ModArg_t* >			m_vArgs;				// 所有参数
	map< int, vector< LineModule_t* > >	m_mapModIds;	// mod id -> mods 为所有mod编号，等效条件使用一个编号
	map< LineModule_t*, int >			m_mapModPtr;	// mod ptr -> mod id

	Line_st(void)
	{
		m_sName = "";
		m_sDesc = "";
		m_oTrigger.Clear();
		m_pEnd = NULL;
	}

	~Line_st(void)
	{
		Clear();
	}

	void Clear(void)
	{
        m_sName = "";
        m_sDesc = "";
        m_oTrigger.Clear();
        m_pEnd = NULL;
        m_mapModIds.clear();
        m_mapModPtr.clear();
        for (size_t i = 0; i < m_vModules.size(); ++i)
		{
			delete m_vModules[i];
		}
		m_vModules.clear();
		for (size_t i = 0; i < m_vArgs.size(); ++i)
		{
			delete m_vArgs[i];
		}
		m_vArgs.clear();
	}
};


class MainlineManager : public boost::serialization::singleton< MainlineManager >
{
public:
	int Init(void);
	int LoadMainlines(const vector< ConfigMainLine_t* >& vCfgMainLines);				// 主程序中后调用
	int LoadStaticModules(const vector< ConfigStaticModule_t* >& vCfgStaticModules);	// 主程序中先调用
	const vector< Line_t* >& GetMainLines(void) const;
	const vector< LineStaticModule_t* >& GetStaticModules(void) const;

protected:
    MainlineManager(void);
    ~MainlineManager(void);

private:
	int LineSyntaxCheck(const vector< ConfigMainLine_t* >& vCfgMainLines);
	int LineLogicCheck(void);
	int LoadLines(const vector< ConfigMainLine_t* >& vCfgMainLines);
	int LoadLineTrigger(const ConfigTrigger_t& cfgTrigger, Line_t* pLine);
	int LoadLineModule(const ConfigModule_t& cfgModule, Line_t* pLine, LineModule_t* pMod);
	int LoadLineModuleId(Line_t* pLine, LineModule_t* pMod, int* maxId);
	int ModuleLogicCheck(void);
	int LoadModules(const vector< ConfigStaticModule_t* >& vCfgStaticModules);
	inline bool IsInited(void);
	inline void Clear(void);
	inline void ClearLine(void);
	inline void ClearStaticModule(void);
	inline int LineLogicCheck_Rule2(void);
	inline int LineLogicCheck_Rule3(void);
	inline int LineLogicCheck_Rule4(void);
	inline int LineLogicCheck_Rule6(void);
	inline int LineLogicCheck_Rule10(void);
	inline int LineLogicCheck_Rule11(void);
	inline int CheckRequireSet(const Line_t* pLine, const LineModule_t* pModule, const map< Module_t*, LineModule_t* >& mapModules);
	inline int ParseArgs(const string& strArgs, vector< ModArg_t* >& vAllArgs, vector< ModArg_t* >& vModArgs);
	inline ModArg_t* FindArg(const string& strArg, const vector< ModArg_t* >& vAllArgs);
	inline int PushArg(const string& strArgs, vector< ModArg_t* >& vAllArgs, vector< ModArg_t* >& vModArgs, unsigned int type = AT_INVALID);
	inline int TrimString(string& str);
	inline int TrimQuote(string& str);

private:
    vector< Line_t* >               m_vLines;           // 所有主线
    vector< LineStaticModule_t* >	m_vStaticModules;	// 静态模块
    bool							m_bInited;
    ModuleManager*					m_pModule;
    jfr::LoggerSingleton*			m_pLogger;

};


CLOSE_NAMESPACE_JFR


#endif // JFR_MAINLINE_MANAGER_H
