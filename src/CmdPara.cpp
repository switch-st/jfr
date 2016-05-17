#include "CmdPara.h"

OPEN_NAMESPACE_JFR


CmdPara::CmdPara(void)
{
	Clear();
}

CmdPara::~CmdPara(void)
{
}

void CmdPara::Clear(void)
{
	m_nArgc = 0;
	m_pArgv = NULL;
    m_bInit = false;
    m_pBaseDir = NULL;
    m_pFlowCfg = NULL;
    m_pJfrCfg = NULL;
    m_pMaxRunLines = NULL;
    m_pMaxThreadSize = NULL;
    m_pMaxJobSize = NULL;
    m_pDebugLevel = NULL;
    m_pLogPrefix = NULL;
    m_bDaemon = false;
    m_bLog2Term = false;
    m_bVersion = false;
    m_bCheckCfg = false;
}

void CmdPara::ShowHelp(void)
{
    cout << "help:" << endl;
	cout << MAIN_NAME;
    cout << " [-b base_dir] [-f flow_config_file] [-s jfr_config_file]" << endl;
    cout << "\t [-l max_run_lines] [-t max_thread_size] [-j max_job_size]" << endl;
    cout << "\t [-v debug_level] [-p log_prefix] [-cdmhV]" << endl;
    cout << "\t-b base_dir\t\tbase directory." << endl;
    cout << "\t-f flow_config_file\tflow config file." << endl;
    cout << "\t-s jfr_config_file\tjfr config file." << endl;
    cout << "\t-l max_run_lines\tmax run lines num." << endl;
    cout << "\t-t max_thread_size\tmax thread pool size." << endl;
    cout << "\t-j max_job_size\t\tmax thread jobs num." << endl;
    cout << "\t-v debug_level\t\tdebug level:0, 1, 2, 3, default 0." << endl;
    cout << "\t-p log_prefix\t\tlog prefix." << endl;
    cout << "\t-c\t\t\tonly config syntax check." << endl;
    cout << "\t-d\t\t\trun as daemon." << endl;
    cout << "\t-m\t\t\tlog to terminal(stderr)." << endl;
    cout << "\t-h\t\t\tshow this help page." << endl;
    cout << "\t-V\t\t\tshow version massage." << endl;
    cout << endl;
}

int CmdPara::ParsePara(int argc, char** argv)
{
	int ret;

	if (m_bInit)
		Clear();

    assert(argc > 0 && argv);
	m_nArgc = argc;
	m_pArgv = argv;

	optind = 1;
	while (1)
	{
		ret = getopt(argc, argv, JFR_CMDPARA_OPTSTRING);
		if (ret == -1)
		{
			break;
		}
		switch (ret)
		{
			case 'b':
				m_pBaseDir = optarg;
				break;
			case 'c':
				m_bCheckCfg = true;
				break;
			case 'd':
				m_bDaemon = true;
				break;
			case 'f':
				m_pFlowCfg = optarg;
				break;
			case 'h':
				m_bShowHelp = true;
				break;
			case 'j':
				m_pMaxJobSize = optarg;
				break;
			case 'l':
				m_pMaxRunLines = optarg;
				break;
			case 'm':
				m_bLog2Term = true;
				break;
			case 'p':
				m_pLogPrefix = optarg;
				break;
			case 's':
				m_pJfrCfg = optarg;
				break;
			case 't':
				m_pMaxThreadSize = optarg;
				break;
			case 'v':
				m_pDebugLevel = optarg;
				break;
			case 'V':
				m_bVersion = true;
				break;
			default:
				return -1;
				break;
		}
	}
	m_bInit = true;

	return 0;
}

bool CmdPara::IsHelp(void)
{
	return m_bShowHelp;
}

int CmdPara::SetConfigbyPara(Config& cfg)
{
	int ret;
    if (!m_bInit)
		return -1;

	ret = 0;
    if (m_pBaseDir && cfg.SetBaseDir(string(m_pBaseDir)))
		ret = -1;
	if (m_pFlowCfg && cfg.SetFlowFileName(string(m_pFlowCfg)))
		ret = -1;
	if (m_pJfrCfg && cfg.SetCfgFileName(string(m_pJfrCfg)))
		ret = -1;
	if (m_pMaxRunLines && cfg.SetMaxRunLines(atoi(m_pMaxRunLines)))
		ret = -1;
	if (m_pMaxThreadSize && cfg.SetMaxThreadSize(atoi(m_pMaxThreadSize)))
		ret = -1;
	if (m_pMaxJobSize && cfg.SetMaxJobSize(atoi(m_pMaxJobSize)))
		ret = -1;
	if (m_pDebugLevel && cfg.SetDebugLevel(atoi(m_pDebugLevel)))
		ret = -1;
	if (m_pLogPrefix && cfg.SetLogPrefix(string(m_pLogPrefix)))
		ret = -1;
	if (m_bDaemon && cfg.SetDaemonFlag(m_bDaemon))
		ret = -1;
	if (m_bLog2Term && cfg.SetLog2TermFlag(m_bLog2Term))
		ret = -1;
	if (m_bVersion && cfg.SetVersionFlag(m_bVersion))
		ret = -1;
	if (m_bCheckCfg && cfg.SetCfgCheckFlag(m_bCheckCfg))
		ret = -1;

	return ret;
}


CLOSE_NAMESPACE_JFR
