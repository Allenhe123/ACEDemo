#pragma once

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Log_Msg.h"
#include "ace/Time_Value.h"

int startup_srv()
{
	ACE_INET_Addr port(1002);
	ACE_SOCK_Acceptor acceptor;
	if (acceptor.open(port, 1) == -1)
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("accept.open")), 100);
	while (1)
	{
		ACE_SOCK_Stream peer;
		ACE_INET_Addr peer_addr;
		ACE_Time_Value timeout(10, 0);
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("accepting...\n")));
		if (acceptor.accept(peer, &peer_addr, &timeout, 0) == -1)
		{
			if (ACE_OS::last_error() == EINTR)
				ACE_DEBUG((LM_ERROR, ACE_TEXT("(%p|%t) interrupted while"), ACE_TEXT(" waitting for client connection")));
			else if (ACE_OS::last_error() == ETIMEDOUT)
				ACE_DEBUG((LM_ERROR, ACE_TEXT("(%p|%t) timeout while"), ACE_TEXT(" waitting for client connection")));
			else
				ACE_DEBUG((LM_ERROR, ACE_TEXT("(%p|%t) other error while"), ACE_TEXT(" waitting for client connection")));
		}
		else
		{
			ACE_TCHAR peer_name[MAXHOSTNAMELEN];
			peer_addr.addr_to_string(peer_name, MAXHOSTNAMELEN);
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) connection from %s\n"), peer_name));
			char buffer[4096];
			ssize_t byte_recv;
			while ((byte_recv = peer.recv(buffer, sizeof(buffer))) > 0)
			{
				buffer[byte_recv] = '\0';
				ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) recv: %s\n"), buffer));
				peer.send_n(buffer, byte_recv);
			}
			peer.close();
		}
	}

	return 0;
}