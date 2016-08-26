#pragma once
class MyIOCP:public CTextIOCPClient
{
public:
	MyIOCP(void);
	~MyIOCP(void);

	virtual VOID NotifyConnectionStatus(IOCPClient_ConnectionType ConnectionType);
	VOID NotifyReceivedFormatPackage(const char* lpszBuffer);
};

