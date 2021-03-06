#pragma once

#include "ace/Log_Msg.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Reactor.h"
#include "ace/Message_Block.h"
#include "ace/Message_Queue.h"
#include "ace/SOCK_Stream.h"
#include "ace/Null_Condition.h"
#include "ace/OS.h"
//#include "ace/Auto_Ptr.h"

class ClientService : public ACE_Event_Handler
{
public:
	ACE_SOCK_Stream& peer() { return this->sock_; }

	int open()
	{
		ACE_TCHAR peer_name[MAXPATHLEN];
		ACE_INET_Addr perr_addr;
		if (this->sock_.get_remote_addr(perr_addr) == 0 && perr_addr.addr_to_string(peer_name, MAXPATHLEN) == 0)
			ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) connection from %s\n"), peer_name));
		return this->reactor()->register_handler(this, ACE_Event_Handler::READ_MASK);
	}

	virtual ACE_HANDLE get_handle() const { return sock_.get_handle(); }

	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		char buffer[4096];
		ssize_t recv_cnt, send_cnt;
		if ((recv_cnt = sock_.recv(buffer, sizeof(buffer))) <= 0)
		{
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) connection closed\n")));
			return -1;
		}

		ACE_OS::sleep(10);  /// just for test, sleep manually

		send_cnt = sock_.send(buffer, static_cast<size_t>(recv_cnt));
		// return 0 and waitting for more input data
		if (send_cnt == recv_cnt)
			return 0;
		if (send_cnt == -1 && ACE_OS::last_error() != EWOULDBLOCK)
			// not return -1, if real error happened on socket it will be readable, handle_input will be called again then recv will failed and do the clear up
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) %p\n send")), 0);
		// erro is EWOULDBLOCK, retry later?
		if (-1 == send_cnt)
			send_cnt == 0;

		size_t remainning = static_cast<size_t>(recv_cnt - send_cnt);
		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb, ACE_Message_Block(&buffer[send_cnt], remainning), -1);
		int output_off = this->output_q_.is_empty();
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		if (this->output_q_.enqueue_tail(mb, &nowait) == -1)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %p; discarding data\n enqueue failed")));
			mb->release();
			return 0;
		}

		if (output_off)
			return this->reactor()->register_handler(this, ACE_Event_Handler::WRITE_MASK);
		return 0;
	}

	virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		ACE_Message_Block* mb;
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (0 == this->output_q_.dequeue_head(mb, &nowait))
		{
			ssize_t send_cnt = this->sock_.send(mb->rd_ptr(), mb->length());
			if (send_cnt == -1)
				ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %p\n"), ACE_TEXT("send")));
			else
				mb->rd_ptr(static_cast<size_t>(send_cnt));

			if (mb->length() > 0)
			{
				this->output_q_.enqueue_head(mb);
				break;
			}

			mb->release();
		}

		return (this->output_q_.is_empty()) ? -1 : 0;
	}

	virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
	{
		if (close_mask == ACE_Event_Handler::WRITE_MASK)
			return 0;
		close_mask = ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL;
		this->reactor()->remove_handler(this, close_mask);
		this->sock_.close();
		this->output_q_.flush();
		delete this;
		return 0;
	}

protected:
	ACE_SOCK_Stream sock_;
	ACE_Message_Queue<ACE_NULL_SYNCH> output_q_;
};

class ClientAcceptor: public ACE_Event_Handler
{
public:
	virtual ~ClientAcceptor() 
	{
		this->handle_close(ACE_INVALID_HANDLE, 0);
	}
	
	int open(const ACE_INET_Addr& listen_addr) 
	{
		int ret = this->acceptor_.open(listen_addr, 1);
		if (-1 == ret)
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("acceptor.open")), -1);
		return this->reactor()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
	}

	virtual ACE_HANDLE get_handle() const { return this->acceptor_.get_handle(); }

	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		ClientService* client;
		ACE_NEW_RETURN(client, ClientService, -1);
		//auto_ptr<ClientService> p(client);
		ACE_Time_Value timeout(10, 0);
		if (this->acceptor_.accept(client->peer(), 0, &timeout, 0) == -1)
		{
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) %p\n"), ACE_TEXT("failed to accept client connection.")), -1);
			delete client;
			//p.release();
		}
		client->reactor(this->reactor());
		if (client->open() == -1)
			client->handle_close(ACE_INVALID_HANDLE, 0);

		return 0;
	}

	virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask) 
	{
		if (this->acceptor_.get_handle() != ACE_INVALID_HANDLE)
		{
			ACE_Reactor_Mask m = ACE_Event_Handler::ACCEPT_MASK | ACE_Event_Handler::DONT_CALL;
			this->reactor()->remove_handler(this, m);
			this->acceptor_.close();
		}
		return 0;
	}

protected:
	ACE_SOCK_Acceptor acceptor_;
};



int startup_reactor1_srv()
{
	ACE_INET_Addr port_to_listen(1002);
	ClientAcceptor acceptor;
	acceptor.reactor(ACE_Reactor::instance());
	int ret = acceptor.open(port_to_listen);
	if (-1 == ret)
		return 1;

	ACE_Reactor::instance()->run_reactor_event_loop();
	return 0;
}