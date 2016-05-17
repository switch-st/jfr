#include "Common.h"
#include "CmdPara.h"
#include "Config.h"
#include "Logger.h"
#include "ConfigParser.h"
#include "ModuleManager.h"
#include "ModuleCaller.h"
#include "MainlineManager.h"
#include "RuntimeManager.h"
#include "RuntimeSet.h"
#include "ThreadPool.h"
#include <Daemonize/Daemonize.hpp>

using namespace std;
USING_NAMESPACE_JFR
USING_NAMESPACE_SWITCHTOOL

static void ShowConfig(jfr::LoggerSingleton& logger, Config& config);
static void ShowStart(jfr::LoggerSingleton& logger, Config& config);
static void ShowEnd(jfr::LoggerSingleton& logger, Config& config);

int main(int argc, char** argv)
{
	int ret;

	CmdPara& cmd_para = CmdPara::get_mutable_instance();
	ret = cmd_para.ParsePara(argc, argv);
	if (ret == -1)
	{
        cerr << "parse parameters failed." << endl;
        cmd_para.ShowHelp();
        return 1;
	}
	if (cmd_para.IsHelp())
	{
		cmd_para.ShowHelp();
		return 0;
	}

    Config& config = Config::get_mutable_instance();
    ret = cmd_para.SetConfigbyPara(config);		// get basedir & jfr cfg file from cmd
    if (config.GetVersionFlag())
	{
        cout << config.GetVersion() << endl;
        cout << config.GetBuiltDate() << endl;
        return 0;
	}
    if (ret == -1)
	{
		cerr << "invalid parameters." << endl;
		cmd_para.ShowHelp();
		return 1;
	}
    ret = config.ParseConfigFile();
    if (ret == -1)
	{
		cerr << "parse config file failed." << endl;
		return 1;
	}
	ret = cmd_para.SetConfigbyPara(config);		// override all file cfg by cmd
    if (ret == -1)
	{
		cerr << "invalid parameter or config." << endl;
		return 1;
	}
	if (config.GetCfgCheckFlag())
	{
        config.SetDebugLevel(JFR_DEFAULT_CONFIG_DEBUGLEVEL_MAX);
        config.SetLog2TermFlag(true);
	}

	DaemonizeSingleton& daemon = DaemonizeSingleton::get_mutable_instance();
	daemon.BaseDir() = config.GetBaseDir();
	daemon.PidFile() = MAIN_NAME".pid";
	if (!config.GetCfgCheckFlag() && daemon.CheckPidFile() == -1)
	{
        cerr << "Already started." << endl;
        cerr << "Check the pid file : " << daemon.BaseDir() << "/" << daemon.PidFile() << endl;
        return 1;
	}

	jfr::LoggerSingleton& logger = jfr::LoggerSingleton::get_mutable_instance();
	ret = logger.Init(config.GetDebugLevel() + 3, config.GetBaseDir(), config.GetLogPrefix(), config.GetLog2TermFlag());
	if (ret == -1)
	{
		cerr << "Init logger failed." << endl;
        return 1;
	}
	ShowStart(logger, config);

	ConfigParser& cfg_parser = ConfigParser::get_mutable_instance();
	string sFlowFileName = config.GetBaseDir() + "/" + config.GetFlowFileName();
	ret = cfg_parser.Init(config.GetBaseDir(), sFlowFileName);
	if (ret == -1)
	{
		cerr << "config parser init failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "config parser init failed, %s start failed.", MODULE_JFR);
		return 1;
	}
	ret = cfg_parser.Parse();
	if (ret == -1)
	{
        cerr << "config parser parse failed." << endl;
        logger.LogWrite(ERROR, MODULE_JFR, "config parser init failed, %s start failed.", MODULE_JFR);
        return 1;
	}

	ModuleManager& module_man = ModuleManager::get_mutable_instance();
	ret = module_man.Init();
	if (ret == -1)
	{
		cerr << "module manager init failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "module manager init failed, %s start failed.", MODULE_JFR);
		return 1;
	}
	ret = module_man.LoadAllModules(cfg_parser.GetModules(), cfg_parser.GetTriggers(), cfg_parser.GetStaticModules());
	if (ret == -1)
	{
		cerr << "module manager load all modules failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "module manager load all modules failed, %s start failed.", MODULE_JFR);
		return 1;
	}

	MainlineManager& mainline_man = MainlineManager::get_mutable_instance();
	ret = mainline_man.Init();
	if (ret == -1)
	{
		cerr << "mainline manager init failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "mainline manager init failed, %s start failed.", MODULE_JFR);
		return 1;
	}
	ret = mainline_man.LoadStaticModules(cfg_parser.GetStaticModules());
	if (ret == -1)
	{
		cerr << "mainline manager load static modules failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "mainline manager load static modules failed, %s start failed.", MODULE_JFR);
		return 1;
	}
	ret = mainline_man.LoadMainlines(cfg_parser.GetMainlines());
	if (ret == -1)
	{
		cerr << "mainline manager load mainlines failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "mainline manager load mainlines failed, %s start failed.", MODULE_JFR);
		return 1;
	}

	if (config.GetCfgCheckFlag())
	{
		cout << "config check success." << endl;
		logger.LogWrite(INFO, MODULE_JFR, "config check success, %s exit normally.", MODULE_JFR);
		return 0;
	}

    if (config.GetDaemonFlag())
	{
		ret = daemon.SetDaemonize();
		if (ret == -1)
		{
            cerr << "daemonize failed." << endl;
            logger.LogWrite(ERROR, MODULE_JFR, "daemonize failed, %s start failed.", MODULE_JFR);
            return 1;
		}
	}
	ret = daemon.WritePidFile();
	if (ret == -1)
	{
		cerr << "daemon write pid file failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "daemon write pid file failed, %s start failed.", MODULE_JFR);
		return 1;
	}
	logger.SetChecker();
	/// TODO signal

	ThreadPool& pool = ThreadPool::get_mutable_instance();
    pool.set_max_thread_size(config.GetMaxThreadSize());
    pool.set_max_job_size(config.GetMaxJobSize());

    RuntimeManager& runtime_man = RuntimeManager::get_mutable_instance();
    ret = runtime_man.Init(config.GetMaxRunLines(), mainline_man.GetMainLines(), mainline_man.GetStaticModules());
    if (ret == -1)
	{
        cerr << "runtime manager init failed." << endl;
        logger.LogWrite(ERROR, MODULE_JFR, "runtime manager init failed, %s start failed.", MODULE_JFR);
        return 1;
	}
	ShowEnd(logger, config);
	ShowConfig(logger, config);
	ret = runtime_man.Run();
	if (ret == -1)
	{
		cerr << "runtime manager run failed." << endl;
		logger.LogWrite(ERROR, MODULE_JFR, "runtime manager run failed, %s start failed.", MODULE_JFR);
        return 1;
	}

	return 0;
}

void ShowConfig(jfr::LoggerSingleton& logger, Config& config)
{
	if (!config.GetLog2TermFlag())
	{
        printf("config:\n");
        printf("base dir: %s\n", config.GetBaseDir().c_str());
        printf("config file name: %s\n", config.GetCfgFileName().c_str());
        printf("flow file name: %s\n", config.GetFlowFileName().c_str());
        printf("log prefix: %s\n", config.GetLogPrefix().c_str());
        printf("max run lines: %d\n", config.GetMaxRunLines());
        printf("max thread size: %d\n", config.GetMaxThreadSize());
        printf("max job size: %d\n", config.GetMaxJobSize());
        printf("debug level: %d\n", config.GetDebugLevel());
        printf("daemonize: %s\n", config.GetDaemonFlag() ? "true" : "false");
        printf("log to terminal: %s\n", config.GetLog2TermFlag() ? "true" : "false");
        printf("%s\n", config.GetVersion().c_str());
        printf("%s\n", config.GetBuiltDate().c_str());
	}
    logger.LogWrite(INFO, MODULE_JFR, "config:");
    logger.LogWrite(INFO, MODULE_JFR, "base dir: %s", config.GetBaseDir().c_str());
    logger.LogWrite(INFO, MODULE_JFR, "config file name: %s", config.GetCfgFileName().c_str());
    logger.LogWrite(INFO, MODULE_JFR, "flow file name: %s", config.GetFlowFileName().c_str());
    logger.LogWrite(INFO, MODULE_JFR, "log prefix: %s", config.GetLogPrefix().c_str());
    logger.LogWrite(INFO, MODULE_JFR, "max run lines: %d", config.GetMaxRunLines());
    logger.LogWrite(INFO, MODULE_JFR, "max thread size: %d", config.GetMaxThreadSize());
    logger.LogWrite(INFO, MODULE_JFR, "max job size: %d", config.GetMaxJobSize());
    logger.LogWrite(INFO, MODULE_JFR, "debug level: %d", config.GetDebugLevel());
    logger.LogWrite(INFO, MODULE_JFR, "daemonize: %s", config.GetDaemonFlag() ? "true" : "false");
    logger.LogWrite(INFO, MODULE_JFR, "log to terminal: %s", config.GetLog2TermFlag() ? "true" : "false");
    logger.LogWrite(INFO, MODULE_JFR, "%s", config.GetVersion().c_str());
    logger.LogWrite(INFO, MODULE_JFR, "%s", config.GetBuiltDate().c_str());
}

void ShowStart(jfr::LoggerSingleton& logger, Config& config)
{
	if (!config.GetLog2TermFlag())
	{
		logger.LogWrite(INFO, MODULE_JFR, "");
		logger.LogWrite(INFO, MODULE_JFR, "");
		printf("%s starting\n", MODULE_JFR);
	}
    logger.LogWrite(INFO, MODULE_JFR, "%s starting", MODULE_JFR);
}

void ShowEnd(jfr::LoggerSingleton& logger, Config& config)
{
	if (!config.GetLog2TermFlag())
	{
		printf("%s start successfully\n", MODULE_JFR);
	}
	logger.LogWrite(INFO, MODULE_JFR, "%s start successfully", MODULE_JFR);
}
