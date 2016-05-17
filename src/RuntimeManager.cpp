#include "RuntimeManager.h"

OPEN_NAMESPACE_JFR

RuntimeManager::RuntimeManager(void)
{
	m_pLineSet = NULL;
	m_pStaticSet = NULL;
	m_pLogger = NULL;
	m_nRunListMaxSize = 0;
	m_bInited = false;
}

RuntimeManager::~RuntimeManager(void)
{
    Clear();
}

int RuntimeManager::Init(size_t max, const vector< Line_t* >& vLines, const vector< LineStaticModule_t* >& vStaticModules)
{
	if (IsInited())
	{
		Clear();
	}

	m_pLineSet = &RuntimeSet< Runtime_t, Line_t >::get_mutable_instance();
	m_pStaticSet = &RuntimeSet< StaticRuntime_t, LineStaticModule_t >::get_mutable_instance();
	m_pLogger = &jfr::LoggerSingleton::get_mutable_instance();
    m_nRunListMaxSize = max;
    if (m_pLineSet->Init(vLines) || m_pStaticSet->Init(vStaticModules))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "init runtime set failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	for(size_t i = 0; i < vLines.size(); ++i)
	{
		m_vLineNames.push_back(vLines[i]->m_sName);
	}
	for (size_t i = 0; i < vStaticModules.size(); ++i)
	{
		m_vStaticNames.push_back(vStaticModules[i]->m_sName);
	}
	m_bInited = true;

	return 0;
}

void RuntimeManager::Clear(void)
{
	ClearLines();
	ClearStaticModules();
}

void RuntimeManager::ClearStaticModules(void)
{
	list< StaticRuntime_t* >::iterator l_iter, l_end;

	l_end = m_lStaticList.end();
	for (l_iter = m_lStaticList.begin(); l_iter != l_end; ++l_iter)
	{
		if (*l_iter == NULL)
		{
			continue;
		}
        m_pStaticSet->Release(*l_iter);
	}
	m_lStaticList.clear();
}

void RuntimeManager::ClearLines(void)
{
	list< Runtime_t* >::iterator l_iter, l_end;

	l_end = m_lRunList.end();
	for (l_iter = m_lRunList.begin(); l_iter != l_end; ++l_iter)
	{
		if (*l_iter == NULL)
		{
			continue;
		}
        m_pLineSet->Release(*l_iter);
	}
	m_lRunList.clear();
}

bool RuntimeManager::IsInited(void)
{
	return m_bInited;
}

int RuntimeManager::Run(void)
{
	int ret;
	if (!IsInited())
	{
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING"runtime error, RuntimeManager not inited.", LOG_FUNC_VALUE);
		return -1;
	}

	ret = RunStaticModules();
	if (ret == -1)
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "run static modules failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	return RunLines();
}

int RuntimeManager::RunStaticModules(void)
{
	int ret;

    if (!m_lStaticList.empty())
	{
		ClearStaticModules();
	}

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to run static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	for (size_t i = 0; i < m_vStaticNames.size(); ++i)
	{
		StaticRuntime_t* pRuntime = m_pStaticSet->Obtain(m_vStaticNames[i]);
		if (pRuntime == NULL)
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "create static modules instance failed, static module name: %s.", m_vStaticNames[i].c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		ret = ModuleCaller::Call(pRuntime);
		if (ret == -1)
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "call static modules instance failed, static module name: %s.", m_vStaticNames[i].c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		pRuntime->m_pCtx->m_oMutex.lock();
		if (pRuntime->m_pCtx->m_nStat != RTS_FINISH)
		{
			pRuntime->m_pCtx->m_oMutex.unlock();
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "call static modules instance failed, not finished, static module name: %s.", m_vStaticNames[i].c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		if (pRuntime->m_pCtx->m_nRetValue != 0)		// 静态模块返回值非零表示运行失败
		{
			pRuntime->m_pCtx->m_nStat = RTS_ERROR;
			pRuntime->m_pCtx->m_oMutex.unlock();
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "call static modules instance failed, return value not expected, static module name: %s, return value: %d.", m_vStaticNames[i].c_str(), pRuntime->m_pCtx->m_nRetValue);
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		else
		{
            pRuntime->m_pCtx->m_nStat = RTS_STATIC;
		}
		pRuntime->m_pCtx->m_oMutex.unlock();
		m_lStaticList.push_back(pRuntime);
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to run static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int RuntimeManager::RunLines(void)
{
	list< Runtime_t* >::iterator l_iter, l_end;
	vector< string >::iterator v_iter;

	if (!m_lRunList.empty())
	{
		ClearLines();
	}

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to run lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	while (1)
	{
		v_iter = m_vLineNames.begin();
		while (v_iter != m_vLineNames.end())
		{
			if (m_lRunList.size() < m_nRunListMaxSize)
			{
				Runtime_t* pRuntime = m_pLineSet->Obtain(*v_iter);
				assert(pRuntime);
				m_lRunList.push_back(pRuntime);
				v_iter = m_vLineNames.erase(v_iter);
				continue;
			}
			else
			{
				break;
			}
			++v_iter;
		}

		l_iter = m_lRunList.begin();
		l_end = m_lRunList.end();
		while (l_iter != l_end)
		{
			if (*l_iter == NULL)
			{
				l_iter = m_lRunList.erase(l_iter);
				continue;
			}

			MainlineFSM(*l_iter);
			++l_iter;
		}

		usleep(100 * 1000);
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to run lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int RuntimeManager::MainlineFSM(Runtime_t* pRuntime)
{
	assert(pRuntime);
	switch (pRuntime->m_nStat)
	{
		case RTS_INIT:
			m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "New line init.");
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		case RTS_WAIT:
			TriggerFSM(pRuntime);
			break;
		case RTS_RUN:
			ModuleFSM(pRuntime);
			break;
		case RTS_FINISH:
			m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "New line finish.");
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			pRuntime->m_oMutex.lock();
			pRuntime->m_nStat = RTS_DESTROY;
			pRuntime->m_oMutex.unlock();
			break;
		case RTS_ERROR:
			m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "New line error.");
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			pRuntime->m_oMutex.lock();
			pRuntime->m_nStat = RTS_DESTROY;
			pRuntime->m_oMutex.unlock();
			break;
		case RTS_DESTROY:
			m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "New line destroy.");
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			FSMLineDestroy(pRuntime);
			break;
		default:
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
			break;
	}

	return 0;
}

int RuntimeManager::TriggerFSM(Runtime_t* pRuntime)
{
	assert(pRuntime);
	switch (pRuntime->m_pTriggerCtx->m_nStat)
	{
		case RTS_INIT:
			FSMTriggerInit(pRuntime);
			m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "New trigger init.");
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			break;
		case RTS_RUN:
			break;
		case RTS_FINISH:
			FSMTriggerFinish(pRuntime);
			m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "New trigger finish.");
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			break;
		case RTS_SYSERROR:
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
			break;
		case RTS_DESTROY:
			break;
		default:
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
			break;
	}

	return 0;
}

int RuntimeManager::ModuleFSM(Runtime_t* pRuntime)
{
	map< LineModule_t*, ModContext_t* >::iterator m_iter, m_end;
	ModContext_t* pCtx;

	assert(pRuntime);
	m_end = pRuntime->m_mapModCtxs.end();
	m_iter = pRuntime->m_mapModCtxs.find(pRuntime->m_pLine->m_pEnd);
	assert(m_iter != m_end);
	pCtx = m_iter->second;
	switch (pCtx->m_nStat)
	{
		case RTS_INIT:
		case RTS_WAIT:
		case RTS_RUN:
		case RTS_EQUAL:
			break;
		case RTS_FINISH:
			FSMEndModuleFinish(pRuntime, pCtx);
			break;
		case RTS_ERROR:
			FSMEndModuleFinish(pRuntime, pCtx);
			break;
		case RTS_SYSERROR:
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
			break;
		case RTS_DESTROY:
			break;
		default:
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
			break;
	}

	for (size_t i = 0; i < pRuntime->m_pLine->m_vModules.size(); ++i)
	{
        LineModule_t* pLineMod = pRuntime->m_pLine->m_vModules[i];
		m_iter = pRuntime->m_mapModCtxs.find(pLineMod);
		assert(m_iter != m_end);
		pCtx = m_iter->second;
        switch (pCtx->m_nStat)
        {
			case RTS_INIT:
				pCtx->m_oMutex.lock();
				pCtx->m_nStat = RTS_WAIT;
				pCtx->m_oMutex.unlock();
				break;
			case RTS_WAIT:
				FSMModuleWait(pRuntime, pLineMod, pCtx);
				break;
			case RTS_RUN:
			case RTS_EQUAL:
			case RTS_FINISH:
			case RTS_ERROR:
				break;
			case RTS_SYSERROR:
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				assert(0);
				break;
			case RTS_DESTROY:
				break;
			default:
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				assert(0);
				break;
        }
	}

	return 0;
}

int RuntimeManager::FSMLineDestroy(Runtime_t* pRuntime)
{
	list< Runtime_t* >::iterator l_iter, l_end;

	assert(pRuntime);
	l_end = m_lRunList.end();
	for (l_iter = m_lRunList.begin(); l_iter != l_end; ++l_iter)
	{
        if (*l_iter == pRuntime)
		{
            *l_iter = NULL;
            m_pLineSet->Release(pRuntime);
            return 0;
		}
	}

	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	assert(0);
	return -1;
}

int RuntimeManager::FSMTriggerInit(Runtime_t* pRuntime)
{
	int ret;
	map< LineModule_t*, ModContext_t* >::iterator m_iter, m_end;
	assert(pRuntime);

	pRuntime->m_oMutex.lock();
	pRuntime->m_nStat = RTS_WAIT;
	pRuntime->m_pTriggerCtx->m_oMutex.lock();
	pRuntime->m_pTriggerCtx->m_nStat = RTS_RUN;
	pRuntime->m_pTriggerCtx->m_oMutex.unlock();
	m_end = pRuntime->m_mapModCtxs.end();
	for (m_iter = pRuntime->m_mapModCtxs.begin(); m_iter != m_end; ++m_iter)
	{
		ModContext_t* pCtx = m_iter->second;
		pCtx->m_oMutex.lock();
		pCtx->m_nStat = RTS_WAIT;
		pCtx->m_oMutex.unlock();
	}
	pRuntime->m_oMutex.unlock();
	ret = ModuleCaller::Call(pRuntime);
	if (ret == -1)
	{
		m_pLogger->LogWrite(FATAL, MODULE_JFR, "call trigger instance failed, trigger name: %s.", pRuntime->m_pLine->m_oTrigger.m_pTrigger->m_sName.c_str());
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		assert(0);
		return -1;
	}

	return 0;
}

int RuntimeManager::FSMTriggerFinish(Runtime_t* pRuntime)
{
    assert(pRuntime);
    pRuntime->m_oMutex.lock();
    pRuntime->m_nStat = RTS_RUN;
    m_vLineNames.push_back(pRuntime->m_pLine->m_sName);
    pRuntime->m_oMutex.unlock();

    return 0;
}

int RuntimeManager::FSMEndModuleFinish(Runtime_t* pRuntime, ModContext_t* pEndCtx)
{
	bool flag = false;
	map< LineModule_t*, ModContext_t* >::iterator m_iter, m_end;

	assert(pRuntime && pEndCtx);
	m_end = pRuntime->m_mapModCtxs.end();
	for (m_iter = pRuntime->m_mapModCtxs.begin(); m_iter != m_end; ++m_iter)
	{
		ModContext_t* pCtx = m_iter->second;
		if (pCtx == pEndCtx)
		{
			continue;
		}
		pCtx->m_oMutex.lock();
		if (pCtx->m_nStat != RTS_RUN)
		{
			pCtx->m_nStat = RTS_DESTROY;
		}
		else
		{
			flag = true;
		}
		pCtx->m_oMutex.unlock();
	}
	if (!flag)
	{
		pRuntime->m_oMutex.lock();
		pEndCtx->m_oMutex.lock();
		pRuntime->m_nStat = pEndCtx->m_nStat;
		pEndCtx->m_nStat = RTS_DESTROY;
		pEndCtx->m_oMutex.unlock();
		pRuntime->m_oMutex.unlock();
	}

	return 0;
}

int RuntimeManager::FSMModuleWait(Runtime_t* pRuntime, LineModule_t* pLineMod, ModContext_t* pCtx)
{
	map< LineModule_t*, int >::iterator m_iter1, m_end1;
	map< int, vector< LineModule_t* > >::iterator m_iter2, m_end2;
	map< LineModule_t*, ModContext_t* >::iterator m_iter3, m_end3;
	set< Module_t* > sProcMods;

    assert(pRuntime && pLineMod && pCtx);
    m_end1 = pRuntime->m_pLine->m_mapModPtr.end();
    m_end2 = pRuntime->m_pLine->m_mapModIds.end();
    m_end3 = pRuntime->m_mapModCtxs.end();
    size_t nReqRstIgnoreTimes = 0, nReqRstFinishTimes = 0, nReqRstErrorTimes = 0;
	for (size_t i = 0; i < pLineMod->m_vRequirement.size(); ++i)
	{
        Module_t* pReqMod = pLineMod->m_vRequirement[i].first;
        if (FindProcedMod(sProcMods, pReqMod))
		{
			continue;
		}
		/// trigger 特殊处理，是trigger，根据返回值，直接设定run or error
		if (pRuntime->m_pLine->m_oTrigger.m_pTrigger == pReqMod)
		{
			pRuntime->m_pTriggerCtx->m_oMutex.lock();
			if (pRuntime->m_pTriggerCtx->m_nStat != RTS_FINISH)
			{
				pRuntime->m_pTriggerCtx->m_oMutex.unlock();
				continue;
			}
			RetValue_t& retVal = pLineMod->m_vRequirement[i].second;
			if (retVal.Euqal(pRuntime->m_pTriggerCtx->m_nRetValue))
			{
				++nReqRstFinishTimes;
			}
			else
			{
				++nReqRstErrorTimes;
			}
			pRuntime->m_pTriggerCtx->m_oMutex.unlock();
			continue;
		}
        LineModule_t* pReqLineMod = FindLineMod(pRuntime->m_pLine->m_vModules, pReqMod);
        assert(pReqLineMod);
        m_iter1 = pRuntime->m_pLine->m_mapModPtr.find(pReqLineMod);
        assert(m_iter1 != m_end1);
        int nReqSetId = m_iter1->second;
        m_iter2 = pRuntime->m_pLine->m_mapModIds.find(nReqSetId);
        assert(m_iter2 != m_end2);
        vector< LineModule_t* >& vEquLineMods = m_iter2->second;
        size_t nEquRstIgnoreTimes = 0, nEquRstFinishTimes = 0, nEquRstErrorTimes = 0;
        for (size_t j = 0; j < vEquLineMods.size(); ++j)
		{
            LineModule_t* pEquLineMod = vEquLineMods[j];
            sProcMods.insert(pEquLineMod->m_pModule);
            m_iter3 = pRuntime->m_mapModCtxs.find(pEquLineMod);
            assert(m_iter3 != m_end3);
            ModContext_t* pEquCtx = m_iter3->second;
            pEquCtx->m_oMutex.lock();
            switch (pEquCtx->m_nStat)
            {
				case RTS_INIT:
				case RTS_WAIT:
				case RTS_RUN:
				case RTS_EQUAL:	// ignore
					++nEquRstIgnoreTimes;
					break;
				case RTS_FINISH:
					{
						const RetValue_t& retVal = FindRequireRetVal(pLineMod, pEquLineMod->m_pModule);
						if (retVal.Euqal(pEquCtx->m_nRetValue))			// finish
						{
							++nEquRstFinishTimes;
						}
						else	// error
						{
							++nEquRstErrorTimes;
						}
					}
					break;
				case RTS_ERROR:	// error
					++nEquRstErrorTimes;
					break;
				case RTS_DESTROY:	// line quit, do not check
					pEquCtx->m_oMutex.unlock();
					return 0;
					break;
				case RTS_SYSERROR:
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					assert(0);
					break;
				default:
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					assert(0);
					break;
            }
            pEquCtx->m_oMutex.unlock();
		}
		if (nEquRstFinishTimes > 0)
		{
			++nReqRstFinishTimes;
		}
		else if (nEquRstErrorTimes == vEquLineMods.size())
		{
			++nReqRstErrorTimes;
		}
		else if (nEquRstIgnoreTimes > 0)
		{
			++nReqRstIgnoreTimes;
		}
	}

	pCtx->m_oMutex.lock();
	if (nReqRstIgnoreTimes > 0)
	{
		pCtx->m_nStat = RTS_WAIT;
	}
	else if (nReqRstErrorTimes > 0)
	{
		pCtx->m_nStat = RTS_ERROR;
	}
	else
	{
		pCtx->m_nStat = RTS_RUN;
		if (ModuleCaller::Call(pRuntime, pLineMod))
		{
			m_pLogger->LogWrite(FATAL, MODULE_JFR, "call module instance failed, module name: %s.", pLineMod->m_pModule->m_sName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		}
	}
	pCtx->m_oMutex.unlock();

    return 0;
}

LineModule_t* RuntimeManager::FindLineMod(const vector< LineModule_t* >& vLineMods, const Module_t* pMod)
{
	assert(pMod);
    for (size_t i = 0; i < vLineMods.size(); ++i)
	{
		if (vLineMods[i]->m_pModule == pMod)
		{
			return vLineMods[i];
		}
	}

	return NULL;
}

const RetValue_t& RuntimeManager::FindRequireRetVal(const LineModule_t* pLineMod, const Module_t* pMod)
{
	assert(pLineMod && pMod);
	for (size_t i = 0; i < pLineMod->m_vRequirement.size(); ++i)
	{
        if (pLineMod->m_vRequirement[i].first == pMod)
		{
			return pLineMod->m_vRequirement[i].second;
		}
	}
	assert(0);
}

bool RuntimeManager::FindProcedMod(const set< Module_t* >& sProcedMods, const Module_t* pMod)
{
	assert(pMod);
	if (sProcedMods.find((Module_t*)pMod) == sProcedMods.end())
	{
        return false;
	}

	return true;
}


CLOSE_NAMESPACE_JFR
