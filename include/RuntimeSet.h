#ifndef JFR_RUNTIME_SET_H
#define JFR_RUNTIME_SET_H


#include <string>
#include <map>
#include <vector>
#include <list>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/serialization/singleton.hpp>
#include <RecyclineFactory/RecyclineFactory.hpp>
#include "Common.h"
#include "MainlineManager.h"
#include "Logger.h"


OPEN_NAMESPACE_JFR


using namespace std;
USING_NAMESPACE_SWITCHTOOL


/// 运行状态
enum RuntimeStatus
{
    RTS_INIT,           // 初始化
    RTS_WAIT,           // 等待
    RTS_RUN,            // 正在运行
    RTS_FINISH,         // 运行结束
    RTS_EQUAL,          // 等效状态
    RTS_STATIC,         // 静态模块状态
    RTS_ERROR,          // 运行出错
    RTS_SYSERROR,		// 系统错误
    RTS_DESTROY,        // 销毁
    RTS_INVALID = 100	// 非法状态
};

/// 模块运行上下文结构
struct ModContext_st
{
    unsigned int 					m_nStat;	    // 状态 : init, wait, run, finish, equal, static, error, destroy
    int								m_nRetValue;    // 运行返回值
    boost::mutex					m_oMutex;
};

/// 主线运行时结构
struct Runtime_st : public Product
{
public:
	Runtime_st(void);
	~Runtime_st(void);
	void Recycling(void);
	int Init(Line_t* line);

	unsigned int 				    m_nStat;			// 状态 : init, wait, run, finish, error, destroy
	Line_t*						    m_pLine;            // 主线指针
	map< ModArg_t*, ArgValue_t* >	m_mapArgs;          // 参数结果map
	map< LineModule_t*, ModContext_t* >	m_mapModCtxs;	// 模块运行上下文
	ModContext_t*					m_pTriggerCtx;		// 触发器运行上下文
	bool							m_bInit;
	boost::mutex					m_oMutex;
};

/// 静态模块运行时结构
struct StaticRuntime_st : public Product
{
public:
	StaticRuntime_st(void);
	~StaticRuntime_st(void);
	void Recycling(void);
	int Init(LineStaticModule_t* module);

	LineStaticModule_t*				m_pStaticModule;	// 静态模块指针
	ModContext_t*					m_pCtx;				// 运行上下文
	map< ModArg_t*, ArgValue_t* >	m_mapArgs;          // 参数结果map
	bool							m_bInit;
	boost::mutex					m_oMutex;
};

/// 运行时环境集合
template < typename RuntimeType, typename LineType >
class RuntimeSet : public boost::serialization::singleton< RuntimeSet< RuntimeType,  LineType > >
{
public:
	typedef RecyclingFactorySingleton< RuntimeType, string > RuntimeFactory;
	typedef typename RecyclingFactorySingleton< RuntimeType, string >::TheProduct RuntimeProduct;

    int Init(const vector< LineType* >& vLines)
    {
        boost::lock_guard< boost::mutex > guard(m_oMutex);
        if (m_bInited)
        {
            Clear();
        }

        m_pRuntimeFactory = &RuntimeFactory::get_mutable_instance();
        for(size_t i = 0; i < vLines.size(); ++i)
        {
            assert(vLines[i]);
            m_mapLines.insert(make_pair(vLines[i]->m_sName, vLines[i]));
        }
        m_bInited = true;

        return 0;
	}
	RuntimeType* Obtain(const string& name)
	{
        typename map< string, LineType*>::iterator m_iter;
        RuntimeProduct* container;
        RuntimeType* product;

        boost::lock_guard< boost::mutex > guard(m_oMutex);
        container = m_pRuntimeFactory->Produce(name);
        product = container->GetProduct();
        if (!product->m_bInit)		// init product
        {
            m_iter = m_mapLines.find(name);
            if (m_iter == m_mapLines.end())
            {
                m_pRuntimeFactory->Destroy(container);
                return NULL;
            }
            if (product->Init(m_iter->second))
            {
                return NULL;
            }
        }
        m_lEffectProducts.push_back(container);

        return product;
    }
	int Release(const RuntimeType* product)
	{
        typename list< RuntimeProduct* >::iterator l_iter, l_end;
        RuntimeProduct* container;

        assert(product);
        boost::lock_guard< boost::mutex > guard(m_oMutex);
        l_end = m_lEffectProducts.end();
        for(l_iter = m_lEffectProducts.begin(); l_iter != l_end; ++l_iter)
        {
            container = *l_iter;
            if (container->GetProduct() == product)
            {
                break;
            }
        }
        if (l_iter == l_end)
        {
            return -1;
        }
        m_lEffectProducts.erase(l_iter);
        m_pRuntimeFactory->Recycling(container);

        return 0;
	}

protected:
	RuntimeSet(void)
	{
        assert(TypeValid());
        m_pRuntimeFactory = NULL;
        m_bInited = false;
	}
	~RuntimeSet(void)
	{
        Clear();
	}

private:
	inline void Clear(void)
	{
        typename list< RuntimeProduct* >::iterator l_iter, l_end;
        boost::lock_guard< boost::mutex > guard(m_oMutex);

        m_mapLines.clear();
        l_end = m_lEffectProducts.end();
        for (l_iter = m_lEffectProducts.begin(); l_iter != l_end; ++l_iter)
        {
            m_pRuntimeFactory->Recycling(*l_iter);
        }
        m_lEffectProducts.clear();
        m_bInited = false;
	}
    inline bool TypeValid(void)
    {
        return (is_same< RuntimeType, StaticRuntime_t >::value && is_same< LineType, LineStaticModule_t >::value) || \
				(is_same< RuntimeType, Runtime_t >::value && is_same< LineType, Line_t >::value);
    }

private:
	RuntimeFactory*					m_pRuntimeFactory;
    map< string, LineType* >		m_mapLines;				// line name -- line
    list< RuntimeProduct* >			m_lEffectProducts;		// 有效产品
    boost::mutex					m_oMutex;
	bool							m_bInited;
};

/// 静态模块参数集合
class StaticModuleArgSet : public boost::serialization::singleton< StaticModuleArgSet >
{
public:
	int InsertElement(ModArg_t* pModArg, ArgValue_t* pArgVal);
	ArgValue_t* GetElement(ModArg_t* pModArg);

protected:
	StaticModuleArgSet(void);
	~StaticModuleArgSet(void);

private:
    map< ModArg_t*, ArgValue_t* >	m_mapArgs;
};


CLOSE_NAMESPACE_JFR


#endif // JFR_RUNTIME_SET_H
