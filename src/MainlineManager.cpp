#include "MainlineManager.h"

OPEN_NAMESPACE_JFR

MainlineManager::MainlineManager(void)
{
	m_bInited = false;
	m_pModule = NULL;
	m_pLogger = NULL;
}

MainlineManager::~MainlineManager(void)
{
	Clear();
}

void MainlineManager::Clear()
{
    ClearStaticModule();
    ClearLine();
}

void MainlineManager::ClearStaticModule()
{
	for (size_t i = 0; i < m_vStaticModules.size(); ++i)
	{
		delete m_vStaticModules[i];
	}
	m_vStaticModules.clear();
}

void MainlineManager::ClearLine()
{
	for (size_t i = 0; i < m_vLines.size(); ++i)
	{
		delete m_vLines[i];
	}
	m_vLines.clear();
}

int MainlineManager::Init(void)
{
	if (m_bInited)
	{
		Clear();
	}
	m_pModule = &ModuleManager::get_mutable_instance();
	m_pLogger = &jfr::LoggerSingleton::get_mutable_instance();
	m_bInited = true;

	return 0;
}

bool MainlineManager::IsInited(void)
{
	return m_bInited;
}

int MainlineManager::LoadMainlines(const vector< ConfigMainLine_t* >& vCfgMainLines)
{
	if (!IsInited())
	{
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING"load mainlines failed, MainlineManager not inited.", LOG_FUNC_VALUE);
		return -1;
	}

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load main lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	ClearLine();
	if (LineSyntaxCheck(vCfgMainLines))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "mainline syntax check failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	if (LoadLines(vCfgMainLines))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "load mainline failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	if (LineLogicCheck())
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "mainline logic check failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load main lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int MainlineManager::LoadStaticModules(const vector< ConfigStaticModule_t* >& vCfgStaticModules)
{
	if (!IsInited())
	{
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING"load static modules failed, MainlineManager not inited.", LOG_FUNC_VALUE);
		return -1;
	}

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	ClearStaticModule();
	if (LoadModules(vCfgStaticModules))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "load static modules failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	if (ModuleLogicCheck())
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module logic check failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

/// 静态语法检查
int MainlineManager::LineSyntaxCheck(const vector< ConfigMainLine_t* >& vCfgMainLines)
{
	// 检查规则：
	// 1. 主线name不为空，且名称唯一
	// 2. 触发器名称不为空
	// 3. 模块名称不为空，且主线内唯一
	// 4. 必要条件名称不为空，且存在(可以是静态模块)
	// 5. 等效条件名称不为空，且存在(不可以是静态模块)
    set< string > sLineNames;
    set< string > sModNames;
    set< string >::iterator iter, end;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to check line syntax.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    for(size_t i = 0; i < vCfgMainLines.size(); ++i)	// check main line and trigger
	{
		assert(vCfgMainLines[i]);
		if (vCfgMainLines[i]->m_sName == "" || vCfgMainLines[i]->m_oTrigger.m_sName == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "line name and trigger name should not be empty, line name: %s, trigger name: %s.", \
														vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_oTrigger.m_sName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		iter = sLineNames.find(vCfgMainLines[i]->m_sName);
		if (iter != sLineNames.end())
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "main line name must be unique, line name: %s.", vCfgMainLines[i]->m_sName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		sLineNames.insert(vCfgMainLines[i]->m_sName);
	}
    for(size_t i = 0; i < vCfgMainLines.size(); ++i)	// check modules
	{
		sModNames.clear();
		for (size_t j = 0; j < vCfgMainLines[i]->m_vModules.size(); ++j)		// module name
		{
			if (vCfgMainLines[i]->m_vModules[j].m_sName == "")
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "module name should not be empty, line name: %s, module name: %s.", \
															vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
			sModNames.insert(vCfgMainLines[i]->m_oTrigger.m_sName);
			iter = sModNames.find(vCfgMainLines[i]->m_vModules[j].m_sName);
			if (iter != sModNames.end())
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "module name must be unique, line name: %s, module name: %s.", \
															vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
			sModNames.insert(vCfgMainLines[i]->m_vModules[j].m_sName);
		}
		end = sModNames.end();
		for (size_t j = 0; j < vCfgMainLines[i]->m_vModules.size(); ++j)		// requirement and equivalent
		{
            for (size_t k = 0; k < vCfgMainLines[i]->m_vModules[j].m_vRequirement.size(); ++k)
			{
				const string& strName = vCfgMainLines[i]->m_vModules[j].m_vRequirement[k].first;
				if (strName == "")
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "requirement module name should not be empty, line name: %s, module name: %s, req module name: %s.", \
																vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str(), strName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
				if (sModNames.find(strName) == end)
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "requirement module name not defined in line modules, line name: %s, module name: %s, req module name: %s.", \
																vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str(), strName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
			}
			for (size_t k = 0; k < vCfgMainLines[i]->m_vModules[j].m_vEquivalent.size(); ++k)
			{
				if (vCfgMainLines[i]->m_vModules[j].m_vEquivalent[k] == "")
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "equivalent module name should not be empty, line name: %s, module name: %s, equ module name: %s.", \
																vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_vEquivalent[k].c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
				if (sModNames.find(vCfgMainLines[i]->m_vModules[j].m_vEquivalent[k]) == end)
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "equivalent module name not defined in line modules, line name: %s, module name: %s, equ module name: %s.", \
																vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_vEquivalent[k].c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
				if (vCfgMainLines[i]->m_vModules[j].m_vEquivalent[k] == vCfgMainLines[i]->m_oTrigger.m_sName)
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "equivalent module should not be trigger, line name: %s, module name: %s, equ module name: %s.", \
																vCfgMainLines[i]->m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_sName.c_str(), vCfgMainLines[i]->m_vModules[j].m_vEquivalent[k].c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
			}
		}
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to check line syntax.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int MainlineManager::LoadLineTrigger(const ConfigTrigger_t& cfgTrigger, Line_t* pLine)
{
	assert(pLine);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load line triggers.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    pLine->m_oTrigger.m_pTrigger = (Trigger_t*)m_pModule->GetTrigger(cfgTrigger.m_sName);
    if (pLine->m_oTrigger.m_pTrigger == NULL)
    {
    	m_pLogger->LogWrite(ERROR, MODULE_JFR, "not found trigger: %s, line name: %s.", cfgTrigger.m_sName.c_str(), pLine->m_sName.c_str());
    	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return -1;
    }
	if (ParseArgs(cfgTrigger.m_sInput, pLine->m_vArgs, pLine->m_oTrigger.m_vInputArgs))
	{
    	m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse input args failed, line name: %s, trigger name: %s, args: %s.", \
													pLine->m_sName.c_str(), cfgTrigger.m_sName.c_str(), cfgTrigger.m_sInput.c_str());
    	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	if (ParseArgs(cfgTrigger.m_sOutput, pLine->m_vArgs, pLine->m_oTrigger.m_vOutputArgs))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse output args failed, line name: %s, trigger name: %s, args: %s.", \
													pLine->m_sName.c_str(), cfgTrigger.m_sName.c_str(), cfgTrigger.m_sOutput.c_str());
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load line triggers.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

    return 0;
}

int MainlineManager::LoadLineModule(const ConfigModule_t& cfgModule, Line_t* pLine, LineModule_t* pMod)
{
	assert(pLine && pMod);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load line modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    pMod->m_pModule = (Module_t*)m_pModule->GetModule(cfgModule.m_sName);
    if (pMod->m_pModule == NULL)
    {
    	m_pLogger->LogWrite(ERROR, MODULE_JFR, "not found module: %s, line name: %s.", cfgModule.m_sName.c_str(), pLine->m_sName.c_str());
    	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    	return -1;
    }
	if (ParseArgs(cfgModule.m_sInput, pLine->m_vArgs, pMod->m_vInputArgs))
	{
    	m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse input args failed, line name: %s, module name: %s, args: %s.", \
													pLine->m_sName.c_str(), cfgModule.m_sName.c_str(), cfgModule.m_sInput.c_str());
    	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	if (ParseArgs(cfgModule.m_sOutput, pLine->m_vArgs, pMod->m_vOutputArgs))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse output args failed, line name: %s, module name: %s, args: %s.", \
													pLine->m_sName.c_str(), cfgModule.m_sName.c_str(), cfgModule.m_sOutput.c_str());
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
    for (size_t k = 0; k < cfgModule.m_vRequirement.size(); ++k)
    {
        const string& sName = cfgModule.m_vRequirement[k].first;
        Module_t* pTmp;
        if ((pTmp = (Module_t*)m_pModule->GetModule(sName)) == NULL && (pTmp = (StaticModule_t*)m_pModule->GetStaticModule(sName)) == NULL && (pTmp = (Trigger_t*)m_pModule->GetTrigger(sName)) == NULL)		// !module or !static module or !trigger
        {
        	m_pLogger->LogWrite(ERROR, MODULE_JFR, "not found requirement module, line name: %s, module name: %s, req module: %s.", \
														pLine->m_sName.c_str(), cfgModule.m_sName.c_str(), sName.c_str());
        	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
        }
        pMod->m_vRequirement.push_back(make_pair(pTmp, cfgModule.m_vRequirement[k].second));
    }
    for (size_t k = 0; k < cfgModule.m_vEquivalent.size(); ++k)
    {
        Module_t* pTmp = (Module_t*)m_pModule->GetModule(cfgModule.m_vEquivalent[k]);
        if (pTmp == NULL)
        {
        	m_pLogger->LogWrite(ERROR, MODULE_JFR, "not found equivalent module, line name: %s, module name: %s, equ module: %s.", \
														pLine->m_sName.c_str(), cfgModule.m_sName.c_str(), cfgModule.m_vEquivalent[k].c_str());
        	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
        }
        pMod->m_vEquivalent.push_back(pTmp);
    }
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load line modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

    return 0;
}

int MainlineManager::LoadLineModuleId(Line_t* pLine, LineModule_t* pMod, int* maxId)
{
	map< Module_t*, LineModule_t* > mapModules;
	map< Module_t*, LineModule_t* >::iterator iter, end;
	map< int, vector< LineModule_t* > >::iterator iter_ids;
	map< LineModule_t*, int >::iterator iter_ptr;
	int nCurrModId = -1;

	assert(pLine && pMod && maxId);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load line module id.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	for(size_t k = 0; k < pLine->m_vModules.size(); ++k)
	{
		mapModules.insert(make_pair(pLine->m_vModules[k]->m_pModule, pLine->m_vModules[k]));
	}
	end = mapModules.end();
    for (size_t k = 0; k < pMod->m_vEquivalent.size(); ++k)
    {
    	iter = mapModules.find(pMod->m_vEquivalent[k]);
    	if (iter == end)
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "equivalent module not defined in line module, line name: %s, module name: %s, equ module name: %s.", \
														pLine->m_sName.c_str(), pMod->m_pModule->m_sName.c_str(), pMod->m_vEquivalent[k]->m_sName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
        iter_ptr = pLine->m_mapModPtr.find(iter->second);
        if (iter_ptr != pLine->m_mapModPtr.end())
        {
            nCurrModId = iter_ptr->second;
            break;
        }
    }
    if (nCurrModId == -1)
    {
        nCurrModId = (*maxId)++;
    }
    iter_ids = pLine->m_mapModIds.find(nCurrModId);
    if (iter_ids == pLine->m_mapModIds.end())
    {
        vector< LineModule_t* > vTmp;
        vTmp.push_back(pMod);
        pLine->m_mapModIds.insert(make_pair(nCurrModId, vTmp));
    }
    else
    {
        iter_ids->second.push_back(pMod);
    }
    pLine->m_mapModPtr.insert(make_pair(pMod, nCurrModId));
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load line module id.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

    return 0;
}

int MainlineManager::LoadLines(const vector< ConfigMainLine_t* >& vCfgMainLines)
{
	bool flag;
	Line_t* pLine;
	LineModule_t* pMod;
	int nMaxModId;

	flag = false;
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	for (size_t i = 0; i < vCfgMainLines.size(); ++i)
	{
		assert(vCfgMainLines[i]);
		if (!flag)
		{
			pLine = new Line_t;
			flag = true;
		}

		nMaxModId = 0;
		pLine->m_sName = vCfgMainLines[i]->m_sName;
		pLine->m_sDesc = vCfgMainLines[i]->m_sDesc;
		if (LoadLineTrigger(vCfgMainLines[i]->m_oTrigger, pLine))
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "load line trigger failed, line name: %s.", pLine->m_sName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}

		for (size_t j = 0; j < vCfgMainLines[i]->m_vModules.size(); ++j)
		{
			pMod = new LineModule_t;
			if (LoadLineModule(vCfgMainLines[i]->m_vModules[j], pLine, pMod))
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "load line modules failed, line name: %s.", pLine->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}

			pLine->m_vModules.push_back(pMod);
		}
		for (size_t j = 0; j < pLine->m_vModules.size(); ++j)
		{
			pMod = pLine->m_vModules[j];
			if (LoadLineModuleId(pLine, pMod, &nMaxModId))
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "load line module id failed, line name: %s.", pLine->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
		}

		m_vLines.push_back(pLine);
		flag = false;
	}

	if (flag)
	{
		delete pLine;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

/// 主线流程逻辑合法性检查
int MainlineManager::LineLogicCheck(void)
{
	// 检查规则：
	// 1. 触发器、模块必须存在 (checked)
	// 2. 各模块的必要模块、等效模块均存在
	// 3. 结束条件只能有一个
	// 4. 出参只能是变量，进程的出参只能有一个，出参不能与静态模块名称相同
	// 5. 等效模块不可以是静态模块 (checked)
	// 6. 等效条件集合必须且只能是下一级的必要条件
	// 7. 一个模块只能定义一次，一个模块只能运行一次
	// 8. 不能有闭环 (same as rule 7)
	// 9. 模块参数满足依赖关系，后置模块的入参必须是前置模块的出参或固定字符串
	// 10.必要条件如果是触发器，则不能包含其他模块
	// 11.等效模块必须是相互的

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to line logic check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	if (LineLogicCheck_Rule2() || LineLogicCheck_Rule3() || LineLogicCheck_Rule4() || LineLogicCheck_Rule6() || LineLogicCheck_Rule10() || LineLogicCheck_Rule11())
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "mainline logic check failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to line logic check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int MainlineManager::LineLogicCheck_Rule2(void)
{
    for (size_t i = 0; i < m_vLines.size(); ++i)
	{
        set< Module_t* > sModules;

        sModules.insert(m_vLines[i]->m_oTrigger.m_pTrigger);
        for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			sModules.insert(m_vLines[i]->m_vModules[j]->m_pModule);
		}
		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
            for (size_t k = 0; k < m_vLines[i]->m_vModules[j]->m_vRequirement.size(); ++k)
			{
				Module_t* pReqMod = m_vLines[i]->m_vModules[j]->m_vRequirement[k].first;
				if (sModules.find(pReqMod) == sModules.end())
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "requirement module not defined in line modules, line name: %s, module name: %s, req module name: %s.", \
																m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str(), pReqMod->m_sName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                    return -1;
				}
			}
			for (size_t k = 0; k < m_vLines[i]->m_vModules[j]->m_vEquivalent.size(); ++k)
			{
				Module_t* pEquMod = m_vLines[i]->m_vModules[j]->m_vEquivalent[k];
				if (sModules.find(pEquMod) == sModules.end())
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "equivalent module not defined in line modules, line name: %s, module name: %s, equ module name: %s.", \
																m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str(), pEquMod->m_sName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                    return -1;
				}
			}
		}
	}

	return 0;
}

int MainlineManager::LineLogicCheck_Rule3(void)
{
    for (size_t i = 0; i < m_vLines.size(); ++i)
	{
        set< Module_t* > sModules;

        for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			sModules.insert(m_vLines[i]->m_vModules[j]->m_pModule);
		}
		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
            for (size_t k = 0; k < m_vLines[i]->m_vModules[j]->m_vRequirement.size(); ++k)
			{
				sModules.erase(m_vLines[i]->m_vModules[j]->m_vRequirement[k].first);
			}
		}

		if (sModules.size() != 1)
		{
			if (sModules.size() == 0)
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "not found end condition module, line name: %s.", m_vLines[i]->m_sName.c_str());
			}
			else
			{
				string strEnd;
				for (set< Module_t* >::iterator iter = sModules.begin(); iter != sModules.end(); ++iter)
				{
                    strEnd += (*iter)->m_sName + ", ";
				}
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "found more than one end condition modules, line name: %s, found end modules: %s.", m_vLines[i]->m_sName.c_str(), strEnd.c_str());
			}
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
        for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			if (*sModules.begin() == m_vLines[i]->m_vModules[j]->m_pModule)
			{
				m_vLines[i]->m_pEnd = m_vLines[i]->m_vModules[j];
				break;
			}
		}
	}

	return 0;
}

int MainlineManager::LineLogicCheck_Rule4(void)
{
    set< string > sStaticOutArgs;

    for (size_t i = 0; i < m_vStaticModules.size(); ++i)
	{
        for (size_t j = 0; j < m_vStaticModules[i]->m_vArgs.size(); ++j)
		{
			if (m_vStaticModules[i]->m_vArgs[j]->m_nType == AT_VARIABLE)
			{
				sStaticOutArgs.insert(m_vStaticModules[i]->m_vArgs[j]->m_sName);
			}
		}
	}
	for (size_t i = 0; i < m_vLines.size(); ++i)
	{
		for (size_t j = 0; j < m_vLines[i]->m_oTrigger.m_vOutputArgs.size(); ++j)
		{
			ModArg_t* pArg = m_vLines[i]->m_oTrigger.m_vOutputArgs[j];
			if (pArg->m_nType != AT_VARIABLE)
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger output arg type wrong, should be 'VARIABLE', line name: %s, trigger name: %s, output arg: %s.", \
															m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_oTrigger.m_pTrigger->m_sName.c_str(), pArg->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                return -1;
			}
			if (sStaticOutArgs.find(pArg->m_sName) != sStaticOutArgs.end())
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger output arg can not be same with the static module's, line name: %s, trigger name: %s, output arg: %s.", \
															m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_oTrigger.m_pTrigger->m_sName.c_str(), pArg->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
		}
		if (IS_PRO(m_vLines[i]->m_oTrigger.m_pTrigger->m_nType) && m_vLines[i]->m_oTrigger.m_vOutputArgs.size() > 1)
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger(type pro) can not have more than one output arg, line name: %s, trigger name: %s, output args size: %d.", \
														m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_oTrigger.m_pTrigger->m_sName.c_str(), m_vLines[i]->m_oTrigger.m_vOutputArgs.size());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}

		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			for (size_t k = 0; k < m_vLines[i]->m_vModules[j]->m_vOutputArgs.size(); ++k)
			{
				ModArg_t* pArg = m_vLines[i]->m_vModules[j]->m_vOutputArgs[k];
				if (pArg->m_nType != AT_VARIABLE)
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "module output arg type wrong, should be 'VARIABLE', line name: %s, module name: %s, output arg: %s.", \
																m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str(), pArg->m_sName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
				if (sStaticOutArgs.find(pArg->m_sName) != sStaticOutArgs.end())
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "module output arg can not be same with the static module's, line name: %s, module name: %s, output arg: %s.", \
																m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str(), pArg->m_sName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					return -1;
				}
			}
			if (IS_PRO(m_vLines[i]->m_vModules[j]->m_pModule->m_nType) && m_vLines[i]->m_vModules[j]->m_vOutputArgs.size() > 1)
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "module(type pro) can not have more than one output arg, line name: %s, module name: %s, output args size: %d.", \
															m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_vOutputArgs.size());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                return -1;
			}
		}
	}

	return 0;
}

int MainlineManager::LineLogicCheck_Rule6(void)
{
    for (size_t i = 0; i < m_vLines.size(); ++i)
	{
		map< Module_t*, LineModule_t* > mapModules;
		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			mapModules.insert(make_pair(m_vLines[i]->m_vModules[j]->m_pModule, m_vLines[i]->m_vModules[j]));
		}
		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			if (CheckRequireSet(m_vLines[i], m_vLines[i]->m_vModules[j], mapModules))
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "check module require set failed, line name: %s, module name: %s.", \
															m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                return -1;
			}
		}
	}

	return 0;
}

int MainlineManager::LineLogicCheck_Rule10(void)
{
    for (size_t i = 0; i < m_vLines.size(); ++i)
	{
		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			for (size_t k = 0; k < m_vLines[i]->m_vModules[j]->m_vRequirement.size(); ++k)
			{
                Module_t* pReqMod = m_vLines[i]->m_vModules[j]->m_vRequirement[k].first;
                if (pReqMod == m_vLines[i]->m_oTrigger.m_pTrigger && m_vLines[i]->m_vModules[j]->m_vRequirement.size() != 1)
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "requirement module can not have both trigger and regular modules, line name: %s, module name: %s.", \
																m_vLines[i]->m_sName.c_str(), m_vLines[i]->m_vModules[j]->m_pModule->m_sName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                    return -1;
				}
			}
		}
	}

	return 0;
}

int MainlineManager::LineLogicCheck_Rule11(void)
{
	bool flag;
    for (size_t i = 0; i < m_vLines.size(); ++i)
	{
		for (size_t j = 0; j < m_vLines[i]->m_vModules.size(); ++j)
		{
			for (size_t k = 0; k < m_vLines[i]->m_vModules[j]->m_vEquivalent.size(); ++k)
			{
				Module_t* pMod = m_vLines[i]->m_vModules[j]->m_pModule;
                Module_t* pEquMod = m_vLines[i]->m_vModules[j]->m_vEquivalent[k];
                for (size_t s = 0; s < m_vLines[i]->m_vModules.size(); ++s)
				{
                    if (m_vLines[i]->m_vModules[s]->m_pModule != pEquMod)
					{
						continue;
					}
					flag = false;
					for (size_t t = 0; t < m_vLines[i]->m_vModules[s]->m_vEquivalent.size(); ++t)
					{
						if (m_vLines[i]->m_vModules[s]->m_vEquivalent[t] == pMod)
						{
							flag = true;
							break;
						}
					}
					break;
				}
				if (!flag)
				{
					m_pLogger->LogWrite(ERROR, MODULE_JFR, "Equivalent module must be mutual, line name: %s, module name: %s, equ module name: %s.", \
																m_vLines[i]->m_sName.c_str(), pMod->m_sName.c_str(), pEquMod->m_sName.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                    return -1;
				}
			}
		}
	}

	return 0;
}

int MainlineManager::CheckRequireSet(const Line_t* pLine, const LineModule_t* pModule, const map< Module_t*, LineModule_t* >& mapModules)
{
	map< Module_t*, LineModule_t* >::const_iterator iter, end;
	map< LineModule_t*, int >::const_iterator iter1, end1;
	map< int, vector< LineModule_t* > >::const_iterator iter2, end2;
	vector< Module_t* > vAllModuleSets;
	Module_t* pMod;
	LineModule_t* pLineMod;
	int mid;
	bool flag;

	// 根据每个必要条件，找到其关联的等效条件；
	// 如果关联后的模块都在其必要条件内，则符合要求
	assert(pLine && pModule);
	end = mapModules.end();
	end1 = pLine->m_mapModPtr.end();
	end2 = pLine->m_mapModIds.end();
	for (size_t i = 0; i < pModule->m_vRequirement.size(); ++i)
	{
		pMod = pModule->m_vRequirement[i].first;
		iter = mapModules.find(pMod);
		if (iter == end)
		{
			if (pMod == pLine->m_oTrigger.m_pTrigger)	// 触发器跳过
			{
				continue;
			}
			return -1;
		}
		pLineMod = iter->second;
		iter1 = pLine->m_mapModPtr.find(pLineMod);
		if (iter1 == end1)
		{
			return -1;
		}
		mid = iter1->second;
		iter2 = pLine->m_mapModIds.find(mid);
		if (iter2 == end2)
		{
			return -1;
		}
		const vector< LineModule_t* >& vTmp = iter2->second;
		for (size_t j = 0; j < vTmp.size(); ++j)
		{
			vAllModuleSets.push_back(vTmp[j]->m_pModule);
		}
	}
	for (size_t i = 0; i < vAllModuleSets.size(); ++i)
	{
		flag = false;
		for (size_t j = 0; j < pModule->m_vRequirement.size(); ++j)
		{
			if (vAllModuleSets[i] == pModule->m_vRequirement[j].first)
			{
				flag = true;
				break;
			}
		}
		if (!flag)
		{
			return -1;
		}
	}

	return 0;
}

int MainlineManager::LoadModules(const vector< ConfigStaticModule_t* >& vCfgStaticModules)
{
    bool flag;
    LineStaticModule_t* pMod;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    flag = false;
    for (size_t i = 0; i < vCfgStaticModules.size(); ++i)
    {
        assert(vCfgStaticModules[i]);
        if (!flag)
        {
            pMod = new LineStaticModule_t;
            flag = true;
        }

        pMod->m_sName = vCfgStaticModules[i]->m_sName;
        pMod->m_oModule.m_pModule = (StaticModule_t*)m_pModule->GetStaticModule(vCfgStaticModules[i]->m_sName);
        if (pMod->m_oModule.m_pModule == NULL)
        {
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "not found static module: %s.", vCfgStaticModules[i]->m_sName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        	return -1;
        }
		if (ParseArgs(vCfgStaticModules[i]->m_sInput, pMod->m_vArgs, pMod->m_oModule.m_vInputArgs))
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse input args failed, static module name: %s, args: %s.", \
														vCfgStaticModules[i]->m_sName.c_str(), vCfgStaticModules[i]->m_sInput.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		if (ParseArgs(vCfgStaticModules[i]->m_sOutput, pMod->m_vArgs, pMod->m_oModule.m_vOutputArgs))
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse output args failed, static module name: %s, args: %s.", \
														vCfgStaticModules[i]->m_sName.c_str(), vCfgStaticModules[i]->m_sOutput.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}

        m_vStaticModules.push_back(pMod);
        flag = false;
    }

    if (flag)
    {
        delete pMod;
    }
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

    return 0;
}

/// 静态模块逻辑合法性检查
int MainlineManager::ModuleLogicCheck(void)
{
	// 检查规则：
	// 1. 入参只能是字符串
	// 2. 出参只能是变量
	// 3. 模块如果是进程类型，出参只能有一个
	// 4. 静态模块间出参名不能相同

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to module logic check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	set< ModArg_t* > sAllMods;
	for (size_t i = 0; i < m_vStaticModules.size(); ++i)
	{
		for (size_t j = 0; j < m_vStaticModules[i]->m_oModule.m_vInputArgs.size(); ++j)
		{
			if (m_vStaticModules[i]->m_oModule.m_vInputArgs[j]->m_nType != AT_STRING)
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module input args type wrong, should be 'STRING', static module name: %s, input args: %s.", \
															m_vStaticModules[i]->m_oModule.m_pModule->m_sName.c_str(), m_vStaticModules[i]->m_oModule.m_vInputArgs[j]->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
		}
		if (IS_PRO(m_vStaticModules[i]->m_oModule.m_pModule->m_nType) && m_vStaticModules[i]->m_oModule.m_vOutputArgs.size() > 1)
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module(type pro) can not have more than one output arg, static module name: %s, output args size: %d.", \
														m_vStaticModules[i]->m_oModule.m_pModule->m_sName.c_str(), m_vStaticModules[i]->m_oModule.m_vOutputArgs.size());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		for (size_t j = 0; j < m_vStaticModules[i]->m_oModule.m_vOutputArgs.size(); ++j)
		{
			if (m_vStaticModules[i]->m_oModule.m_vOutputArgs[j]->m_nType != AT_VARIABLE)
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module output args type wrong, should be 'VARIABLE', static module name: %s, output args: %s.", \
															m_vStaticModules[i]->m_oModule.m_pModule->m_sName.c_str(), m_vStaticModules[i]->m_oModule.m_vOutputArgs[j]->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
			if (sAllMods.find(m_vStaticModules[i]->m_oModule.m_vOutputArgs[j]) != sAllMods.end())
			{
				m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module output args mixed, static module name: %s, output args: %s.", \
															m_vStaticModules[i]->m_oModule.m_pModule->m_sName.c_str(), m_vStaticModules[i]->m_oModule.m_vOutputArgs[j]->m_sName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                return -1;
			}
			sAllMods.insert(m_vStaticModules[i]->m_oModule.m_vOutputArgs[j]);
		}
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to module logic check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int MainlineManager::ParseArgs(const string& args, vector< ModArg_t* >& vAllArgs, vector< ModArg_t* >& vModArgs)
{
	const char *pBegin, *pEnd, *pArgHead, *pArgTail, *p;
	bool flag;
	string strArgs = args;

	TrimString(strArgs);
	pBegin = strArgs.c_str();
	pEnd = strArgs.c_str() + strArgs.length() + 1;
	p = pBegin;
	do
	{
		pArgHead = p;
		pArgTail = NULL;
		flag = false;
		do
		{
			if (*p == '\"')
			{
				flag = !flag;
			}
			if (flag)
			{
				continue;
			}
			if (*p == ',')
			{
				pArgTail = p;
				break;
			}
		} while (++p < pEnd);
		if (p == pEnd)
		{
			pArgTail = pEnd;
		}
		if (pArgTail)
		{
            string tmp = string(pArgHead, pArgTail - pArgHead);
            if (tmp[0] != '\0')
			{
				if (TrimQuote(tmp))
				{
					PushArg(tmp, vAllArgs, vModArgs, AT_STRING);
				}
				else
				{
					PushArg(tmp, vAllArgs, vModArgs);
				}
			}
			if (++p >= pEnd)
			{
				break;
			}
		}
	} while (1);

	return 0;
}

/// 去除空格、'\r'、'\n'、'\t'、不可见字符；引号之间的上述字符不处理
int MainlineManager::TrimString(string& str)
{
	bool flag = true;
	string tmp = str;
	str = "";
	const char* p = tmp.c_str();

	do
	{
		if (flag)
            if (*p >= 0 && !isgraph(*p))	// 不可见字符、中文不需判断
                continue;
		str.append(1, *p);
		if (*p == '\"')
			flag = !flag;
	} while (*++p != '\0');

    return 0;
}

/// 去除双引号
int MainlineManager::TrimQuote(string& str)
{
	string tmp = str;
	str = "";
	const char* p = tmp.c_str();
	bool flag = false;

	do
	{
        if (*p != '\"')
			str.append(1, *p);
		else
			flag = true;
	} while (*++p != '\0');

	return flag ? 1 : 0;
}

ModArg_t* MainlineManager::FindArg(const string& strArg, const vector< ModArg_t* >& vAllArgs)
{
	for(size_t i = 0; i < vAllArgs.size(); ++i)
	{
		assert(vAllArgs[i]);
		if (vAllArgs[i]->m_sName == strArg)
		{
			return vAllArgs[i];
		}
	}

	return NULL;
}

int MainlineManager::PushArg(const string& strArg, vector< ModArg_t* >& vAllArgs, vector< ModArg_t* >& vModArgs, unsigned int type)
{
	ModArg_t *pArg = NULL;
	assert(type == AT_INVALID || type == AT_STRING || type == AT_VARIABLE);

    for (size_t i = 0; i < m_vStaticModules.size(); ++i)
    {
        assert(m_vStaticModules[i]);
        pArg = FindArg(strArg, m_vStaticModules[i]->m_vArgs);
        if (pArg)
		{
			break;
		}
    }
    if (!pArg)
	{
		pArg = FindArg(strArg, vAllArgs);
	}
	if (!pArg)
	{
        pArg = new ModArg_t;
        pArg->m_sName = strArg;
        if (type != AT_INVALID)
        {
            pArg->m_nType = type;
        }
        else if (strArg[0] == '$')
        {
            pArg->m_nType = AT_VARIABLE;
        }
        else
        {
            pArg->m_nType = AT_STRING;
        }
        vAllArgs.push_back(pArg);
	}
	vModArgs.push_back(pArg);

    return 0;
}

const vector< Line_t* >& MainlineManager::GetMainLines(void) const
{
    return m_vLines;
}

const vector< LineStaticModule_t* >& MainlineManager::GetStaticModules(void) const
{
	return m_vStaticModules;
}


CLOSE_NAMESPACE_JFR
