#ifndef _TEXTIOCPCLIENT_H_
#define _TEXTIOCPCLIENT_H_


class CTextIOCPClient
{
public:
	WSADATA m_stWSAData;
	IOCPMutex m_Mutex;
	IOCPClientContext m_ClientContext;
	sockaddr_in m_stRemoteAddress;
	BOOL m_bIsConnected;
	bool m_bShutdown;
private:
	HANDLE m_hThread;
	VOID CloseConnect();
	SOCKET GetSockets();
	VOID AsyncConnect(u_long adr,u_short port);
	VOID AsyncProcessReceive();
	VOID AsyncProcessSending();
	VOID AsyncProcessConnected();
	VOID AsyncProcessDisconnected();
	static UINT WINAPI AsyncProcessThread(LPVOID Param);
public:
	CTextIOCPClient();
	virtual ~CTextIOCPClient();

	BOOL Connect(char* szHostName,USHORT Port);

	VOID Send(const char* lpszText);


public:
	//用户连接状态
	virtual VOID NotifyConnectionStatus(IOCPClient_ConnectionType ConnectionType);
	virtual VOID NotifyReceivedFormatPackage(const char* lpszBuffer);
};

#endif