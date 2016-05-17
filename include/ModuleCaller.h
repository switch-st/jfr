#ifndef JFR_MODULE_CALLER_H
#define JFR_MODULE_CALLER_H


#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <map>
#include <vector>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/serialization/singleton.hpp>
#include "Common.h"
#include "ThreadPool.h"
#include "RuntimeSet.h"
#include "MainlineManager.h"
#include "Logger.h"


OPEN_NAMESPACE_JFR


using namespace std;


#define JFR_CALLPRO_ERROR_STRING			"JFR_CALLPRO_ERROR_STRING"
#define JFR_CALLPRO_ERROR_LEN				25
#define JFR_CALLPRO_ERROR_NO				200


class ModuleCaller
{
public:
	static int Call(StaticRuntime_t* pRuntime);
	static int Call(Runtime_t* pRuntime);
	static int Call(Runtime_t* pRuntime, LineModule_t* pModule);

private:
	static int CallStaticModule(StaticRuntime_t* pRuntime);
	static int CallModule(Runtime_t* pRuntime, LineModule_t* pModule);
	static int CallTrigger(Runtime_t* pRuntime);
	static int CallPro(Module_t* pMod, ModContext_t* pCtx, const vector< ModArg_t* >& vInput, const vector< ModArg_t* >& vOutput, map< ModArg_t*, ArgValue_t* >& mapArg);
	static int CallSo(Module_t* pMod, ModContext_t* pCtx, const vector< ModArg_t* >& vInput, const vector< ModArg_t* >& vOutput, map< ModArg_t*, ArgValue_t* >& mapArg);
	static char* Read(int fd);
	static int Wait(pid_t pid);

private:
	static ThreadPool* 						pool;
	static jfr::LoggerSingleton*			logger;
};


CLOSE_NAMESPACE_JFR


#endif // JFR_MODULE_CALLER_H
