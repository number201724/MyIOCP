#include "StdAfx.h"
#include "MyIOCP.h"


MyIOCP::MyIOCP(void)
{
}


MyIOCP::~MyIOCP(void)
{
}


VOID MyIOCP::NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText)
{
	sockaddr_in sin;
	int sin_len = sizeof(sin);
	getpeername(lpPerSocketContext->m_Socket,(sockaddr *)&sin,&sin_len);

	printf("ReceivePackage:%s from %s\n",lpszText,inet_ntoa(sin.sin_addr));

	SendEx(lpPerSocketContext,lpszText);
}