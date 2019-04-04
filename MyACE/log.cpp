#include "log.h"

void foo()
{
	ACE_TRACE(ACE_TEXT("foo"));

	ACE_DEBUG((LM_INFO, ACE_TEXT("%IHowdy Pardner\n")));
}

void logTest(int argc, char* argv[])
{
	ACE_TRACE(ACE_TEXT("main"));

	int old_mask1 = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
	int old_mask2 = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::THREAD);
	ACE_DEBUG((LM_INFO, ACE_TEXT("old_mask_process:%i, old_mask_thread:%i\n", old_mask1, old_mask2)));

	ACE_LOG_MSG->priority_mask(LM_INFO | LM_DEBUG | LM_NOTICE | LM_TRACE, ACE_Log_Msg::THREAD);

	ACE_DEBUG((LM_INFO, ACE_TEXT("%IHi Mom!\n")));

	ACE_LOG_MSG->open(argv[0], ACE_Log_Msg::SYSLOG, ACE_TEXT("syslogTest"));

	foo();

	ACE_LOG_MSG->open(argv[0]);

	ACE_DEBUG((LM_INFO, ACE_TEXT("%IGoodnight!\n")));

	ACE_OSTREAM_TYPE* output = new std::ofstream("output.test");
	ACE_LOG_MSG->msg_ostream(output, 0);
	ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
	ACE_DEBUG((LM_DEBUG, ACE_TEXT("%IThis will go to STDERR, syslog & an stream\n")));
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::OSTREAM);
	delete output;
	output = nullptr;

	CallBack* callback = new CallBack;
	ACE_LOG_MSG->set_flags(ACE_Log_Msg::MSG_CALLBACK);
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR);
	ACE_LOG_MSG->msg_callback(callback);
	//ACE_TRACE(ACE_TEXT("callback"));
	ACE_DEBUG((LM_INFO, ACE_TEXT("%Iheyhey\n")));
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::MSG_CALLBACK);
	delete callback;
	callback = nullptr;
}


LogManager::LogManager()
{
}

LogManager::~LogManager()
{
	if (log_stream_)
		dynamic_cast<std::ofstream*>(log_stream_)->close();
	delete log_stream_;

}

void LogManager::redirectToDaemon(const ACE_TCHAR* prog_name)
{
	ACE_LOG_MSG->open(prog_name, ACE_Log_Msg::LOGGER, ACE_DEFAULT_LOGGER_KEY);
}

void LogManager::redirectToSysLog(const ACE_TCHAR* prog_name)
{
	ACE_LOG_MSG->open(prog_name, ACE_Log_Msg::SYSLOG, prog_name);
}

void LogManager::redirectToOStream(ACE_OSTREAM_TYPE* output)
{
	output_stream_ = output;
	ACE_LOG_MSG->msg_ostream(output_stream_);
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR | ACE_Log_Msg::LOGGER);
	ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
}

void LogManager::redirectToFile(const char* filename)
{
	log_stream_ = new std::ofstream();
	dynamic_cast<std::ofstream*>(log_stream_)->open(filename, ios::out | ios::app);
	redirectToOStream(log_stream_);
}

void LogManager::redirectToStderr()
{
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::OSTREAM | ACE_Log_Msg::LOGGER);
	ACE_LOG_MSG->set_flags(ACE_Log_Msg::STDERR);
}

ACE_Log_Msg_Callback* LogManager::redirectToCallBack(ACE_Log_Msg_Callback* callback)
{
	ACE_Log_Msg_Callback* previous = ACE_LOG_MSG->msg_callback(callback);
	if (callback == nullptr)
		ACE_LOG_MSG->clr_flags(ACE_Log_Msg::MSG_CALLBACK);
	else
		ACE_LOG_MSG->set_flags(ACE_Log_Msg::MSG_CALLBACK);
	return previous;
}