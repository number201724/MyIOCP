#pragma once
class MyIOCP:public CTextIOCPServer
{
public:
	MyIOCP(void);
	~MyIOCP(void);




	virtual VOID NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText);
};

