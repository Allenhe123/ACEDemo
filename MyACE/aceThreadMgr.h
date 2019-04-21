#pragma once

#include "ace/Task.h"
#include "ace/OS.h"
#include "ace/Time_Value.h"

class ExitHandler : public ACE_At_Thread_Exit
{
public:
	virtual void apply()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) is exiting\n")));
	}
};

class HA_CommandHandler : public ACE_Task<ACE_MT_SYNCH>
{
public:
	HA_CommandHandler(const char* name, ExitHandler& eh) : name_(name), exitHandler_(eh) {}

	virtual int svc()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) starting up %C\n"), name_));
		//ACE_OS::sleep(2);
		this->thr_mgr()->at_exit(exitHandler_);
		ACE_Message_Block* mb;
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (this->getq(mb, &nowait) != -1)
		{
			process_msg(mb);
			mb->release();
		}

		// forceful exit!
		ACE_Thread::exit();

		return 0;
	}

	void process_msg(ACE_Message_Block* mb)
	{
		char msg[32] = {0};
		memcpy(msg, mb->rd_ptr(), 32);
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) processing message %C, msg:%C\n"), name_, msg));
		for (int i = 0; i < 100; i++);  // simulate compute bound work
	}

private:
	const char* name_ = nullptr;
	ExitHandler& exitHandler_;
};

int startup_thread_mgr()
{
/*
	HA_CommandHandler threadHandler_hp("high");
	int ret = threadHandler_hp.activate(THR_NEW_LWP | THR_JOINABLE, 1, 1, 15);
	ACE_ASSERT(ret == 0);

	HA_CommandHandler threadHandler_lp("low");
	ret = threadHandler_lp.activate(THR_NEW_LWP | THR_JOINABLE, 1, 1, ACE_THR_PRI_OTHER_DEF);
	ACE_ASSERT(ret == 0);

	//ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) the current thread count is %d\n"), threadHandler.thr_count()));
	//ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) the group id is %d\n"), threadHandler.grp_id()));
	//threadHandler_hp.resume();
	//threadHandler_lp.resume();

	//ACE_Message_Block mb;
	for (int i=0; i<100; i++)
	{
		ACE_Message_Block* newmb1, *newmb2;
		ACE_NEW_RETURN(newmb1, ACE_Message_Block(32), -1);
		ACE_NEW_RETURN(newmb2, ACE_Message_Block(32), -1);

		char* msg = (char*)ACE_Allocator::instance()->malloc(32);
		memset(msg, 0, 32);
		ACE_OS::sprintf((char*)msg, "%d\n", i);

		memcpy(newmb1->wr_ptr(), msg, strlen(msg) + 1);
		newmb1->wr_ptr(strlen(msg) + 1);

		memcpy(newmb2->wr_ptr(), msg, strlen(msg) + 1);
		newmb2->wr_ptr(strlen(msg) + 1);

		threadHandler_hp.putq(newmb1);
		threadHandler_lp.putq(newmb2);
	}

	threadHandler_hp.wait();
	threadHandler_lp.wait();
*/
	//HA_CommandHandler handler("abc");
	//handler.activate(THR_NEW_LWP | THR_JOINABLE, 4);
	//handler.wait();

	ExitHandler eh;
	HA_CommandHandler handler("abc", eh);
	handler.activate();
	ACE_Thread_Manager::instance()->wait();


	return 0;
}
