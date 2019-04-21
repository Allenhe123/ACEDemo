#pragma once

#include "ace/Task.h"
#include "ace/Thread.h"
#include "ace/OS.h"
#include "ace/Sig_Handler.h"

class SigTask : public ACE_Task<ACE_MT_SYNCH>
{
public:
	virtual int handle_signal(int signum, siginfo_t* = 0, ucontext_t* = 0)
	{
		if (signum == SIGUSR1)
		{
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) received a %S signal\n"), signum));
			handle_alert();
		}

		return 0;
	}

	virtual int svc()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) starting thread\n")));
		while (1)
		{
			ACE_Message_Block* mb;
			ACE_Time_Value tv(0, 1000);
			tv += ACE_OS::time(0);
			int ret = this->getq(mb, &tv);
			if (-1 == ret && errno == EWOULDBLOCK)
				continue;
			else
				process_msg(mb);

		}

		return 0;
	}

	void handle_alert() {}
	void process_msg(ACE_Message_Block* mb)
	{

	}
};


int startup_thrd_sig()
{
	SigTask handler;
	handler.activate(THR_NEW_LWP | THR_JOINABLE, 5);

	ACE_Sig_Handler sh;
	sh.register_handler(SIGUSR1, &handler);

	ACE_OS::sleep(1);

	ACE_Thread_Manager::instance()->kill_grp(handler.grp_id(), SIGUSR1);

	handler.wait();

	return 0;
}