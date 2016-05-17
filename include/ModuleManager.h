#ifndef JFR_MODULE_NAMAGER_H
#define JFR_MODULE_NAMAGER_H


#include <unistd.h>
#include <dlfcn.h>
#include <string>
#include <map>
#include <vector>
#include <boost/serialization/singleton.hpp>
#include "Common.h"
#include "ConfigParser.h"
#include "Logger.h"


OPEN_NAMESPACE_JFR


#define GET_MODULE_TYPE(type1, type2)	((type1) == "so" ? MT_SO | (type2) : MT_PRO | (type2))
#define GET_MODULE_TYPE_MOD(type1)		GET_MODULE_TYPE(type1, MT_MODULE)
#define GET_MODULE_TYPE_TRIG(type1)		GET_MODULE_TYPE(type1, MT_TRIGGER)
#define GET_MODULE_TYPE_STATIC(type1)	GET_MODULE_TYPE(type1, MT_STATIC)
#define IS_MODULE(type)					(((type) & MT_MODULE) == MT_MODULE ? true : false)
#define IS_TRIGGER(type)				(((type) & MT_TRIGGER) == MT_TRIGGER ? true : false)
#define IS_STATIC(type)					(((type) & MT_STATIC) == MT_STATIC ? true : false)
#define IS_SO(type)						(((type) & MT_SO) == MT_SO ? true : false)
#define IS_PRO(type)					(((type) & MT_PRO) == MT_PRO ? true : false)


/// 模块类型 ored
enum ModuleType
{
	MT_SO 			= 			0x01,					// 动态库
	MT_PRO 			= 			0x02,					// 进程
	MT_MODULE 		= 			0x04,					// 模块
	MT_TRIGGER 		= 			0x08,					// 触发器
	MT_STATIC		=			0x10					// 静态模块
};

/// 模块结构
struct Module_st
{
    string 				m_sName;				// 模块名称
    unsigned int 		m_nType;				// 模块类型
    string				m_sMain;				// 入口函数
    string				m_sFileName;			// 文件
    string 				m_sDesc;				// 描述
    void*				m_pHandle;				// 动态库handle
    ModCallback			m_pCallback;			// 动态库回调函数
												// 进程调用方式：m_sFileName input1 input2 ...

	Module_st(void)
	{
		m_sName = "";
		m_nType = 0;
		m_sMain = "";
		m_sFileName = "";
		m_sDesc = "";
		m_pHandle = NULL;
		m_pCallback = NULL;
	}
};


class ModuleManager : public boost::serialization::singleton< ModuleManager >
{
public:
	int Init(void);
	int LoadAllModules(const vector< ConfigModule_t* >& vCfgModules, \
						const vector< ConfigTrigger_t* >& vCfgTriggers, \
						const vector< ConfigStaticModule_t* >& vCfgStaticModules);
	const Module_t* GetModule(const string& name) const;
	const Trigger_t* GetTrigger(const string& name) const;
	const StaticModule_t* GetStaticModule(const string& name) const;

protected:
	ModuleManager(void);
	~ModuleManager(void);

private:
	int LoadModules(const vector< ConfigModule_t* >& vCfgModules);
	int LoadTriggers(const vector< ConfigTrigger_t* >& vCfgTriggers);
	int LoadStaticModules(const vector< ConfigStaticModule_t* >& vCfgStaticModules);
	int SyntaxCheck(const vector< ConfigModule_t* >& vCfgModules, \
					const vector< ConfigTrigger_t* >& vCfgTriggers, \
					const vector< ConfigStaticModule_t* >& vCfgStaticModules);
	inline bool IsInited(void);
	inline void Clear(void);

private:
	map< string, Module_t* > 					m_mapAllModule;			// 所有模块map
	map< string, Module_t* > 					m_mapModule;			// 普通模块map
	map< string, Trigger_t* > 					m_mapTrigger;			// 触发器模块map
	map< string, StaticModule_t* > 				m_mapStaticModule;		// 静态模块map
	bool										m_bInited;
	jfr::LoggerSingleton*						m_pLogger;
};


CLOSE_NAMESPACE_JFR


#endif // JFR_MODULE_NAMAGER_H
