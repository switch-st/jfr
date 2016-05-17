#include "ConfigParser.h"

OPEN_NAMESPACE_JFR

ConfigParser::ConfigParser(void)
{
	m_bInited = false;
	m_pLogger = NULL;
}

ConfigParser::~ConfigParser(void)
{
    if (m_bInited)
		Clear();
}

void ConfigParser::Clear(void)
{
	for (size_t i = 0; i < m_vModules.size(); ++i)
	{
		delete m_vModules[i];
	}
	m_vModules.clear();
	for (size_t i = 0; i < m_vTriggers.size(); ++i)
	{
		delete m_vTriggers[i];
	}
	m_vTriggers.clear();
	for (size_t i = 0; i < m_vStaticModules.size(); ++i)
	{
		delete m_vStaticModules[i];
	}
	m_vStaticModules.clear();
	for (size_t i = 0; i < m_vMainlines.size(); ++i)
	{
		delete m_vMainlines[i];
	}
	m_vMainlines.clear();
}

int ConfigParser::Init(const string& basedir, const string& filename)
{
	if (m_bInited)
	{
		Clear();
	}
	m_sBaseDir = basedir;
	if (m_sBaseDir.c_str()[m_sBaseDir.length() - 1] != '/')
	{
		m_sBaseDir += '/';
	}
    m_sFilename = filename;
    m_pLogger = &jfr::LoggerSingleton::get_mutable_instance();
    m_bInited = true;

    return 0;
}

bool ConfigParser::IsInited(void)
{
	return m_bInited;
}

int ConfigParser::Parse(void)
{
	if (!IsInited())
	{
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING"parse config failed, parser not inited.", LOG_FUNC_VALUE);
		return -1;
	}

	Clear();
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to parse config file, file name: %s.", m_sFilename.c_str());
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
	try
	{
		rapidxml::xml_document<> xmldoc;
		rapidxml::file<> xmlfile(m_sFilename.c_str());
		xml_node<>* pXMLNode;

		xmldoc.parse<0>(xmlfile.data());
		pXMLNode = xmldoc.first_node("root");
		if (!pXMLNode)
		{
			Clear();
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file format error, not found <root> node, file name: %s.", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}

		if (ParseModules(pXMLNode) || ParseTriggers(pXMLNode) || ParseStaticModules(pXMLNode) || ParseMainlines(pXMLNode))
		{
			Clear();
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "parse config file error, file name: %s.", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}

		if (NameUniqCheck())
		{
			Clear();
			m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file lable uniq check failed, file name: %s.", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			return -1;
		}

		return 0;
	}
	catch (parse_error& err)
	{
		Clear();
        m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file format error, got exception, what: %s, where: %s, file name: %s", \
													err.what(), err.where<char>(), m_sFilename.c_str());
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return -1;
	}
	catch (runtime_error& err)
	{
		Clear();
		m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file parse error, got exception, what: %s, file name: %s", \
													err.what(), m_sFilename.c_str());
		m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return -1;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to parse config file.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ConfigParser::ParseModules(xml_node<>* pRoot)
{
	xml_node<>* pXMLNode;

    assert(pRoot);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to parse modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    pXMLNode = pRoot->first_node("module");
    if (!pXMLNode)
	{
        m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file format error, not found <module> node, file name: %s.", m_sFilename.c_str());
        m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return -1;
	}
	for (xml_node<>* pMod = pXMLNode->first_node(); pMod; pMod = pMod->next_sibling())
	{
        if (strncmp(pMod->name(), "module", 7) != 0)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <module/module>, node name: %s, file name: %s.", \
															pMod->name(), m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		xml_attribute<> *pName, *pType, *pMain, *pFile, *pDesc;
		if ((pName = pMod->first_attribute("name")) == NULL ||
			(pType = pMod->first_attribute("type")) == NULL ||
			(pMain = pMod->first_attribute("main")) == NULL ||
			(pFile = pMod->first_attribute("file")) == NULL ||
			(pDesc = pMod->first_attribute("desc")) == NULL)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <module/module> attribute missed, module name: %s, file name: %s.", \
															pName ? pName->value() : "[NULL]", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		else
		{
			ConfigModule_t* pMod = new ConfigModule_t;
			pMod->m_sName = pName->value();
			pMod->m_sType = pType->value();
			pMod->m_sMain = pMain->value();
			FileExpand(pFile->value(), pMod->m_sFileName);
			pMod->m_sDesc = pDesc->value();
			m_vModules.push_back(pMod);
		}
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to parse modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ConfigParser::ParseTriggers(xml_node<>* pRoot)
{
	xml_node<>* pXMLNode;

    assert(pRoot);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to parse triggers.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    pXMLNode = pRoot->first_node("trigger");
    if (!pXMLNode)
	{
        m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file format error, not found <trigger> node, file name: %s.", m_sFilename.c_str());
        m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return -1;
	}
	for (xml_node<>* pMod = pXMLNode->first_node(); pMod; pMod = pMod->next_sibling())
	{
        if (strncmp(pMod->name(), "trigger", 8) != 0)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <trigger/trigger>, node name: %s, file name: %s.", \
															pMod->name(), m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		xml_attribute<> *pName, *pType, *pMain, *pFile, *pDesc;
		if ((pName = pMod->first_attribute("name")) == NULL ||
			(pType = pMod->first_attribute("type")) == NULL ||
			(pMain = pMod->first_attribute("main")) == NULL ||
			(pFile = pMod->first_attribute("file")) == NULL ||
			(pDesc = pMod->first_attribute("desc")) == NULL)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <trigger/trigger> attribute missed, trigger name: %s, file name: %s.", \
															pName ? pName->value() : "[NULL]", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		else
		{
			ConfigTrigger_t* pTrig = new ConfigTrigger_t;
			pTrig->m_sName = pName->value();
			pTrig->m_sType = pType->value();
			pTrig->m_sMain = pMain->value();
			FileExpand(pFile->value(), pTrig->m_sFileName);
			pTrig->m_sDesc = pDesc->value();
			m_vTriggers.push_back(pTrig);
		}
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to parse triggers.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ConfigParser::ParseStaticModules(xml_node<>* pRoot)
{
	xml_node<>* pXMLNode;

    assert(pRoot);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to parse static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    pXMLNode = pRoot->first_node("static_module");
    if (!pXMLNode)
	{
        m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not found <static_module> node, file name: %s.", m_sFilename.c_str());
        m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return 0;
	}
	for (xml_node<>* pMod = pXMLNode->first_node(); pMod; pMod = pMod->next_sibling())
	{
        if (strncmp(pMod->name(), "static_module", 7) != 0)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <static_module/static_module>, node name: %s, file name: %s.", \
															pMod->name(), m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		xml_attribute<> *pName, *pType, *pMain, *pFile, *pDesc, *pInput, *pOutput;
		if ((pName = pMod->first_attribute("name")) == NULL ||
			(pType = pMod->first_attribute("type")) == NULL ||
			(pMain = pMod->first_attribute("main")) == NULL ||
			(pFile = pMod->first_attribute("file")) == NULL ||
			(pDesc = pMod->first_attribute("desc")) == NULL ||
			(pInput = pMod->first_attribute("argv_in")) == NULL ||
			(pOutput = pMod->first_attribute("argv_out")) == NULL)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <static_module/static_module> attribute missed, static_module name: %s, file name: %s.", \
															pName ? pName->value() : "[NULL]", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		else
		{
			ConfigStaticModule_t* pMod = new ConfigStaticModule_t;
			pMod->m_sName = pName->value();
			pMod->m_sType = pType->value();
			pMod->m_sMain = pMain->value();
			FileExpand(pFile->value(), pMod->m_sFileName);
			pMod->m_sDesc = pDesc->value();
			pMod->m_sInput = pInput->value();
			pMod->m_sOutput = pOutput->value();
			m_vStaticModules.push_back(pMod);
		}
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to parse static modules.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ConfigParser::ParseMainlines(xml_node<>* pRoot)
{
	xml_node<>* pXMLNode;
	bool flag;
	ConfigMainLine_t* pLine;

    assert(pRoot);
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to parse main lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    pXMLNode = pRoot->first_node("main_line");
    if (!pXMLNode)
	{
        m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file format error, not found <main_line> node, file name: %s.", m_sFilename.c_str());
        m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
        return -1;
	}
	flag = false;
	for (xml_node<>* pMod = pXMLNode->first_node(); pMod; pMod = pMod->next_sibling())		// lable <line>
	{
        if (strncmp(pMod->name(), "line", 5) != 0)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <main_line/line>, node name: %s, file name: %s.", \
															pMod->name(), m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		if (!flag)
		{
			pLine = new ConfigMainLine_t;
			flag = true;
		}
		xml_attribute<> *pName, *pDesc, *pInput, *pOutput;
		if ((pName = pMod->first_attribute("name")) == NULL ||
			(pDesc = pMod->first_attribute("desc")) == NULL)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <main_line/line> attribute missed, line name: %s, file name: %s.", \
															pName ? pName->value() : "[NULL]", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		else
		{
			pLine->m_sName = pName->value();
			pLine->m_sDesc = pDesc->value();
		}
		xml_node<>* pTrig = pMod->first_node("trigger");		// lable <trigger>
		if (!pTrig)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format error, node <main_line/line/trigger> missed, line name: %s, file name: %s.", \
															pMod->name(), m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		if ((pName = pTrig->first_attribute("trig_name")) == NULL ||
			(pInput = pTrig->first_attribute("argv_in")) == NULL ||
			(pOutput = pTrig->first_attribute("argv_out")) == NULL)
		{
			m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <main_line/line/trigger> attribute missed, line name: %s, trigger name: %s, file name: %s.", \
															pLine->m_sName.c_str(), pName ? pName->value() : "[NULL]", m_sFilename.c_str());
			m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
			continue;
		}
		else
		{
            pLine->m_oTrigger.m_sName = pName->value();
            pLine->m_oTrigger.m_sInput = pInput->value();
            pLine->m_oTrigger.m_sOutput = pOutput->value();
		}
		for (xml_node<>* pModSub = pMod->first_node(); pModSub; pModSub = pModSub->next_sibling())		// lable <module>
		{
			if (strncmp(pModSub->name(), "module", 7) != 0)
			{
				if (strncmp(pModSub->name(), "trigger", 8) != 0)
				{
					m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <main_line/line/module>, node name: %s, file name: %s.", \
																	pModSub->name(), m_sFilename.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				}
				continue;
			}
            if ((pName = pModSub->first_attribute("mod_name")) == NULL ||
				(pInput = pModSub->first_attribute("argv_in")) == NULL ||
                (pOutput = pModSub->first_attribute("argv_out")) == NULL)
            {
				m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <main_line/line/module> attribute missed, line name: %s, module name: %s, file name: %s.", \
																pLine->m_sName.c_str(), pName ? pName->value() : "[NULL]", m_sFilename.c_str());
				m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
                continue;
            }
            else
            {
                ConfigModule_t mod;
				mod.m_sName = pName->value();
				mod.m_sInput = pInput->value();
				mod.m_sOutput = pOutput->value();
				xml_node<>* pReq = pModSub->first_node("requirement");		// lable <requirement>
				if (!pReq)
				{
					m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format error, node <main_line/line/module/requirement> missed, line name: %s, module name: %s, file name: %s.", \
																	pLine->m_sName.c_str(), mod.m_sName.c_str(), m_sFilename.c_str());
					m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
					continue;
				}
				for (xml_node<>* pModSub2 = pReq->first_node(); pModSub2; pModSub2 = pModSub2->next_sibling())		// lable <mod>
				{
					if (strncmp(pModSub2->name(), "mod", 7) != 0)
					{
						m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <main_line/line/module/requirement/mod>, node name: %s, file name: %s.", \
																		pModSub2->name(), m_sFilename.c_str());
						m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
						continue;
					}
					xml_attribute<> *pSubName, *pSubRet;
					if ((pSubName = pModSub2->first_attribute("name")) == NULL ||
						(pSubRet = pModSub2->first_attribute("ret_val")) == NULL ||
						(!isdigit(pSubRet->value()[0]) && !(pSubRet->value()[0] == '-' && isdigit(pSubRet->value()[1])) && strncmp(pSubRet->value(), "any", 4)))
					{
						m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <main_line/line/module/requirement/mod> attribute missed, line name: %s, module name: %s, mod name: %s, file name: %s.", \
																		pLine->m_sName.c_str(), mod.m_sName.c_str(), pSubName ? pSubName->value() : "[NULL]", m_sFilename.c_str());
						m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
						continue;
					}
					else
					{
						RetValue_t retVal;
						if (pSubRet->value()[0] == 'a')		// string value "any"
						{
							retVal.m_nType = RT_ANY;
						}
						else
						{
							retVal.m_nType = RT_SINGLE;
							retVal.m_nRetValue = atoi(pSubRet->value());
						}
						mod.m_vRequirement.push_back(make_pair(string(pSubName->value()), retVal));
					}
				}
				xml_node<>* pEqu = pModSub->first_node("equivalent");		// lable <equivalent>
				if (pEqu)
				{
					for (xml_node<>* pModSub2 = pEqu->first_node(); pModSub2; pModSub2 = pModSub2->next_sibling())		// lable <mod>
					{
						if (strncmp(pModSub2->name(), "mod", 7) != 0)
						{
							m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, not node <main_line/line/module/equivalent/mod>, node name: %s, file name: %s.", \
																			pModSub2->name(), m_sFilename.c_str());
							m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
							continue;
						}
						xml_attribute<>* pSubName;
						if ((pSubName = pModSub2->first_attribute("name")) == NULL)
						{
							m_pLogger->LogWrite(WARNING, MODULE_JFR, "config file format, node <main_line/line/module/equivalent/mod> attribute missed, line name: %s, module name: %s, mod name: %s, file name: %s.", \
																			pLine->m_sName.c_str(), mod.m_sName.c_str(), pSubName ? pSubName->value() : "[NULL]", m_sFilename.c_str());
							m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
							continue;
						}
						else
						{
							mod.m_vEquivalent.push_back(pSubName->value());
						}
					}
				}
				else
				{
					m_pLogger->LogWrite(DEBUG_2, MODULE_JFR, "config file format, node <main_line/line/module/equivalent> missed, line name: %s, module name: %s, file name: %s.", \
																	pLine->m_sName.c_str(), mod.m_sName.c_str(), m_sFilename.c_str());
					m_pLogger->LogWrite(DEBUG_2, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
				}
				pLine->m_vModules.push_back(mod);
			}
		}
		m_vMainlines.push_back(pLine);
		flag = false;
	}

	if (flag)
	{
		delete pLine;
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to parse main lines.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

int ConfigParser::NameUniqCheck(void)
{
    set< string > sNames;

	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "Begin to name uniq check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
    for (size_t i = 0; i < m_vModules.size(); ++i)
	{
		assert(m_vModules[i]);
		ConfigModule_t* pMod = m_vModules[i];
		if (sNames.find(pMod->m_sName) != sNames.end())
		{
            m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file node name uniq check failed, duplicate node name: %s, file name: %s.", pMod->m_sName.c_str(), m_sFilename.c_str());
            m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		sNames.insert(pMod->m_sName);
	}
    for (size_t i = 0; i < m_vTriggers.size(); ++i)
	{
		assert(m_vTriggers[i]);
		ConfigTrigger_t* pMod = m_vTriggers[i];
		if (sNames.find(pMod->m_sName) != sNames.end())
		{
            m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file node name uniq check failed, duplicate node name: %s, file name: %s.", pMod->m_sName.c_str(), m_sFilename.c_str());
            m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		sNames.insert(pMod->m_sName);
	}
    for (size_t i = 0; i < m_vStaticModules.size(); ++i)
	{
		assert(m_vStaticModules[i]);
		ConfigStaticModule_t* pMod = m_vStaticModules[i];
		if (sNames.find(pMod->m_sName) != sNames.end())
		{
            m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file node name uniq check failed, duplicate node name: %s, file name: %s.", pMod->m_sName.c_str(), m_sFilename.c_str());
            m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		sNames.insert(pMod->m_sName);
	}
    for (size_t i = 0; i < m_vMainlines.size(); ++i)
	{
		assert(m_vMainlines[i]);
		ConfigMainLine_t* pMod = m_vMainlines[i];
		if (sNames.find(pMod->m_sName) != sNames.end())
		{
            m_pLogger->LogWrite(ERROR, MODULE_JFR, "config file node name uniq check failed, duplicate node name: %s, file name: %s.", pMod->m_sName.c_str(), m_sFilename.c_str());
            m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);
            return -1;
		}
		sNames.insert(pMod->m_sName);
	}
	m_pLogger->LogWrite(DEBUG_3, MODULE_JFR, "End to name uniq check.");
	m_pLogger->LogWrite(DEBUG_1, MODULE_JFR, LOG_FUNC_STRING, LOG_FUNC_VALUE);

	return 0;
}

void ConfigParser::FileExpand(const char* in, string& out)
{
	assert(in);
	if (in[0] == '/')
		out = in;
	out = m_sBaseDir + in;
	return;
}


const string& ConfigParser::GetFileName(void) const
{
	return m_sFilename;
}

const vector< ConfigModule_t* >& ConfigParser::GetModules(void) const
{
	return m_vModules;
}

const vector< ConfigTrigger_t* >& ConfigParser::GetTriggers(void) const
{
	return m_vTriggers;
}

const vector< ConfigStaticModule_t* >& ConfigParser::GetStaticModules(void) const
{
	return m_vStaticModules;
}

const vector< ConfigMainLine_t* >& ConfigParser::GetMainlines(void) const
{
	return m_vMainlines;
}

CLOSE_NAMESPACE_JFR
