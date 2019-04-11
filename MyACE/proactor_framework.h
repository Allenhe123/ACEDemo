#pragma once

#include "ace/Asynch_IO.h"
#include "ace/OS.h"
#include "ace/Log_Msg.h"
#include "ace/Message_Block.h"
#include "ace/Asynch_Acceptor.h"
#include "ace/Asynch_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Proactor.h"


/*
* This base class defines the interface for the
* ACE_Asynch_Acceptor to call into when new connection are
* accepted.
*/
class HA_Proactive_Service : public ACE_Service_Handler
{
public:
	~HA_Proactive_Service()
	{
		if (this->handle() != ACE_INVALID_HANDLE)
			ACE_OS::closesocket(this->handle());
	}

	// open is called when new connection established
	virtual void open(ACE_HANDLE h, ACE_Message_Block& blk)
	{
		// save handle in this handler
		this->handle(h);
		// register handler to reader and writer, when IO completed the registered handler will be called
		if (this->reader_.open(*this) != 0 || this->writer_.open(*this) != 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("HA_Proactive_Service open")));
			delete this;
			return;
		}

		// start a read operation on the new socket
		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb, ACE_Message_Block(1024));
		if (this->reader_.read(*mb, mb->space()) != 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("HA_Proactive_Service begin read")));
			mb->release();
			delete this;
			return;
		}

		/// start a write operation
		char* msg = (char*)ACE_Allocator::instance()->malloc(128);
		memset(msg, 0, 128);
		ACE_OS::sprintf((char*)msg, "%s\n", "hello world@@@");
		ACE_Message_Block* mb2 = nullptr;
		ACE_NEW_RETURN(mb2, ACE_Message_Block(msg, strlen(msg)));
		mb2->wr_ptr(strlen(msg));
		if (this->writer_.write(*mb2, mb2->length()) != 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("starting write")));
			mb2->release();
		}

		// mb is now controlled by Proactor framework
		return;
	}

	// called when read operation completed
	virtual void handle_read_stream(const ACE_Asynch_Read_Stream::Result &result)
	{
		ACE_Message_Block& mb = result.message_block();
		if (!result.success() || result.bytes_transferred() == 0)
		{
			ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"),  ACE_TEXT("read operation failed, delete service handler\n")));
			mb.release();
			delete this;
		}
		else
		{
			char msg[1024] = { 0 };
			memcpy(msg, mb.rd_ptr(), mb.length());
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("recv:%s"), msg));

			if (this->writer_.write(mb, mb.length()) != 0)
			{
				ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("starting write")));
				mb.release();
			}
			else
			{
				ACE_Message_Block* newmb;
				ACE_NEW_RETURN(newmb, ACE_Message_Block(1024));
				this->reader_.read(*newmb, newmb->space());
			}
		}
	}

	// called when write operation completed
	virtual void handle_write_stream(const ACE_Asynch_Write_Stream::Result &result)
	{
		result.message_block().release();
		return;
	}

private:
	ACE_Asynch_Read_Stream reader_;
	ACE_Asynch_Write_Stream writer_;
};


class HA_Proactive_Acceptor : public ACE_Asynch_Acceptor<HA_Proactive_Service>
{
public:
	virtual int validate_connection(const ACE_Asynch_Accept::Result& result,
		const ACE_INET_Addr& remote,
		const ACE_INET_Addr& local)
	{
		struct in_addr* remote_addr = reinterpret_cast<struct in_addr*>(remote.get_addr());
		struct in_addr* local_addr = reinterpret_cast<struct in_addr*>(local.get_addr());
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("aio_acceptor validate connection\n")));
		//if (inet_netof(*local_addr) == inet_netof(*remote_addr))
			return 0;
		//return -1;
	}

};

int startup_proactor_framework_acceptor()
{
	ACE_INET_Addr listen_addr(1002);
	HA_Proactive_Acceptor acceptor;
	int ret = acceptor.open(listen_addr, 0, false, 5, 1, 0, 1);
	if (0 != ret)
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("acceptoe open")), 1);
	ACE_Proactor::instance()->proactor_run_event_loop();
	return 0;
}

int startup_proactor_framework_connector()
{
	ACE_INET_Addr peer_addr(1002, ACE_LOCALHOST);
	ACE_Asynch_Connector<HA_Proactive_Service> connector;
	if (connector.open() != 0)
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("connector open")), 1);
	if (-1 == connector.connect(peer_addr))
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("connect")), 1);
	ACE_Proactor::instance()->proactor_run_event_loop();
	return 0;
}