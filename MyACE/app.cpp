#define ACE_NTRACE 1  // 若为0则开启ACE_TRACE功能
#include "ace/Init_ACE.h"
#include "ace/Service_Config.h"

#include "log.h"
#include "getopt.h"
#include "simpleClient.h"
#include "simpleSrv.h"
#include "reactorSrv1.h"
#include "reactorClient1.h"
#include "reactorSig.h"
#include "reactorTimer.h"
#include "acceptor_connect_framework.h"
#include "proactor_framework.h"


//int ACE_MAIN(int, ACE_TCHAR* [])
int main(int argc, char* argv[])
{
	ACE::init();

	//logTest(argc, argv);

	//LOG_MANAGER->redirectToFile("log.dat");
	//ACE_TRACE(ACE_TEXT("main"));
	//ACE_DEBUG((LM_INFO, "%Ihello world!\n"));

	//int ret = ACE_Service_Config::open(argc, argv, ACE_DEFAULT_LOGGER_KEY, 1, 0, 1);
	//if (ret < 0)
	//	ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("Service Config open")), 1);
	
	//ACE_TRACE(ACE_TEXT("main"));
	//ACE_DEBUG((LM_NOTICE, ACE_TEXT("%t%Ihello world!\n")));
	//ACE_DEBUG((LM_INFO, ACE_TEXT("%t%Igood night!\n")));

	//testOpt();

	//startup_client();
	//startup_srv();

	//startup_reactor1_srv();

	//startup_signal();

	//startup_timer();

	//startup_connector_framework();

	//startup_acceptor_framework();

	startup_proactor_framework_acceptor();

	//startup_proactor_framework_connector();


	ACE::fini();

	system("pause");

	return 0;
}