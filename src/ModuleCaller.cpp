#include "ModuleCaller.h"

OPEN_NAMESPACE_JFR

ThreadPool* ModuleCaller::pool = &ThreadPool::get_mutable_instance();
jfr::LoggerSingleton* ModuleCaller::logger = &jfr::LoggerSingleton::get_mutable_instance();

int ModuleCaller::Call(StaticRuntime_t* pRuntime)
{
    return CallStaticModule(pRuntime);
}

int ModuleCaller::Call(Runtime_t* pRuntime)
{
    return CallTrigger(pRuntime);
}

int ModuleCaller::Call(Runtime_t* pRuntime, LineModule_t* pModule)
{
    return CallModule(pRuntime, pModule);
}

int ModuleCaller::CallStaticModule(StaticRuntime_t* pRuntime)
{
	int ret;

    assert(pRuntime->m_pStaticModule && pRuntime->m_pCtx && pRuntime->m_pStaticModule->m_oModule.m_pModule);
    if (IS_PRO(pRuntime->m_pStaticModule->m_oModule.m_pModule->m_nType))
	{
		ret = CallPro(pRuntime->m_pStaticModule->m_oModule.m_pModule, \
						pRuntime->m_pCtx, \
						pRuntime->m_pStaticModule->m_oModule.m_vInputArgs, \
						pRuntime->m_pStaticModule->m_oModule.m_vOutputArgs, \
						pRuntime->m_mapArgs);
		return ret;
	}
	else if (IS_SO(pRuntime->m_pStaticModule->m_oModule.m_pModule->m_nType))
	{
		ret = CallSo(pRuntime->m_pStaticModule->m_oModule.m_pModule, \
					pRuntime->m_pCtx, \
					pRuntime->m_pStaticModule->m_oModule.m_vInputArgs, \
					pRuntime->m_pStaticModule->m_oModule.m_vOutputArgs, \
					pRuntime->m_mapArgs);
		return ret;
	}
	else
	{
		logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		assert(0);
	}

	return 0;
}

int ModuleCaller::CallModule(Runtime_t* pRuntime, LineModule_t* pModule)
{
	bool ret;
	map< LineModule_t*, ModContext_t* >::iterator iter;
	ModContext_t* pCtx;

	assert(pRuntime->m_pLine && pModule->m_pModule);
    iter = pRuntime->m_mapModCtxs.find(pModule);
    if (iter == pRuntime->m_mapModCtxs.end())
    {
    	logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        assert(0);
    }
    pCtx = iter->second;
    assert(pCtx);
	if (IS_PRO(pModule->m_pModule->m_nType))
	{
		SJob job(&ModuleCaller::CallPro, pModule->m_pModule, pCtx, pModule->m_vInputArgs, pModule->m_vOutputArgs, pRuntime->m_mapArgs);
		ret = pool->add_job_block(job);
		if (!ret)
		{
			pCtx->m_oMutex.lock();
			pCtx->m_nStat = RTS_SYSERROR;
			pCtx->m_oMutex.unlock();
			logger->LogWrite(ERROR, MODULE_JFR, "add job failed, module name: %s, line name: %s.", pModule->m_pModule->m_sName.c_str(), pRuntime->m_pLine->m_sName.c_str());
			logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		return 0;
	}
	else if (IS_SO(pModule->m_pModule->m_nType))
	{
		SJob job(&ModuleCaller::CallSo, pModule->m_pModule, pCtx, pModule->m_vInputArgs, pModule->m_vOutputArgs, pRuntime->m_mapArgs);
		ret = pool->add_job_block(job);
		if (!ret)
		{
			pCtx->m_oMutex.lock();
			pCtx->m_nStat = RTS_SYSERROR;
			pCtx->m_oMutex.unlock();
			logger->LogWrite(ERROR, MODULE_JFR, "add job failed, module name: %s, line name: %s.", pModule->m_pModule->m_sName.c_str(), pRuntime->m_pLine->m_sName.c_str());
			logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		return 0;
	}
	else
	{
		logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		assert(0);
	}

	return 0;
}

int ModuleCaller::CallTrigger(Runtime_t* pRuntime)
{
	bool ret;

	assert(pRuntime->m_pLine && pRuntime->m_pTriggerCtx && pRuntime->m_pLine->m_oTrigger.m_pTrigger);
	if (IS_PRO(pRuntime->m_pLine->m_oTrigger.m_pTrigger->m_nType))
	{
		SJob job(&ModuleCaller::CallPro, \
				pRuntime->m_pLine->m_oTrigger.m_pTrigger, \
				pRuntime->m_pTriggerCtx, \
				pRuntime->m_pLine->m_oTrigger.m_vInputArgs, \
				pRuntime->m_pLine->m_oTrigger.m_vOutputArgs, \
				pRuntime->m_mapArgs);
		ret = pool->add_job_block(job);
		if (!ret)
		{
			pRuntime->m_pTriggerCtx->m_oMutex.lock();
			pRuntime->m_pTriggerCtx->m_nStat = RTS_SYSERROR;
			pRuntime->m_pTriggerCtx->m_oMutex.unlock();
			logger->LogWrite(ERROR, MODULE_JFR, "add job failed, module name: %s, line name: %s.", pRuntime->m_pLine->m_oTrigger.m_pTrigger->m_sName.c_str(), pRuntime->m_pLine->m_sName.c_str());
			logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		return 0;
	}
	else if (IS_SO(pRuntime->m_pLine->m_oTrigger.m_pTrigger->m_nType))
	{
		SJob job(&ModuleCaller::CallSo, \
				pRuntime->m_pLine->m_oTrigger.m_pTrigger, \
				pRuntime->m_pTriggerCtx, \
				pRuntime->m_pLine->m_oTrigger.m_vInputArgs, \
				pRuntime->m_pLine->m_oTrigger.m_vOutputArgs, \
				pRuntime->m_mapArgs);
		ret = pool->add_job_block(job);
		if (!ret)
		{
			pRuntime->m_pTriggerCtx->m_oMutex.lock();
			pRuntime->m_pTriggerCtx->m_nStat = RTS_SYSERROR;
			pRuntime->m_pTriggerCtx->m_oMutex.unlock();
			logger->LogWrite(ERROR, MODULE_JFR, "add job failed, module name: %s, line name: %s.", pRuntime->m_pLine->m_oTrigger.m_pTrigger->m_sName.c_str(), pRuntime->m_pLine->m_sName.c_str());
			logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}
		return 0;
	}
	else
	{
		logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		assert(0);
	}

	return 0;
}

int ModuleCaller::CallPro(Module_t* pMod, ModContext_t* pCtx, const vector< ModArg_t* >& vInput, const vector< ModArg_t* >& vOutput, map< ModArg_t*, ArgValue_t* >& mapArg)
{
	pid_t pid;
	int fd[2];
	int ret;
	map< ModArg_t*, ArgValue_t* >::iterator iter, end;
	char** ppArgValIn;
	ArgValue_t* pArgValOut;

	StaticModuleArgSet* pSMASet = &StaticModuleArgSet::get_mutable_instance();
	end = mapArg.end();
	ppArgValIn = (char**)malloc((vInput.size() + 2) * sizeof(char*));
	ppArgValIn[0] = (char*)pMod->m_sFileName.c_str();
	for(size_t i = 0; i < vInput.size(); ++i)
	{
		ArgValue_t* p;
		iter = mapArg.find(vInput[i]);
		if (iter != end)
		{
			p = iter->second;
		}
		else if ((p = pSMASet->GetElement(vInput[i])) == NULL)
		{
			logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            assert(0);
		}
		ppArgValIn[i + 1] = (char*)p->m_pValue;
	}
	ppArgValIn[vInput.size() + 1] = NULL;
	if (vOutput.size() > 1)
	{
		logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		assert(0);
	}
	if (vOutput.size() == 1)
	{
		iter = mapArg.find(vOutput[0]);
		if (iter == end)
		{
			logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
		}
		pArgValOut = iter->second;
	}
	else
	{
		pArgValOut = NULL;
	}

	if (pipe(fd))
	{
		pCtx->m_oMutex.lock();
		pCtx->m_nStat = RTS_SYSERROR;
		pCtx->m_oMutex.unlock();
		free(ppArgValIn);
		logger->LogWrite(ERROR, MODULE_JFR, "make pipe failed, %s, errno: %d.", strerror(errno), errno);
		logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	pid = fork();
	if (pid == -1)
	{
		pCtx->m_oMutex.lock();
		pCtx->m_nStat = RTS_SYSERROR;
		pCtx->m_oMutex.unlock();
		free(ppArgValIn);
		logger->LogWrite(ERROR, MODULE_JFR, "fork process failed, %s, errno: %d.", strerror(errno), errno);
		logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		return -1;
	}
	else if (pid == 0)		// child
	{
		close(fd[0]);
		dup2(fd[1], 1);
		if (fd[1] != 1)
		{
			close(fd[1]);
		}
        execv(pMod->m_sFileName.c_str(), ppArgValIn);
		logger->LogWrite(ERROR, MODULE_JFR, "child: exec process failed, %s, errno: %d.", strerror(errno), errno);
		logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
		cout << JFR_CALLPRO_ERROR_STRING;
		return JFR_CALLPRO_ERROR_NO;
	}
	else	// parent
	{
        char* buf = NULL;

        close(fd[1]);
        buf = Read(fd[0]);
        close(fd[0]);
        ret = Wait(pid);
        if (buf)
		{
            if (strncmp(buf, JFR_CALLPRO_ERROR_STRING, JFR_CALLPRO_ERROR_LEN) == 0 && ret == JFR_CALLPRO_ERROR_NO)	// exec error
			{
				pCtx->m_oMutex.lock();
				pCtx->m_nStat = RTS_SYSERROR;
				pCtx->m_oMutex.unlock();
				free(ppArgValIn);
				free(buf);
				logger->LogWrite(ERROR, MODULE_JFR, "parent: exec process failed.");
				logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				return -1;
			}
		}
		pCtx->m_oMutex.lock();
		pCtx->m_nRetValue = ret;
		pCtx->m_nStat = RTS_FINISH;
		pCtx->m_oMutex.unlock();
		if (pArgValOut)
		{
            pArgValOut->m_oMutex.lock();
            if (pArgValOut->m_pValue)
            {
                pArgValOut->Free();
            }
            pArgValOut->m_pValue = (void*)buf;
            pArgValOut->m_pFreeFunc = free;
            pArgValOut->m_oMutex.unlock();
		}
		else
		{
			free(buf);
		}
	}
	free(ppArgValIn);

	return 0;
}

int ModuleCaller::CallSo(Module_t* pMod, ModContext_t* pCtx, const vector< ModArg_t* >& vInput, const vector< ModArg_t* >& vOutput, map< ModArg_t*, ArgValue_t* >& mapArg)
{
	int ret;
	map< ModArg_t*, ArgValue_t* >::iterator iter, end;
	ArgValue_t** ppArgValIn;
	ArgValue_t** ppArgValOut;

	assert(pMod->m_pCallback && pMod->m_pHandle);
	StaticModuleArgSet* pSMASet = &StaticModuleArgSet::get_mutable_instance();
	end = mapArg.end();
	ppArgValIn = (ArgValue_t**)malloc((vInput.size() + 1) * sizeof(ArgValue_t*));
	for(size_t i = 0; i < vInput.size(); ++i)
	{
		ArgValue_t* p;
		iter = mapArg.find(vInput[i]);
		if (iter != end)
		{
			p = iter->second;
		}
		else if ((p = pSMASet->GetElement(vInput[i])) == NULL)
		{
			logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            assert(0);
		}
		ppArgValIn[i] = p;
	}
	ppArgValIn[vInput.size()] = NULL;
	ppArgValOut = (ArgValue_t**)malloc((vOutput.size() + 1) * sizeof(ArgValue_t*));
	for(size_t i = 0; i < vOutput.size(); ++i)
	{
		iter = mapArg.find(vOutput[i]);
		if (iter == end)
		{
			logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            assert(0);
		}
		ArgValue_t* p = iter->second;
        ppArgValOut[i] = p;
	}
	ppArgValOut[vOutput.size()] = NULL;

	ret = pMod->m_pCallback(logger, ppArgValIn, ppArgValOut);
	pCtx->m_oMutex.lock();
	pCtx->m_nRetValue = ret;
	pCtx->m_nStat = RTS_FINISH;
	pCtx->m_oMutex.unlock();
	free(ppArgValIn);
	free(ppArgValOut);

	return 0;
}

char* ModuleCaller::Read(int fd)
{
	int ret, len, pos;
	char* buf;

    assert(fd >= 0);
    buf = NULL;
    ret = len = pos = 0;
    const int block = 1024;
    while (1)
	{
		if (pos == len)
		{
			len += block;
			buf = (char*)realloc((void*)buf, len);
			assert(buf);
		}
		ret = read(fd, buf + pos, len  - pos);
		if (ret == -1)
		{
            if (errno != EINTR)
			{
                free(buf);
				logger->LogWrite(ERROR, MODULE_JFR, "read failed, %s, errno: %d.", strerror(errno), errno);
				logger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                return NULL;
			}
		}
		else if (ret == 0)
		{
            buf[pos] = '\0';
            return buf;
		}
		else
		{
            pos += ret;
		}
	}

	return NULL;
}

int ModuleCaller::Wait(pid_t pid)
{
	int ret;

    assert(pid > 0);
    while (1)
	{
		if (waitpid(pid, &ret, 0) == -1)
		{
            if (errno == EINTR)
			{
				continue;
			}
			logger->LogWrite(ERROR, MODULE_JFR, "wait process failed, %s, errno: %d.", strerror(errno), errno);
			logger->LogWrite(FATAL, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			assert(0);
		}
		else
		{
			return WEXITSTATUS(ret);
		}
	}

	return 0;
}


CLOSE_NAMESPACE_JFR
