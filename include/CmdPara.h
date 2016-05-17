#ifndef JFR_CMDPARA_H
#define JFR_CMDPARA_H

#include <iostream>
#include <boost/serialization/singleton.hpp>
#include "Common.h"
#include "Config.h"


OPEN_NAMESPACE_JFR


using namespace std;


#define JFR_CMDPARA_OPTSTRING					"b:cdf:hj:l:mp:s:t:v:V"

class CmdPara : public boost::serialization::singleton< CmdPara >
{
public:
	int ParsePara(int argc, char** argv);
	int SetConfigbyPara(Config& cfg);
	bool IsHelp(void);
	void ShowHelp(void);

protected:
	CmdPara(void);
	~CmdPara(void);

private:
	void Clear(void);

private:
	int 						m_nArgc;
	char**						m_pArgv;
	bool						m_bInit;
	char*						m_pBaseDir;
	char*						m_pFlowCfg;
	char*						m_pJfrCfg;
	char*						m_pMaxRunLines;
	char*						m_pMaxThreadSize;
	char*						m_pMaxJobSize;
	char*						m_pDebugLevel;
	char*						m_pLogPrefix;
    bool						m_bDaemon;
    bool						m_bLog2Term;
    bool						m_bVersion;
    bool						m_bCheckCfg;
    bool						m_bShowHelp;
};


CLOSE_NAMESPACE_JFR


#endif // JFR_CMDPARA_H
