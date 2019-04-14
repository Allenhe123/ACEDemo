#pragma once

#include "ace/OS.h"
#include "ace/Log_Msg.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Bcast.h"
#include "ace/SOCK_Dgram_Mcast.h"

// µ¥²¥
int send_unicast(const ACE_INET_Addr& to)
{
	const char* message = "this is the message!\n";
	ACE_INET_Addr my_addr(static_cast<u_short>(10101));
	ACE_SOCK_Dgram udp(my_addr);

	while (true)
	{
		ssize_t sent = udp.send(message, strlen(message) + 1, to);
		if (sent == -1)
		{
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("udp send")), -1);
			continue;
		}

		ACE_INET_Addr your_addr;
		char buff[1024] = { 0 };
		ssize_t recv_cnt = udp.recv(buff, 1024, your_addr);
		if (recv_cnt > 0)
		{
			char addrBuff[128] = { 0 };
			your_addr.addr_to_string(addrBuff, 128);
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("udp connection from:%s\n"), addrBuff));
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("udp recv:%s\n"), buff));
		}
	}

	udp.close();

	return 0;
}

// ¹ã²¥
int send_broadcast(u_short to_port)
{
	const char* message = "this is the message!\n";
	ACE_INET_Addr my_addr(static_cast<u_short>(10101));
	ACE_SOCK_Dgram_Bcast udp(my_addr);

	ssize_t sent = udp.send(message, ACE_OS_String::strlen(message) + 1, to_port);
	if (sent == -1)
	{
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("udp send")), -1);
	}

	//ACE_INET_Addr your_addr;
	//char buff[1024] = { 0 };
	//ssize_t recv_cnt = udp.recv(buff, 1024, your_addr);
	//if (recv_cnt > 0)
	//{
	//	char addrBuff[128] = { 0 };
	//	your_addr.addr_to_string(addrBuff, 128);
	//	ACE_DEBUG((LM_DEBUG, ACE_TEXT("udp connection from:%s\n"), addrBuff));
	//	ACE_DEBUG((LM_DEBUG, ACE_TEXT("udp recv:%s\n"), buff));
	//}

	udp.close();

	return 0;
}

// ¶à²¥
int send_multicast(const ACE_INET_Addr& mcast_addr)
{
	const char* message = "this is the message!\n";
	ACE_SOCK_Dgram_Mcast udp;
	if (udp.join(mcast_addr) == -1)
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("join")), -1);

	ssize_t sent = udp.send(message, ACE_OS_String::strlen(message) + 1);
	if (sent == -1)
	{
		ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("udp send")), -1);
	}
	udp.close();
	return 0;
}

void echo_dgram()
{
	ACE_INET_Addr my_addr(static_cast<u_short>(10102));
	ACE_INET_Addr your_addr;
	ACE_SOCK_Dgram udp(my_addr);

	while (1)
	{
		char buff[1024] = { 0 };
		ssize_t recv_cnt = udp.recv(buff, 1024, your_addr);
		if (recv_cnt > 0)
		{
			char addrBuff[128] = { 0 };
			your_addr.addr_to_string(addrBuff, 128);
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("udp connection from:%s\n"), addrBuff));
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("udp recv:%s\n"), buff));
			ssize_t sent = udp.send(buff, 1024, your_addr);
			if (sent == -1)
				ACE_ERROR((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("udp send")));
		}
	}
	udp.close();
	return;
}

void startup_udp_send()
{
	ACE_INET_Addr to(static_cast<u_short>(10102), ACE_LOCALHOST);
	send_unicast(to);
}


void startup_udp_recv()
{
	echo_dgram();
}