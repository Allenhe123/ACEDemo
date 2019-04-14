#pragma once

#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS.h"
#include "ace/Process.h"
#include "ace/OS_NS_pwd.h"
#include "ace/Process_Manager.h"


class Slave
{
public:
	Slave()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Slave::Slave\n")));
	}

	int doWork()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Slave::doWork\n")));
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P) started at %T, parent is %d\n"), ACE_OS::getppid()));  // windows return -1 for getppid()
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P) the private environment is %s\n"), ACE_OS::getenv("PRIVATE_VAR")));

		ACE_TCHAR str[128];
		ACE_OS::sprintf(str, ACE_TEXT("(%d) enter your command\n" ), ACE_OS::getpid());
		ACE_OS::write(ACE_STDOUT, str, ACE_OS::strlen(str));
		//this->readLine(str);
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P) executed: %C\n"), str));
		return 0;
	}
};

class Manager : public ACE_Process
{
public:
	Manager(const ACE_TCHAR* program_name)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Manager::Manager\n")));
		ACE_OS::strcpy(programName_, program_name);
	}

	int doWork()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Manager::doWork\n")));

		ACE_Process_Options ops;
		pid_t pid = this->spawn(ops);
		if (-1 == pid)
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("spawn")), -1);

		if (this->wait() == -1)
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("wait")), -1);

		this->dumpRun();
		return 0;
	}

	virtual int prepare(ACE_Process_Options &options)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Manager::prepare\n")));
		options.command_line("%s 1", this->programName_);
		if (this->setStdHandles(options) == -1 || this->setEnvVariable(options) == -1)
		{
			return -1;
		}

#if !defined(ACE_WIN32)
		return this->setUserID(options);
#else
		return 0;
#endif
	}

	int setStdHandles(ACE_Process_Options& options)
	{
		ACE_OS::unlink("output.dat");
		this->outputfd_ = ACE_OS::open("output.dat", O_RDWR | O_CREAT);
		return options.set_handles(ACE_STDIN, ACE_STDOUT, this->outputfd_);
	}

	int setEnvVariable(ACE_Process_Options& options)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Manager::setEnvVariable\n")));
		return options.setenv("PRIVATE_VAR=/that/seems/to/be/it");
	}

	// only run on Unix
	int setUserID(ACE_Process_Options& options)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Manager::setUserID\n")));
		//passwd* pw = ACE_OS::getpwnam("nobody");
		//if (pw == 0) return -1;
		//options.seteuid(pw->pw_uid);
		return 0;
	}

	int dumpRun()
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("Manager::dumpRun\n")));
		if (ACE_OS::lseek(this->outputfd_, 0, SEEK_SET) == -1)
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"),ACE_TEXT("lseek")), -1);
		
		char buf[1024];
		int length = 0;
		while ((length = ACE_OS::read(this->outputfd_, buf, sizeof(buf) - 1)) > 0)
		{
			buf[length] = 0;
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("%C\n"), buf));
		}

		ACE_OS::close(this->outputfd_);
		return 0;
	}

private:
	ACE_TCHAR programName_[128];
	ACE_HANDLE outputfd_;
};

////////////

static const int CHILDREN = 2;

class DeathHandler : public ACE_Event_Handler
{
public:
	DeathHandler()
	{
		ACE_DEBUG((LM_INFO, ACE_TEXT("DeathHandler::DeathHandler")));
	}

	virtual int handle_exit(ACE_Process* process)
	{
		ACE_DEBUG((LM_INFO, ACE_TEXT("DeathHandler::handle_exit")));
		ACE_DEBUG((LM_INFO, ACE_TEXT("process %d exit with exit code %d\n"), process->getpid(), process->return_value()));
		if (++count_ == CHILDREN)
			ACE_Reactor::instance()->end_reactor_event_loop();
		return 0;
	}

private:
	int count_ = 0;
};

int startup_process(int argc, ACE_TCHAR* argv[])
{
	if (argc > 1)
	{
		Slave s;
		return s.doWork();
	}
	else
	{
		Manager m(argv[0]);
		return m.doWork();
	}
}

int startup_processMgr(int argc, ACE_TCHAR* argv[])
{
	// child process
	if (argc > 1)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("child process, pid:%d\n"), ACE_OS::getpid()));
		ACE_OS::sleep(10);
	}
	else
	{
		ACE_Process_Manager* pm = ACE_Process_Manager::instance();
		ACE_Process_Options opt;
		opt.command_line(ACE_TEXT("%s a"), argv[0]);

		pid_t pids[2];
		pm->spawn_n(2, opt, pids);

		// destroy the first child
		pm->terminate(pids[0]);
		// wait for the child we just terminated
		ACE_exitcode status;
		pm->wait(pids[0], &status);
		// get the result of termination
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("the process terminated with exit code %d\n"), status));

		pm->wait(0);
	}

	return 0;
}

int startup_processMgr_Reactor(int argc, ACE_TCHAR* argv[])
{
	if (argc > 1)
	{
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("child process, pid:%d\n"), ACE_OS::getpid()));
		return 0;    // child process
	}

	ACE_Process_Manager pm(10, ACE_Reactor::instance());

	DeathHandler handler;
	ACE_Process_Options op;
	op.command_line(ACE_TEXT("%s, a"), argv[0]);

	pid_t pids[CHILDREN];
	pm.spawn_n(CHILDREN, op, pids);

	// register handlers to be called when these processes exit
	for (int i = 0; i < CHILDREN; i++)
		pm.register_handler(&handler, pids[i]);

	ACE_Reactor::instance()->run_reactor_event_loop();
	return 0;
}