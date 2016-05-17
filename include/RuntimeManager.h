#ifndef JFR_RUNTIME_MANAGER_H
#define JFR_RUNTIME_MANAGER_H


#include <string>
#include <map>
#include <vector>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/serialization/singleton.hpp>
#include "Common.h"
#include "RuntimeSet.h"
#include "ModuleCaller.h"
#include "Logger.h"


OPEN_NAMESPACE_JFR

using namespace std;

class RuntimeManager : public boost::serialization::singleton< RuntimeManager >
{
public:
	int Init(size_t max, const vector< Line_t* >& vLines, const vector< LineStaticModule_t* >& vStaticModules);
	int Run(void);

protected:
    RuntimeManager(void);
    ~RuntimeManager(void);

private:
	int RunStaticModules(void);
	int RunLines(void);
	inline int MainlineFSM(Runtime_t* pRuntime);
	inline int TriggerFSM(Runtime_t* pRuntime);
	inline int ModuleFSM(Runtime_t* pRuntime);
	inline int FSMTriggerInit(Runtime_t* pRuntime);
	inline int FSMTriggerFinish(Runtime_t* pRuntime);
	inline int FSMEndModuleFinish(Runtime_t* pRuntime, ModContext_t* pEndCtx);
	inline int FSMModuleWait(Runtime_t* pRuntime, LineModule_t* pLineMod, ModContext_t* pCtx);
	inline int FSMLineDestroy(Runtime_t* pRuntime);
	inline LineModule_t* FindLineMod(const vector< LineModule_t* >& vLineMods, const Module_t* pMod);
	inline const RetValue_t& FindRequireRetVal(const LineModule_t* pLineMod, const Module_t* pMod);
	inline bool FindProcedMod(const set< Module_t* >& sProcedMods, const Module_t* pMod);
    inline void Clear(void);
    inline void ClearStaticModules(void);
    inline void ClearLines(void);
    inline bool IsInited(void);

private:
	RuntimeSet< Runtime_t, Line_t >*						m_pLineSet;
	RuntimeSet< StaticRuntime_t, LineStaticModule_t >*		m_pStaticSet;
    list< Runtime_t* >        		m_lRunList;				// 正在运行的主线集合
    unsigned int 					m_nRunListMaxSize;
    vector< string >				m_vLineNames;			// 主线名称
    list< StaticRuntime_t* >		m_lStaticList;			// 静态模块运行结果集合
    vector< string >				m_vStaticNames;			// 静态模块名称
    bool 							m_bInited;
    jfr::LoggerSingleton*			m_pLogger;
};


/// 运行结束
// 开始运行前现找到结束条件(只能有一个)
// 将等效条件作为一个集合，非等效的条件单独作为一个集合
// 当一个模块所依赖的模块集合状态均满足要求时，此模块开始运行
// 当一个模块所依赖的模块集合状态为error时，此模块的状态置为error
// 以此类推，直至几个结束条件的状态均为error或finish
// 等效条件集合中，处于equal状态的模块不处理，视为第三种状态
// 等效条件集合中，条件有一个满足要求时，整个集合就满足要求
// 等效条件集合中，条件均不满足要求时，整个集合的状态为error


CLOSE_NAMESPACE_JFR


#endif // JFR_RUNTIME_MANAGER_H
