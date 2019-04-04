#pragma once

#include "ace/Log_Msg.h"
#include "ace/streams.h"
#include "ace/Log_Msg_Callback.h"
#include "ace/Log_Record.h"

//

#include "ace/SOCK_Stream.h"
#include "ace/Sock_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Singleton.h"
#include "ace/Null_Mutex.h"
#include <fstream>


class CallBack : public ACE_Log_Msg_Callback
{
public:
	void log(ACE_Log_Record& log_record)
	{
		log_record.print(ACE_TEXT(""), 0, cerr);
		log_record.print(ACE_TEXT(""), ACE_Log_Msg::VERBOSE, cerr);
	}
};


void foo();
void logTest(int argc, char* argv[]);
///

class LogManager
{
public:
	LogManager();
	~LogManager();

	void redirectToDaemon(const ACE_TCHAR* prog_name = ACE_TEXT(""));
	void redirectToSysLog(const ACE_TCHAR* prog_name = ACE_TEXT(""));
	void redirectToOStream(ACE_OSTREAM_TYPE* output);
	void redirectToFile(const char* filename);
	void redirectToStderr();
	ACE_Log_Msg_Callback* redirectToCallBack(ACE_Log_Msg_Callback* callback);

private:
	ACE_OSTREAM_TYPE* output_stream_ = nullptr;
	ACE_OSTREAM_TYPE* log_stream_ = nullptr;
};

typedef ACE_Singleton<LogManager, ACE_Null_Mutex> LogMgrSingleton;
#define LOG_MANAGER LogMgrSingleton::instance()