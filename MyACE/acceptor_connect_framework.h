#pragma once

#include "ace/Log_Msg.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Reactor.h"
#include "ace/Acceptor.h"
#include "ace/Svc_Handler.h"
#include "ace/SOCK_Stream.h"
#include "ace/Message_Block.h"
#include "ace/Reactor_Notification_Strategy.h"
#include "ace/Connector.h"

class ClientService1 : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
	typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> super;

public:
/// Activate the client handler.  This is typically called by the
/// ACE_Acceptor or ACE_Connector, which passes "this" in as the
/// parameter to open.  If this method returns -1 the Svc_Handler's
/// close() method is automatically called.
	virtual int open(void* acceptor_or_connector = 0)
	{
		if (super::open(acceptor_or_connector) == -1) return -1;

		ACE_TCHAR peer_name[MAXPATHLEN];
		ACE_INET_Addr peer_addr;
		if (this->peer().get_remote_addr(peer_addr) == 0 && peer_addr.addr_to_string(peer_name, MAXPATHLEN) == 0)
			ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) connection from %s \n"), peer_name));

		return 0;
	}

	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		char buffer[4096];
		ssize_t recv_cnt, send_cnt;
		if ((recv_cnt = this->peer().recv(buffer, sizeof(buffer))) <= 0)
		{
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) connection closed\n")));
			return -1;
		}

		ACE_OS::sleep(10);  /// just for test, sleep manually

		send_cnt = this->peer().send(buffer, static_cast<size_t>(recv_cnt));
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
		int output_off = this->msg_queue()->is_empty();
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		if (this->putq(mb, &nowait) == -1)
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
		while (0 == this->getq(mb, &nowait))
		{
			ssize_t send_cnt = this->peer().send(mb->rd_ptr(), mb->length());
			if (send_cnt == -1)
				ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %p\n"), ACE_TEXT("send")));
			else
				mb->rd_ptr(static_cast<size_t>(send_cnt));

			if (mb->length() > 0)
			{
				this->ungetq(mb);   // Return a message to the queue.(put mb to queue's head.)
				break;
			}

			mb->release();
		}

		return (this->msg_queue()->is_empty()) ? -1 : 0;
	}

	virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
	{
		if (close_mask == ACE_Event_Handler::WRITE_MASK)
			return 0;
		return super::handle_close(handle, close_mask);
	}
};


typedef ACE_Acceptor<ClientService1, ACE_SOCK_ACCEPTOR> ClientAcceptor1;


////

class Client : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
	typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> super;
public:
	Client() : notifier_(0, this, ACE_Event_Handler::WRITE_MASK) {}
	
	virtual int open(void* connector = 0)
	{
		ACE_Time_Value iter_delay(2); //2 seconds
		if (super::open(connector) == -1) return -1;
		this->notifier_.reactor(this->reactor());
		this->msg_queue()->notification_strategy(&this->notifier_);
		return this->reactor()->schedule_timer(this, 0, ACE_Time_Value::zero, iter_delay);
	}

	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		char buf[64];
		ssize_t recv_cnt = this->peer().recv(buf, sizeof(buf) - 1);
		if (recv_cnt > 0)
		{
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("%*C"), static_cast<size_t>(recv_cnt), buf));
			return 0;
		}
		if (recv_cnt == 0 || ACE_OS::last_error() != EWOULDBLOCK)
		{
			this->reactor()->end_reactor_event_loop();
			return -1;
		}
		return 0;
	}

	virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		ACE_Message_Block* mb;
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (-1 != this->getq(mb, &nowait))
		{
			ssize_t send_cnt = this->peer().send(mb->rd_ptr(), mb->length());
			if (send_cnt == -1)
				ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %p\n"), ACE_TEXT("send")));
			else
				mb->rd_ptr(send_cnt);
			if (mb->length() > 0)
			{
				this->ungetq(mb);
				break;
			}
			mb->release();
		}

		if (this->msg_queue()->is_empty())
			this->reactor()->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);
		else
			this->reactor()->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);
		return 0;
	}

	virtual int handle_timeout(const ACE_Time_Value& current_time, const void* act = 0)
	{
		if (this->iteration_ >= ITERATIONS)
		{
			this->peer().close_writer();
			return 0;
		}

		ACE_Message_Block* mb;
		char msg[128];
		ACE_OS::sprintf(msg, "Iteration %d\n", this->iteration_);
		ACE_NEW_RETURN(mb, ACE_Message_Block(msg), -1);
		this->putq(mb);
		return 0;
	}

private:
	enum
	{
		ITERATIONS = 5
	};

	int iteration_;
	ACE_Reactor_Notification_Strategy notifier_;

};


int startup_acceptor_framework()
{
	ACE_INET_Addr port_to_listen(1002);
	ClientAcceptor1 acceptor;
	if (acceptor.open(port_to_listen) == -1) return 1;
	ACE_Reactor::instance()->run_reactor_event_loop();
	return 0;
}

int startup_connector_framework()
{
	ACE_INET_Addr port_to_connect(1002);
	ACE_Connector<Client, ACE_SOCK_CONNECTOR> connector;
	Client client;
	Client* pc = &client;
	if (connector.connect(pc, port_to_connect) == -1)
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("connect")), 1);
	ACE_Reactor::instance()->run_reactor_event_loop();
	return 0;
}
