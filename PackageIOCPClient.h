#ifndef _IOCPCLIENT_H_
#define _IOCPCLIENT_H_

/*
	A Simple IOCP Class
	Author:201724
	Email:number201724@me.com
*/

typedef enum IOCPClient_ConnectionType
{
	IOCPClient_ConnectionType_ConnectFailed,
	IOCPClient_ConnectionType_Connected,
	IOCPClient_ConnectionType_Disconnected
}IOCPClient_ConnectionType;


class IOCPClientContext:public IOCPMutex
{
public:
	SOCKET sdSocketCopy;

	CIOCPBufferWriter m_ReceivedBytes;
	IOCPQueue<CIOCPBuffer*> m_SendBufferList;

	CIOCPBufferReader* m_SendingBytes;

	IOCPClientContext(){
		sdSocketCopy = INVALID_SOCKET;
		m_SendingBytes = NULL;
	}
};

class PackageIOCPClient
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
	PackageIOCPClient();
	virtual ~PackageIOCPClient();

	BOOL Connect(char* szHostName,USHORT Port);

	VOID Send(CIOCPBuffer buffer);
	VOID Send(BYTE* Buffer,DWORD Size);


public:
	//用户连接状态
	virtual VOID NotifyConnectionStatus(IOCPClient_ConnectionType ConnectionType);

	virtual VOID NotifyReceivedFormatPackage(CIOCPBuffer* lpBuffer);
};

#endif