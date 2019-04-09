#pragma once
#include "ace/Log_Msg.h"
#include "ace/Reactor.h"
#include "ace/Time_Value.h"
#include "ace/OS.h"
#include <iostream>

typedef void(*timerTask_t)();

pid_t timerTask(int initDelay, int interval, timerTask_t task)
{
	if (initDelay < 1 && interval < 1) return -1;
	pid_t pid = ACE_OS::fork();
	if (pid < 0) return -1;
	else if (pid > 0) return pid;

	if (initDelay > 0)
		ACE_OS::sleep(initDelay);
	if (interval < 1) return 0;

	while (1)
	{
		(*task)();
		ACE_OS::sleep(interval);
	}

	return 0;
}

void fooTimer()
{
	time_t now = time(0);
	std::cerr << "the time is : " << ctime(&now) << std::endl;
}

///

class MyTimerHandler : public ACE_Event_Handler
{
public:
	virtual int handle_timeout(const ACE_Time_Value& currentTime, const void* p = 0)
	{
		time_t epoch = ((timespec_t)currentTime).tv_sec;
		ACE_DEBUG((LM_INFO, ACE_TEXT("handle_timeout: %s"), ACE_OS::ctime(&epoch)));
		return 0;
	}
};

class SigHandler : public ACE_Event_Handler
{
public:
	SigHandler(int interval, int timerId): currentInterval_(interval), timerId_(timerId)
	{
		ACE_Reactor::instance()->register_handler(this, SIGINT);
		ACE_Reactor::instance()->register_handler(this, SIGABRT);
	}

	virtual int handle_signal(int signum, siginfo_t* info = 0, ucontext_t* cxt = 0)
	{
		if (signum == SIGINT)
		{
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("recv a sig: %d, adjust the timer's interval, timerId: %d, interval from %d to %d\n"),
				signum, timerId_, currentInterval_, ++currentInterval_));
			ACE_Time_Value newInterval(this->currentInterval_);
			ACE_Reactor::instance()->reset_timer_interval(timerId_, newInterval);
			//ACE_Reactor::instance()->end_reactor_event_loop();
		}
		else if (signum == SIGABRT)
		{
			ACE_DEBUG((LM_INFO, ACE_TEXT("caceling timer, id: %d"), this->timerId_));
			ACE_Reactor::instance()->cancel_timer(this->timerId_);
		}

		return 0;
	}

private:
	long timerId_;
	int currentInterval_;
};


int startup_timer()
{
	//pid_t pid = timerTask(3, 5, fooTimer); // not effect on windows !


	MyTimerHandler timerHandler;
	ACE_Time_Value initDelay(3);
	ACE_Time_Value interval(5);
	long timerId = ACE_Reactor::instance()->schedule_timer(&timerHandler, 0, initDelay, interval);
	SigHandler sigHandler(5, timerId);
	ACE_Reactor::instance()->run_event_loop();

	return 0;
}