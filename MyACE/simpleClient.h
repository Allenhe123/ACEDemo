#pragma once

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"
#include "ace/Time_Value.h"

int startup_client()
{
	ACE_INET_Addr srv(1002, ACE_LOCALHOST);
	ACE_SOCK_Connector connector;
	ACE_SOCK_Stream peer;
	if (connector.connect(peer, srv) == -1)
	{
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("connect")), 1);
	}

	int bc;
	char buf[64];
	peer.send_n("hello world!\n", 13);
	bc = peer.recv(buf, sizeof(buf));
	write(1, buf, bc);
	peer.close();

	return 0;
}