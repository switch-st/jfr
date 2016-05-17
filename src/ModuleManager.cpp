#include "ModuleManager.h"


OPEN_NAMESPACE_JFR


ModuleManager::ModuleManager(void)
{
	m_bInited = false;
	m_pLogger = NULL;
}

ModuleManager::~ModuleManager(void)
{
	if (m_bInited)
	{
		Clear();
	}
}

int ModuleManager::Init(void)
{
	if (m_bInited)
	{
		Clear();
	}
	m_pLogger = &jfr::LoggerSingleton::get_mutable_instance();
    m_bInited = true;

    return 0;
}

bool ModuleManager::IsInited(void)
{
	return m_bInited;
}

void ModuleManager::Clear(void)
{
	map< string, Module_t* >::iterator iter, end;

	m_mapModule.clear();
	m_mapTrigger.clear();
	m_mapStaticModule.clear();
	end = m_mapAllModule.end();
	for (iter = m_mapAllModule.begin(); iter != end; ++iter)
	{
		assert(iter->second);
		if (IS_SO(iter->second->m_nType))
		{
			dlclose(iter->second->m_pHandle);
		}
		delete iter->second;
	}
	m_mapAllModule.clear();
}

int ModuleManager::LoadAllModules(const vector< ConfigModule_t* >& vCfgModules, \
								const vector< ConfigTrigger_t* >& vCfgTriggers, \
								const vector< ConfigStaticModule_t* >& vCfgStaticModules)
{
	if (!IsInited())
	{
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING"load modules failed, ModuleManager not inited.", LOG_FUNC_VALUE);
		return -1;
	}

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load all modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	Clear();
	if (SyntaxCheck(vCfgModules, vCfgTriggers, vCfgStaticModules))
	{
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "module syntax check failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	if (LoadModules(vCfgModules) || LoadTriggers(vCfgTriggers) || LoadStaticModules(vCfgStaticModules))
	{
		Clear();
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "load modules failed.");
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load all modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

/// 静态语法检查
int ModuleManager::SyntaxCheck(const vector< ConfigModule_t* >& vCfgModules, \
							const vector< ConfigTrigger_t* >& vCfgTriggers, \
							const vector< ConfigStaticModule_t* >& vCfgStaticModules)
{
	// 检查规则：
	// 1. name、type、main、file不为空
	// 2. type -> so or pro
	// 3. name唯一
	set< string > sNames;
	set< string >::iterator iter;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to module syntax check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	for (size_t i = 0; i < vCfgModules.size(); ++i)
	{
		assert(vCfgModules[i]);
		if (vCfgModules[i]->m_sName == "" || vCfgModules[i]->m_sType == "" || vCfgModules[i]->m_sFileName == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "module syntax check failed, empty attribute, name: %s, type: %s, main: %s, filename: %s.",
														vCfgModules[i]->m_sName.c_str(), \
														vCfgModules[i]->m_sType.c_str(), \
														vCfgModules[i]->m_sMain.c_str(), \
														vCfgModules[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		if (vCfgModules[i]->m_sType != "so" && vCfgModules[i]->m_sType != "pro")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "module syntax check failed, wrong <type>, name: %s, type: %s, main: %s, filename: %s.",
														vCfgModules[i]->m_sName.c_str(), \
														vCfgModules[i]->m_sType.c_str(), \
														vCfgModules[i]->m_sMain.c_str(), \
														vCfgModules[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		if (vCfgModules[i]->m_sType == "so" && vCfgModules[i]->m_sMain == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "module syntax check failed, type so module not specified <main>, name: %s, type: %s, main: %s, filename: %s.",
														vCfgModules[i]->m_sName.c_str(), \
														vCfgModules[i]->m_sType.c_str(), \
														vCfgModules[i]->m_sMain.c_str(), \
														vCfgModules[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		iter = sNames.find(vCfgModules[i]->m_sName);
		if (iter != sNames.end())
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "module syntax check failed, duplicate module name, name: %s, type: %s, main: %s, filename: %s.",
														vCfgModules[i]->m_sName.c_str(), \
														vCfgModules[i]->m_sType.c_str(), \
														vCfgModules[i]->m_sMain.c_str(), \
														vCfgModules[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		sNames.insert(vCfgModules[i]->m_sName);
	}
	for (size_t i = 0; i < vCfgTriggers.size(); ++i)
	{
		assert(vCfgTriggers[i]);
		if (vCfgTriggers[i]->m_sName == "" || vCfgTriggers[i]->m_sType == "" || vCfgTriggers[i]->m_sFileName == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger syntax check failed, empty attribute, name: %s, type: %s, main: %s, filename: %s.",
														vCfgTriggers[i]->m_sName.c_str(), \
														vCfgTriggers[i]->m_sType.c_str(), \
														vCfgTriggers[i]->m_sMain.c_str(), \
														vCfgTriggers[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		if (vCfgTriggers[i]->m_sType != "so" && vCfgTriggers[i]->m_sType != "pro")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger syntax check failed, wrong <type>, name: %s, type: %s, main: %s, filename: %s.",
														vCfgTriggers[i]->m_sName.c_str(), \
														vCfgTriggers[i]->m_sType.c_str(), \
														vCfgTriggers[i]->m_sMain.c_str(), \
														vCfgTriggers[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		if (vCfgTriggers[i]->m_sType == "so" && vCfgTriggers[i]->m_sMain == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger syntax check failed, type so trigger not specified <main>, name: %s, type: %s, main: %s, filename: %s.",
														vCfgTriggers[i]->m_sName.c_str(), \
														vCfgTriggers[i]->m_sType.c_str(), \
														vCfgTriggers[i]->m_sMain.c_str(), \
														vCfgTriggers[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		iter = sNames.find(vCfgTriggers[i]->m_sName);
		if (iter != sNames.end())
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "trigger syntax check failed, duplicate trigger name, name: %s, type: %s, main: %s, filename: %s.",
														vCfgTriggers[i]->m_sName.c_str(), \
														vCfgTriggers[i]->m_sType.c_str(), \
														vCfgTriggers[i]->m_sMain.c_str(), \
														vCfgTriggers[i]->m_sFileName.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		sNames.insert(vCfgTriggers[i]->m_sName);
	}
	for (size_t i = 0; i < vCfgStaticModules.size(); ++i)
	{
		assert(vCfgStaticModules[i]);
		if (vCfgStaticModules[i]->m_sName == "" || vCfgStaticModules[i]->m_sType == "" || vCfgStaticModules[i]->m_sFileName == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module syntax check failed, empty attribute, name: %s, type: %s, main: %s, filename: %s, input args: %s, output args: %s.",
														vCfgStaticModules[i]->m_sName.c_str(), \
														vCfgStaticModules[i]->m_sType.c_str(), \
														vCfgStaticModules[i]->m_sMain.c_str(), \
														vCfgStaticModules[i]->m_sFileName.c_str(), \
														vCfgStaticModules[i]->m_sInput.c_str(), \
														vCfgStaticModules[i]->m_sOutput.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		if (vCfgStaticModules[i]->m_sType != "so" && vCfgStaticModules[i]->m_sType != "pro")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module syntax check failed, wrong <type>, name: %s, type: %s, main: %s, filename: %s, input args: %s, output args: %s.",
														vCfgStaticModules[i]->m_sName.c_str(), \
														vCfgStaticModules[i]->m_sType.c_str(), \
														vCfgStaticModules[i]->m_sMain.c_str(), \
														vCfgStaticModules[i]->m_sFileName.c_str(), \
														vCfgStaticModules[i]->m_sInput.c_str(), \
														vCfgStaticModules[i]->m_sOutput.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		if (vCfgStaticModules[i]->m_sType == "so" && vCfgStaticModules[i]->m_sMain == "")
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module syntax check failed, type so static module not specified <main>, name: %s, type: %s, main: %s, filename: %s, input args: %s, output args: %s.",
														vCfgStaticModules[i]->m_sName.c_str(), \
														vCfgStaticModules[i]->m_sType.c_str(), \
														vCfgStaticModules[i]->m_sMain.c_str(), \
														vCfgStaticModules[i]->m_sFileName.c_str(), \
														vCfgStaticModules[i]->m_sInput.c_str(), \
														vCfgStaticModules[i]->m_sOutput.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		iter = sNames.find(vCfgStaticModules[i]->m_sName);
		if (iter != sNames.end())
		{
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "static module syntax check failed, duplicate static module name, name: %s, type: %s, main: %s, filename: %s, input args: %s, output args: %s.",
														vCfgStaticModules[i]->m_sName.c_str(), \
														vCfgStaticModules[i]->m_sType.c_str(), \
														vCfgStaticModules[i]->m_sMain.c_str(), \
														vCfgStaticModules[i]->m_sFileName.c_str(), \
														vCfgStaticModules[i]->m_sInput.c_str(), \
														vCfgStaticModules[i]->m_sOutput.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		sNames.insert(vCfgStaticModules[i]->m_sName);
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to module syntax check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ModuleManager::LoadModules(const vector< ConfigModule_t* >& vCfgModules)
{
	Module_t* pMod = NULL;
	void* pRet;
	bool flag;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	flag = false;
	for (size_t i = 0; i < vCfgModules.size(); ++i)
	{
		assert(vCfgModules[i]);
        if (!flag)
		{
			pMod = new Module_t;
			flag = true;
		}
        pMod->m_sName = vCfgModules[i]->m_sName;
        pMod->m_nType = GET_MODULE_TYPE_MOD(vCfgModules[i]->m_sType);
        pMod->m_sMain = vCfgModules[i]->m_sMain;
        pMod->m_sFileName = vCfgModules[i]->m_sFileName;
        pMod->m_sDesc = vCfgModules[i]->m_sDesc;

        if (IS_SO(pMod->m_nType))
		{
			if (access(pMod->m_sFileName.c_str(), F_OK | R_OK))
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "module access failed, %s, errno: %d, name: %s, filename: %s.", strerror(errno), errno, pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pMod->m_pHandle = dlopen(pMod->m_sFileName.c_str(), RTLD_LAZY);
			if (!pMod->m_pHandle)
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "module open dynamic library failed, %s, name: %s, filename: %s", dlerror(), pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pRet = dlsym(pMod->m_pHandle, pMod->m_sMain.c_str());
			if (!pRet)
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "module load dynamic library's symbol failed, %s, name: %s, main: %s, filename: %s", dlerror(), pMod->m_sName.c_str(), pMod->m_sMain.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pMod->m_pCallback = (ModCallback)pRet;
		}
		else
		{
			if (access(pMod->m_sFileName.c_str(), F_OK | R_OK | X_OK))
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "module access failed, %s, errno: %d, name: %s, filename: %s.", strerror(errno), errno, pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
		}
		m_mapAllModule.insert(make_pair(pMod->m_sName, pMod));
		m_mapModule.insert(make_pair(pMod->m_sName, pMod));
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

int ModuleManager::LoadTriggers(const vector< ConfigTrigger_t* >& vCfgTriggers)
{
	Trigger_t* pMod = NULL;
	void* pRet;
	bool flag;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load triggers.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	flag = false;
	for (size_t i = 0; i < vCfgTriggers.size(); ++i)
	{
		assert(vCfgTriggers[i]);
        if (!flag)
		{
			pMod = new Trigger_t;
			flag = true;
		}
        pMod->m_sName = vCfgTriggers[i]->m_sName;
        pMod->m_nType = GET_MODULE_TYPE_TRIG(vCfgTriggers[i]->m_sType);
        pMod->m_sMain = vCfgTriggers[i]->m_sMain;
        pMod->m_sFileName = vCfgTriggers[i]->m_sFileName;
        pMod->m_sDesc = vCfgTriggers[i]->m_sDesc;

        if (IS_SO(pMod->m_nType))
		{
			if (access(pMod->m_sFileName.c_str(), F_OK | R_OK))
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "trigger access failed, %s, errno: %d, name: %s, filename: %s.", strerror(errno), errno, pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pMod->m_pHandle = dlopen(pMod->m_sFileName.c_str(), RTLD_LAZY);
			if (!pMod->m_pHandle)
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "trigger open dynamic library failed, %s, name: %s, filename: %s", dlerror(), pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pRet = dlsym(pMod->m_pHandle, pMod->m_sMain.c_str());
			if (!pRet)
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "trigger load dynamic library's symbol failed, %s, name: %s, main: %s, filename: %s", dlerror(), pMod->m_sName.c_str(), pMod->m_sMain.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pMod->m_pCallback = (ModCallback)pRet;
		}
		else
		{
			if (access(pMod->m_sFileName.c_str(), F_OK | R_OK | X_OK))
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "trigger access failed, %s, errno: %d, name: %s, filename: %s.", strerror(errno), errno, pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
		}
		m_mapAllModule.insert(make_pair(pMod->m_sName, pMod));
		m_mapTrigger.insert(make_pair(pMod->m_sName, pMod));
		flag = false;
	}

	if (flag)
	{
		delete pMod;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load triggers.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ModuleManager::LoadStaticModules(const vector< ConfigStaticModule_t* >& vCfgStaticModules)
{
	Trigger_t* pMod = NULL;
	void* pRet;
	bool flag;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to load static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	flag = false;
	for (size_t i = 0; i < vCfgStaticModules.size(); ++i)
	{
		assert(vCfgStaticModules[i]);
        if (!flag)
		{
			pMod = new StaticModule_t;
			flag = true;
		}
        pMod->m_sName = vCfgStaticModules[i]->m_sName;
        pMod->m_nType = GET_MODULE_TYPE_STATIC(vCfgStaticModules[i]->m_sType);
        pMod->m_sMain = vCfgStaticModules[i]->m_sMain;
        pMod->m_sFileName = vCfgStaticModules[i]->m_sFileName;
        pMod->m_sDesc = vCfgStaticModules[i]->m_sDesc;

        if (IS_SO(pMod->m_nType))
		{
			if (access(pMod->m_sFileName.c_str(), F_OK | R_OK))
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "static module access failed, %s, errno: %d, name: %s, filename: %s.", strerror(errno), errno, pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pMod->m_pHandle = dlopen(pMod->m_sFileName.c_str(), RTLD_LAZY);
			if (!pMod->m_pHandle)
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "static module open dynamic library failed, %s, name: %s, filename: %s", dlerror(), pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pRet = dlsym(pMod->m_pHandle, pMod->m_sMain.c_str());
			if (!pRet)
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "static module load dynamic library's symbol failed, %s, name: %s, main: %s, filename: %s", dlerror(), pMod->m_sName.c_str(), pMod->m_sMain.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
			pMod->m_pCallback = (ModCallback)pRet;
		}
		else
		{
			if (access(pMod->m_sFileName.c_str(), F_OK | R_OK | X_OK))
			{
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "static module access failed, %s, errno: %d, name: %s, filename: %s.", strerror(errno), errno, pMod->m_sName.c_str(), pMod->m_sFileName.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				continue;
			}
		}
		m_mapAllModule.insert(make_pair(pMod->m_sName, pMod));
		m_mapStaticModule.insert(make_pair(pMod->m_sName, pMod));
		flag = false;
	}

	if (flag)
	{
		delete pMod;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to load static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

const Module_t* ModuleManager::GetModule(const string& name) const
{
    map< string, Module_t* >::const_iterator iter = m_mapModule.find(name);
    if (iter != m_mapModule.end())
	{
        return iter->second;
	}

	return NULL;
}

const Trigger_t* ModuleManager::GetTrigger(const string& name) const
{
    map< string, Trigger_t* >::const_iterator iter = m_mapTrigger.find(name);
    if (iter != m_mapTrigger.end())
	{
        return iter->second;
	}

	return NULL;
}

const StaticModule_t* ModuleManager::GetStaticModule(const string& name) const
{
    map< string, StaticModule_t* >::const_iterator iter = m_mapStaticModule.find(name);
    if (iter != m_mapStaticModule.end())
	{
        return iter->second;
	}

	return NULL;
}


CLOSE_NAMESPACE_JFR
