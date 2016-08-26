#include "StdAfx.h"
#include "MyIOCP.h"


MyIOCP::MyIOCP(void)
{
}


MyIOCP::~MyIOCP(void)
{
}


VOID MyIOCP::NotifyReceivedFormatPackage(const char* lpszBuffer)
{
	printf("%s\n", lpszBuffer);
	Send(lpszBuffer);
}

VOID MyIOCP::NotifyConnectionStatus(IOCPClient_ConnectionType ConnectionType){
	if(ConnectionType == IOCPClient_ConnectionType_Connected){
		Send("hello");
	}
}