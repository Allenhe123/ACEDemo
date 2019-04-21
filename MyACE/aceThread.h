#pragma once

#include "ace/Task.h"
#include "ace/OS.h"

class HA_Device_Repository
{
//public:
//	int update_device(int device_id)
//	{
//		ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, -1);
//		//ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
//		//mutex_.acquire();
//		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) updating device %d\n"), device_id));
//		ACE_OS::sleep(1);
//		//mutex_.release();
//	}
//
//private:
//	ACE_Thread_Mutex mutex_;

public:
	int is_free() { return this->owner_ == nullptr; }
	int is_owner_(ACE_Task_Base* tb) { return (this->owner_ == tb); }
	ACE_Task_Base* get_owner() { return owner_; }
	void set_owner(ACE_Task_Base* owner) { owner_ = owner; }
	int update_device(int device_id)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) updating device %d\n"), device_id));
		//ACE_OS::sleep(1);
		return 0;
	}

private:
	ACE_Task_Base* owner_ = nullptr;
};


class HA_COmmandHandler : public ACE_Task_Base
{
public:

	HA_COmmandHandler(HA_Device_Repository& rep, ACE_Thread_Mutex& mutex, ACE_Thread_Condition<ACE_Thread_Mutex>& cond) : rep_(rep), mutex_(mutex), cond_(cond) {}

	enum { NUM_USES = 10 };

	//virtual int svc()
	//{
	//	ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) Handler Thread running\n")));
	//	//ACE_OS::sleep(4);
	//	for (int i=0; i<NUM_USES; i++)
	//	{
	//		this->rep_.update_device(i);
	//	}
	//	return 0;
	//}

	virtual int svc()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) Handler Thread running\n")));
		for (int i = 0; i < NUM_USES; i++)
		{
			mutex_.acquire();
			while (!rep_.is_free())
			{
				cond_.wait();
			}
			rep_.set_owner(this);
			mutex_.release();

			this->rep_.update_device(i);

			ACE_ASSERT(this->rep_.is_owner_(this));

			rep_.set_owner(nullptr);

			this->cond_.signal();
		}
		return 0;
	}

private:
	HA_Device_Repository& rep_;
	ACE_Thread_Mutex& mutex_;
	ACE_Thread_Condition<ACE_Thread_Mutex>& cond_;
};

int startup_thread()
{
	//ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) Main thread running\n")));

	//HA_Device_Repository rep;
	//HA_COmmandHandler handler1(rep);
	//HA_COmmandHandler handler2(rep);
	//int ret1 = handler1.activate();
	//int ret2 = handler2.activate();
	//ACE_ASSERT(ret1 == 0);
	//ACE_ASSERT(ret2 == 0);

	//handler1.wait();
	//handler2.wait();

	//return 0;


	ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) Main thread running\n")));

	HA_Device_Repository rep;
	ACE_Thread_Mutex mutex;
	ACE_Thread_Condition<ACE_Thread_Mutex> cond(mutex);

	HA_COmmandHandler handler1(rep, mutex, cond);
	HA_COmmandHandler handler2(rep, mutex, cond);

	int ret1 = handler1.activate();
	int ret2 = handler2.activate();
	ACE_ASSERT(ret1 == 0);
	ACE_ASSERT(ret2 == 0);

	handler1.wait();
	handler2.wait();

	return 0;
}
