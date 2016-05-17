#ifndef JFR_COMMON_H
#define JFR_COMMON_H


#ifndef NAMESPACE_JFR
    #define NAMESPACE_JFR
    #define OPEN_NAMESPACE_JFR       namespace jfr {
    #define CLOSE_NAMESPACE_JFR      	};
    #define USING_NAMESPACE_JFR      using namespace jfr;
#endif


#include <string.h>
#include <errno.h>
#include <string>
#include <boost/thread/mutex.hpp>
#include <Logger/Logger.hpp>


using namespace std;


OPEN_NAMESPACE_JFR


#ifndef JFR_ERROR_INFO
#define JFR_ERROR_INFO
	#ifndef SET_ERROR
	#define SET_ERROR(error,code,error_str) \
			do{\
				error.clear();\
				error.errno_ = code;\
				error.message += error_str;\
				error.filename += __FILE__;\
				error.funname += __func__;\
				error.line = __LINE__;\
			}while(0);
	#endif
	#ifndef SHOW_ERROR
	#define SHOW_ERROR(error)\
			do{\
				std::cerr << error.errno_;\
				std::cerr << ": " << error.message;\
				std::cerr << " : position : "  <<  error.filename;\
				std::cerr << " | " << error.funname;\
				std::cerr << " | " << error.line << std::endl;\
			}while(0);
	#endif
struct ErrorInfo
{
    int errno_;
    int line;
    std::string message;
    std::string filename;
    std::string funname;

    inline void clear()
    {
        message.clear();
        filename.clear();
        funname.clear();
        errno_ = 0;
        line = 0;
    }
};
#endif

struct false_type
{
    static const bool value = false;
};
struct true_type
{
    static const bool value = true;
};
template<typename, typename>
struct is_same
: public false_type { };
template<typename _Tp>
struct is_same<_Tp, _Tp>
: public true_type { };

/// 参数类型
enum ArgType
{
    AT_STRING = 1,			// 字符串
    AT_VARIABLE,			// 变量
    AT_INVALID = 100
};

/// 参数结构
struct ModArg_st
{
	string				m_sName;		// 参数名 or 字符串参数值
	unsigned int 		m_nType;		// 参数类型

	ModArg_st(void)
	{
		m_sName = "";
		m_nType = 0;
	}
};

/// 参数值结构
struct ArgValue_st
{
	typedef void (*ArgFree)(void*);
    void*						m_pValue;           // 参数值指针
    ArgFree            			m_pFreeFunc;        // 释放回调函数
    ModArg_st*					m_pModArg;
    boost::mutex				m_oMutex;
	ArgValue_st(void)
	{
        m_pValue = NULL;
        m_pFreeFunc = NULL;
        m_pModArg = NULL;
	}
	~ArgValue_st(void)
	{
		Free();
	}
	inline void Free(void)
	{
		if (m_pFreeFunc)
		{
			m_pFreeFunc(m_pValue);
		}
		m_pValue = NULL;
		m_pFreeFunc = NULL;
	}
};

/// 返回值配置项
enum RetType
{
	RT_ANY	=	1,		// 任意返回值
	RT_SINGLE			// 单一返回值
};
struct RetValue_st
{
	RetType				m_nType;			// 类型
    int 				m_nRetValue;		// 返回值

    inline bool Euqal(int val) const
    {
		return (m_nType == RT_ANY || m_nRetValue == val);
    }
};


class LoggerSingleton;
typedef struct ModArg_st ModArg_t;
typedef struct Module_st Module_t;
typedef struct Module_st Trigger_t;
typedef struct Module_st StaticModule_t;
typedef struct LineTrigger_st LineTrigger_t;
typedef struct LineModule_st LineModule_t;
typedef struct LineStaticModule_st LineStaticModule_t;
typedef struct Line_st Line_t;
typedef struct ArgValue_st ArgValue_t;
typedef struct ModContext_st ModContext_t;
typedef struct Runtime_st Runtime_t;
typedef struct StaticRuntime_st StaticRuntime_t;
typedef struct ConfigModule_st ConfigModule_t;
typedef struct ConfigModule_st ConfigTrigger_t;
typedef struct ConfigModule_st ConfigStaticModule_t;
typedef struct ConfigMainLine_st ConfigMainLine_t;
typedef struct RetValue_st RetValue_t;
typedef void (*ArgFree)(void*);
typedef int (*ModCallback)(jfr::LoggerSingleton* pLog, ArgValue_t** pInput, ArgValue_t** pOutput);


#ifndef MAIN_NAME
#define MAIN_NAME								"jfr"
#endif // MAIN_NAME
#ifndef FLOW_NAME
#define FLOW_NAME								MAIN_NAME"_flow"
#endif // FLOW_NAME
#define MODULE_JFR								MAIN_NAME


CLOSE_NAMESPACE_JFR


#endif // JFR_COMMON_H
