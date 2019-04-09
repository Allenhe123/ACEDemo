#pragma once
#include "ace/Log_Msg.h"
#include "ace/Reactor.h"
#include "ace/Signal.h"

class LoopStopper : public ACE_Event_Handler
{
public:
	LoopStopper(int sig = SIGINT)
	{
		ACE_Reactor::instance()->register_handler(this, sig);
	}

	virtual int handle_signal(int signum, siginfo_t* info = 0, ucontext_t* cxt = 0)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("recv a sig: %d, end the event loop.\n"), signum));
		ACE_Reactor::instance()->end_reactor_event_loop();
		return 0;
	}
};


class LogSwitcher : public ACE_Event_Handler
{
public:
	LogSwitcher(int on_sig, int off_sig): on_sig_(on_sig), off_sig_(off_sig) 
	{
		ACE_Sig_Set sigs;
		sigs.sig_add(on_sig_);
		sigs.sig_add(off_sig_);
		ACE_Reactor::instance()->register_handler(this, sigs);
	}

	virtual int handle_signal(int signum, siginfo_t* info = 0, ucontext_t* cxt = 0)
	{
		if (signum == this->on_sig_ || signum == this->off_sig_)
		{
			this->on_off_ = signum == this->on_sig_;
			ACE_Reactor::instance()->notify(this, ACE_Event_Handler::EXCEPT_MASK);
		}
		return 0;
	}

	virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		if (this->on_off_)
			ACE_LOG_MSG->clr_flags(ACE_Log_Msg::SILENT);
		else
			ACE_LOG_MSG->set_flags(ACE_Log_Msg::SILENT);
		return 0;
	}

private:
	LogSwitcher() {}

	int on_sig_;
	int off_sig_;
	int on_off_;
};

void startup_signal()
{
	LoopStopper stopper;
	LogSwitcher switcher(SIGABRT, SIGALRM);
	ACE_Reactor::instance()->run_event_loop();
}