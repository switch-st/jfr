#include "Config.h"

OPEN_NAMESPACE_JFR

Config::Config(void)
{
    Switch::Tool::DaemonizeSingleton& daemon = Switch::Tool::DaemonizeSingleton::get_mutable_instance();
    if (daemon.SetDefaultBaseDir())
	{
		m_sBaseDir = JFR_DEFAULT_CONFIG_BASEDIR;
	}
	else
	{
		m_sBaseDir = daemon.BaseDir();
	}
    m_sCfgFileName = JFR_DEFAULT_CONFIG_CFGFILENAME;
    m_sFlowFileName = JFR_DEFAULT_CONFIG_FLOWFILENAME;
    m_sLogPrefix = JFR_DEFAULT_CONFIG_LOGPREFIX;
    m_nMaxRunLines = JFR_DEFAULT_CONFIG_MAXRUNLINES;
    m_nMaxThreadSize = JFR_DEFAULT_CONFIG_MAXTHREADSIZE;
    m_nMaxJobSize = JFR_DEFAULT_CONFIG_MAXJOBSIZE;
    m_nDebugLevel = JFR_DEFAULT_CONFIG_DEBUGLEVEL;
    m_bDaemon = JFR_DEFAULT_CONFIG_DEAMON;
    m_bCfgCheck = JFR_DEFAULT_CONFIG_CFGCHECK;
    m_bLog2Term = JFR_DEFAULT_CONFIG_LOG2TERM;
    m_sVersion = JFR_DEFAULT_CONFIG_VERSIONSTRING;
    m_sBuiltDate = JFR_DEFAULT_CONFIG_DATESTRING;
    m_bVersion = JFR_DEFAULT_CONFIG_VERSIONFLAG;
}

Config::~Config(void)
{
}

const string& Config::GetBaseDir(void) const { return m_sBaseDir;}
const string& Config::GetCfgFileName(void) const { return m_sCfgFileName;}
const string& Config::GetFlowFileName(void) const { return m_sFlowFileName;}
const string& Config::GetLogPrefix(void) const { return m_sLogPrefix;}
const int Config::GetMaxRunLines(void) const { return m_nMaxRunLines;}
const int Config::GetMaxThreadSize(void) const { return m_nMaxThreadSize;}
const int Config::GetMaxJobSize(void) const { return m_nMaxJobSize;}
const int Config::GetDebugLevel(void) const { return m_nDebugLevel;}
const bool Config::GetDaemonFlag(void) const { return m_bDaemon;}
const bool Config::GetLog2TermFlag(void) const { return m_bLog2Term;}
const bool Config::GetCfgCheckFlag(void) const { return m_bCfgCheck;}
const string& Config::GetVersion(void) const { return m_sVersion;}
const string& Config::GetBuiltDate(void) const { return m_sBuiltDate;}
const bool Config::GetVersionFlag(void) const { return m_bVersion;}

int Config::SetBaseDir(const string& sBaseDir)
{
    if (sBaseDir != "")
	{
		m_sBaseDir = sBaseDir;
	}

	return 0;
}
int Config::SetCfgFileName(const string& sCfgFileName)
{
    if (sCfgFileName != "")
	{
		m_sCfgFileName = sCfgFileName;
	}

	return 0;
}
int Config::SetFlowFileName(const string& sFlowFileName)
{
    if (sFlowFileName != "")
	{
		m_sFlowFileName = sFlowFileName;
	}

	return 0;
}
int Config::SetLogPrefix(const string& sLogPrefix)
{
    if (sLogPrefix != "")
	{
		m_sLogPrefix = sLogPrefix;
	}

    return 0;
}
int Config::SetMaxRunLines(const int max)
{
	if (max <= 0)
		return -1;
	m_nMaxRunLines = max;
	return 0;
}
int Config::SetMaxThreadSize(const int max)
{
	if (max <= 0)
		return -1;
	m_nMaxThreadSize = max;
	return 0;
}
int Config::SetMaxJobSize(const int max)
{
	if (max <= 0)
		return -1;
	m_nMaxJobSize = max;
	return 0;
}
int Config::SetDebugLevel(const int level)
{
	if (level < JFR_DEFAULT_CONFIG_DEBUGLEVEL_MIN || level > JFR_DEFAULT_CONFIG_DEBUGLEVEL_MAX)
		return -1;
	m_nDebugLevel = level;
	return 0;
}
int Config::SetDaemonFlag(const bool flag)
{
    m_bDaemon = flag;
    return 0;
}
int Config::SetLog2TermFlag(const bool flag)
{
	m_bLog2Term = flag;
	return 0;
}
int Config::SetCfgCheckFlag(const bool flag)
{
	m_bCfgCheck = flag;
	return 0;
}
int Config::SetVersionFlag(const bool flag)
{
	m_bVersion = flag;
	return 0;
}

int Config::ParseConfigFile(void)
{
	xml_node<> *pXMLRoot, *pXMLNode;
    string sFileName = m_sBaseDir + "/" + m_sCfgFileName;
    int max, ret;

	ret = 0;
    try
    {
		rapidxml::xml_document<> xmldoc;
		rapidxml::file<> xmlfile(sFileName.c_str());
		xmldoc.parse<0>(xmlfile.data());
		pXMLRoot = xmldoc.first_node("root");
		if (!pXMLRoot)
		{
            cerr << MODULE_JFR"[ERROR]: config file format error, not found <root> node, file name: " << sFileName << endl;
            return -1;
		}
		pXMLNode = pXMLRoot->first_node("max_thread_num");
		if (!pXMLNode)
		{
			cerr << MODULE_JFR"[ERROR]: config file format error, not found <max_thread_num> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				max = atoi(pXMLNode->value());
				if (SetMaxThreadSize(max))
				{
					cerr << MODULE_JFR"[ERROR]: config file node <max_thread_num> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
		pXMLNode = pXMLRoot->first_node("max_job_num");
		if (!pXMLNode)
		{
            cerr << MODULE_JFR"[ERROR]: config file format error, not found <max_job_num> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				max = atoi(pXMLNode->value());
				if (SetMaxJobSize(max))
				{
					cerr << MODULE_JFR"[ERROR]: config file node <max_job_num> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
		pXMLNode = pXMLRoot->first_node("max_runline_num");
		if (!pXMLNode)
		{
			cerr << MODULE_JFR"[ERROR]: config file format error, not found <max_runline_num> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				max = atoi(pXMLNode->value());
				if (SetMaxRunLines(max))
				{
					cerr << MODULE_JFR"[ERROR]: config file node <max_runline_num> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
		pXMLNode = pXMLRoot->first_node("log_prefix");
		if (!pXMLNode)
		{
			cerr << MODULE_JFR"[ERROR]: config file format error, not found <log_prefix> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				if (SetLogPrefix(string(pXMLNode->value())))
				{
					cerr << MODULE_JFR"[ERROR]: config file node <log_prefix> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
		pXMLNode = pXMLRoot->first_node("log_2_dev");
		if (!pXMLNode)
		{
			cerr << MODULE_JFR"[ERROR]: config file format error, not found <log_2_dev> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				bool flag;
				if (strncmp(pXMLNode->value(), "true", 5) == 0)
				{
					flag = true;
					if (SetLog2TermFlag(flag))
					{
						cerr << MODULE_JFR"[ERROR]: config file node <log_2_dev> value error, file name: " << sFileName << endl;
						ret = -1;
					}
				}
				else if (strncmp(pXMLNode->value(), "false", 6) == 0)
				{
					flag = false;
					if (SetLog2TermFlag(flag))
					{
						cerr << MODULE_JFR"[ERROR]: config file node <log_2_dev> value error, file name: " << sFileName << endl;
						ret = -1;
					}
				}
				else
				{
					cerr << MODULE_JFR"[ERROR]: config file node <log_2_dev> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
		pXMLNode = pXMLRoot->first_node("debug_level");
		if (!pXMLNode)
		{
            cerr << MODULE_JFR"[ERROR]: config file format error, not found <debug_level> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				max = atoi(pXMLNode->value());
				if (SetDebugLevel(max))
				{
					cerr << MODULE_JFR"[ERROR]: config file node <debug_level> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
		pXMLNode = pXMLRoot->first_node("daemonize");
		if (!pXMLNode)
		{
			cerr << MODULE_JFR"[ERROR]: config file format error, not found <daemonize> node, file name: " << sFileName << endl;
            ret = -1;
		}
		else
		{
			if (string(pXMLNode->value()) != "")
			{
				bool flag;
				if (strncmp(pXMLNode->value(), "true", 5) == 0)
				{
					flag = true;
					if (SetDaemonFlag(flag))
					{
						cerr << MODULE_JFR"[ERROR]: config file node <daemonize> value error, file name: " << sFileName << endl;
						ret = -1;
					}
				}
				else if (strncmp(pXMLNode->value(), "false", 6) == 0)
				{
					flag = false;
					if (SetDaemonFlag(flag))
					{
						cerr << MODULE_JFR"[ERROR]: config file node <daemonize> value error, file name: " << sFileName << endl;
						ret = -1;
					}
				}
				else
				{
					cerr << MODULE_JFR"[ERROR]: config file node <daemonize> value error, file name: " << sFileName << endl;
					ret = -1;
				}
			}
		}
    }
    catch (parse_error& err)
    {
    	cerr << MODULE_JFR"[ERROR] : config file format error, got exception, what: " << err.what() << ", where: " << err.where<char>() << ", file name: " << sFileName << endl;
        return -1;
    }
    catch (runtime_error& err)
    {
    	cerr << MODULE_JFR"[ERROR] : config file parse error, got exception, what: " << err.what() << ", file name: " << sFileName << endl;
        return -1;
    }

    return ret;
}


CLOSE_NAMESPACE_JFR
