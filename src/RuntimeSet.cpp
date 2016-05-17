#include "RuntimeSet.h"

OPEN_NAMESPACE_JFR

/// Runtime_st
Runtime_st::Runtime_st(void)
{
	m_nStat = RTS_INVALID;
	m_pLine = NULL;
	m_pTriggerCtx = NULL;
	m_bInit = false;
}

Runtime_st::~Runtime_st(void)
{
	map< ModArg_t*, ArgValue_t* >::iterator iter1, end1;
	map< LineModule_t*, ModContext_t* >::iterator iter2, end2;

	StaticModuleArgSet& argSet = StaticModuleArgSet::get_mutable_instance();
    end1 = m_mapArgs.end();
    for (iter1 = m_mapArgs.begin(); iter1 != end1; ++iter1)
	{
		if (argSet.GetElement(iter1->first))
		{
			continue;
		}
		delete iter1->second;
	}
	end2 = m_mapModCtxs.end();
	for (iter2 = m_mapModCtxs.begin(); iter2 != end2; ++iter2)
	{
		delete iter2->second;
	}
	delete m_pTriggerCtx;
}

void Runtime_st::Recycling(void)
{
	map< ModArg_t*, ArgValue_t* >::iterator iter1, end1;
	map< LineModule_t*, ModContext_t* >::iterator iter2, end2;

	boost::lock_guard< boost::mutex > guard(m_oMutex);
	StaticModuleArgSet& argSet = StaticModuleArgSet::get_mutable_instance();
    m_nStat = RTS_INIT;
    end1 = m_mapArgs.end();
    for (iter1 = m_mapArgs.begin(); iter1 != end1; ++iter1)
	{
		ArgValue_t* pArgValue = iter1->second;
		ModArg_t* pModArg = iter1->first;
		if (argSet.GetElement(pModArg))
		{
			continue;
		}
		if (pModArg->m_nType != AT_STRING)
		{
			pArgValue->m_oMutex.lock();
			pArgValue->Free();
			pArgValue->m_oMutex.unlock();
		}
	}
	end2 = m_mapModCtxs.end();
	for (iter2 = m_mapModCtxs.begin(); iter2 != end2; ++iter2)
	{
		ModContext_t* pCtx = iter2->second;
		pCtx->m_oMutex.lock();
		pCtx->m_nStat = RTS_INIT;
		pCtx->m_nRetValue = 0;
		pCtx->m_oMutex.unlock();
	}
	m_pTriggerCtx->m_oMutex.lock();
	m_pTriggerCtx->m_nStat = RTS_INIT;
	m_pTriggerCtx->m_nRetValue = 0;
	m_pTriggerCtx->m_oMutex.unlock();
}

int Runtime_st::Init(Line_t* line)
{
	assert(line);
	boost::lock_guard< boost::mutex > guard(m_oMutex);
	StaticModuleArgSet& argSet = StaticModuleArgSet::get_mutable_instance();
	m_nStat = RTS_INIT;
	m_pLine = line;
	for (size_t i = 0; i < m_pLine->m_vArgs.size(); ++i)
	{
		ModArg_t* pModArg = m_pLine->m_vArgs[i];
		assert(pModArg);
		ArgValue_t* pArgValue = argSet.GetElement(pModArg);
		if (!pArgValue)
		{
			pArgValue = new ArgValue_t;		// do not lock
			pArgValue->m_pModArg = pModArg;
			if (pModArg->m_nType == AT_STRING)
			{
				pArgValue->m_pValue = malloc(pModArg->m_sName.length() + 1);
				memset(pArgValue->m_pValue, 0, pModArg->m_sName.length() + 1);
				strncpy((char*)pArgValue->m_pValue, pModArg->m_sName.c_str(), pModArg->m_sName.length());
				pArgValue->m_pFreeFunc = free;
			}
		}
		m_mapArgs.insert(make_pair(pModArg, pArgValue));
	}
	for (size_t i = 0; i < m_pLine->m_vModules.size(); ++i)
	{
		LineModule_t* pLineMod = m_pLine->m_vModules[i];
		assert(pLineMod);
		ModContext_t* pCtx = new ModContext_t;
		pCtx->m_nStat = RTS_INIT;		// do not lock
		pCtx->m_nRetValue = 0;
		m_mapModCtxs.insert(make_pair(pLineMod, pCtx));
	}
	m_pTriggerCtx = new ModContext_t;
	m_pTriggerCtx->m_oMutex.lock();
	m_pTriggerCtx->m_nStat = RTS_INIT;
	m_pTriggerCtx->m_nRetValue = 0;
	m_pTriggerCtx->m_oMutex.unlock();
	m_bInit = true;

	return 0;
}


/// StaticRuntime_st
StaticRuntime_st::StaticRuntime_st(void)
{
	m_pStaticModule = NULL;
	m_pCtx = NULL;
	m_bInit = false;
}

StaticRuntime_st::~StaticRuntime_st(void)
{
	map< ModArg_t*, ArgValue_t* >::iterator iter, end;

	delete m_pCtx;
	end = m_mapArgs.end();
	for (iter = m_mapArgs.begin(); iter != end; ++iter)
	{
		delete iter->second;
	}
}

void StaticRuntime_st::Recycling(void)
{
	map< ModArg_t*, ArgValue_t* >::iterator iter, end;
	boost::lock_guard< boost::mutex > guard(m_oMutex);
	if (!m_bInit)
	{
		return;
	}
	m_pCtx->m_oMutex.lock();
	m_pCtx->m_nStat = RTS_INIT;
	m_pCtx->m_nRetValue = 0;
	m_pCtx->m_oMutex.unlock();
	end = m_mapArgs.end();
	for (iter = m_mapArgs.begin(); iter != end; ++iter)
	{
		ArgValue_t* pArgValue = iter->second;
		ModArg_t* pModArg = iter->first;
		if (pModArg->m_nType != AT_STRING)
		{
			pArgValue->m_oMutex.lock();
			pArgValue->Free();
			pArgValue->m_oMutex.unlock();
		}
	}
}

int StaticRuntime_st::Init(LineStaticModule_t* module)
{
	assert(module);
	m_pStaticModule = module;
	m_pCtx = new ModContext_t;
	m_pCtx->m_oMutex.lock();
	m_pCtx->m_nStat = RTS_INIT;
	m_pCtx->m_nRetValue = 0;
	m_pCtx->m_oMutex.unlock();
	for (size_t i = 0; i < m_pStaticModule->m_vArgs.size(); ++i)
	{
		ModArg_t* pModArg = m_pStaticModule->m_vArgs[i];
		assert(pModArg);
		ArgValue_t* pArgValue = new ArgValue_t;		// do not lock
		pArgValue->m_pModArg = pModArg;
		if (pModArg->m_nType == AT_STRING)
		{
			pArgValue->m_pValue = malloc(pModArg->m_sName.length() + 1);
			memset(pArgValue->m_pValue, 0, pModArg->m_sName.length() + 1);
			strncpy((char*)pArgValue->m_pValue, pModArg->m_sName.c_str(), pModArg->m_sName.length());
			pArgValue->m_pFreeFunc = free;
		}
		m_mapArgs.insert(make_pair(pModArg, pArgValue));
		StaticModuleArgSet& argSet = StaticModuleArgSet::get_mutable_instance();
		argSet.InsertElement(pModArg, pArgValue);
	}
	m_bInit = true;

	return 0;
}


/// StaticModuleArgSet
StaticModuleArgSet::StaticModuleArgSet(void)
{
}

StaticModuleArgSet::~StaticModuleArgSet(void)
{
}

int StaticModuleArgSet::InsertElement(ModArg_t* pModArg, ArgValue_t* pArgVal)
{
	assert(pModArg && pArgVal);
	m_mapArgs.insert(make_pair(pModArg, pArgVal));

	return 0;
}

ArgValue_t* StaticModuleArgSet::GetElement(ModArg_t* pModArg)
{
	map< ModArg_t*, ArgValue_t* >::iterator iter;

	assert(pModArg);
	iter = m_mapArgs.find(pModArg);
	if (iter == m_mapArgs.end())
	{
		return NULL;
	}

	return iter->second;
}


CLOSE_NAMESPACE_JFR
