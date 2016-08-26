#include "IOCPCommon.h"
#include "IOCPBuffer.h"
#include "IOCPBufferWriter.h"
#include "IOCPBufferReader.h"
#include "BaseIOCPServer.h"
#include "PackageIOCP.h"
#include "PackageIOCPClient.h"
#pragma comment(lib,"ws2_32.lib")

PackageIOCPClient::PackageIOCPClient()
{
	WSAStartup(MAKEWORD(2,2),&m_stWSAData);

	m_bIsConnected = FALSE;
	m_bShutdown = false;

	m_hThread = (HANDLE)_beginthreadex(NULL,0,AsyncProcessThread,this,0,NULL);
}

PackageIOCPClient::~PackageIOCPClient()
{
	DWORD dwExitCode;

	m_bShutdown = true;
	WaitForSingleObject(m_hThread,10000);

	if(GetExitCodeThread(m_hThread,&dwExitCode))
	{
		if(dwExitCode != 0){
			TerminateThread(m_hThread,0);
		}
	}

	CloseHandle(m_hThread);

	if(m_bIsConnected){
		CloseConnect();
	}
}
VOID PackageIOCPClient::CloseConnect(){
	closesocket(m_ClientContext.sdSocketCopy);
	m_ClientContext.sdSocketCopy = INVALID_SOCKET;

	while(m_ClientContext.m_SendBufferList.Empty() == false){
		delete m_ClientContext.m_SendBufferList.Pop();
	}

	if(m_ClientContext.m_SendingBytes){
		delete m_ClientContext.m_SendingBytes;
		m_ClientContext.m_SendingBytes = NULL;
	}

	m_ClientContext.m_ReceivedBytes.GetBuffer()->m_pData->m_dwDataLength = 0;
}
SOCKET PackageIOCPClient::GetSockets(){
	SOCKET sd = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,NULL,0);

	if(sd != INVALID_SOCKET){
		u_long mode = 1;
		ioctlsocket(sd,FIONBIO,&mode);
	}
	
	return sd;
}
VOID PackageIOCPClient::AsyncConnect(u_long adr,u_short port)
{
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr = adr;
	sin.sin_port = htons(port);

	connect(m_ClientContext.sdSocketCopy,(sockaddr *)&sin,sizeof(sin));
}

VOID PackageIOCPClient::AsyncProcessReceive(){
	char buf[0x1000];
	int buf_len = recv(m_ClientContext.sdSocketCopy,buf,sizeof(buf),0);

	if(buf_len <= 0){
		if(WSAGetLastError() == WSAEWOULDBLOCK){
			return;
		}

		CloseConnect();
		return;
	}

	m_ClientContext.m_ReceivedBytes.Write((const unsigned char*)&buf,buf_len);

	while(true){
		CIOCPBuffer* lpBuffer = m_ClientContext.m_ReceivedBytes.GetBuffer();

		//等待下次接受数据
		if(lpBuffer->GetLength() < sizeof(NETPackage)){
			return;
		}

		NETPackage* Package = (NETPackage*)lpBuffer->GetBytes();
		//包数据大小
		if(Package->length < lpBuffer->GetLength()){
			return;
		}
		//非法的数据
		if(Package->title != NET_PACKAGE_TITLE){
			CloseConnect();
			return;
		}

		CIOCPBuffer NotifBuffer;
		NotifBuffer.Append(&lpBuffer->GetBytes()[sizeof(NETPackage)],(Package->length - sizeof(NETPackage)));

		NotifyReceivedFormatPackage(&NotifBuffer);

		memcpy(lpBuffer->GetBytes(),
			&lpBuffer->GetBytes()[Package->length],
			lpBuffer->GetLength() - Package->length);
		lpBuffer->m_pData->m_dwDataLength = lpBuffer->GetLength() - Package->length;
	}
}
VOID PackageIOCPClient::AsyncProcessSending(){
	if(m_ClientContext.m_SendBufferList.Empty() == TRUE && m_ClientContext.m_SendingBytes == NULL){
		return;
	}

	if(m_ClientContext.m_SendingBytes == NULL){
		CIOCPBuffer* lpBuffer = m_ClientContext.m_SendBufferList.Pop();
		if(lpBuffer){
			m_ClientContext.m_SendingBytes = new CIOCPBufferReader(lpBuffer);
			OP_DELETE<CIOCPBuffer>(lpBuffer,_FILE_AND_LINE_);
		}
	}

	if(m_ClientContext.m_SendingBytes == NULL){
		return;
	}

	CIOCPBufferReader* lpBufferReader = m_ClientContext.m_SendingBytes;
	
	BYTE* lpData = &lpBufferReader->GetBytes()[lpBufferReader->m_nPosition];
	DWORD dwLength = lpBufferReader->GetLength() - lpBufferReader->m_nPosition;

	//DumpBuffer(lpData,dwLength);

	int SentLength = send(m_ClientContext.sdSocketCopy,(const char*)lpData,dwLength,0);

	if(SentLength <= 0){
		if(SentLength == -1){
			if(WSAGetLastError() == WSAEWOULDBLOCK){
				return;
			}
		}
		AsyncProcessDisconnected();
		return;
	}
	lpBufferReader->m_nPosition += SentLength;

	if(lpBufferReader->m_nPosition == lpBufferReader->GetLength()){
		delete m_ClientContext.m_SendingBytes;
		m_ClientContext.m_SendingBytes = NULL;
	}
}
VOID PackageIOCPClient::AsyncProcessConnected(){
	m_bIsConnected = TRUE;
	NotifyConnectionStatus(IOCPClient_ConnectionType_Connected);
	
}
VOID PackageIOCPClient::AsyncProcessDisconnected(){
	if(m_bIsConnected == FALSE){
		NotifyConnectionStatus(IOCPClient_ConnectionType_ConnectFailed);
	} else {
		NotifyConnectionStatus(IOCPClient_ConnectionType_Disconnected);
		m_bIsConnected = FALSE;
	}

	CloseConnect();
}

UINT WINAPI PackageIOCPClient::AsyncProcessThread(LPVOID Param){
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	timeval timeout = {0,0};
	
	PackageIOCPClient* self = (PackageIOCPClient*)Param;
	
	while(self->m_bShutdown == false){

		if(self->m_ClientContext.sdSocketCopy != INVALID_SOCKET){
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_ZERO(&exceptfds);

#define _SOCKET_FD_CURRENT_ self->m_ClientContext.sdSocketCopy

			FD_SET(_SOCKET_FD_CURRENT_,&readfds);
			FD_SET(_SOCKET_FD_CURRENT_,&writefds);
			FD_SET(_SOCKET_FD_CURRENT_,&exceptfds);

			int result = select(_SOCKET_FD_CURRENT_ + 1,&readfds,&writefds,&exceptfds,&timeout);

			//what the fuck?
			if(result == SOCKET_ERROR){
				return -1;
			}

			self->m_Mutex.Lock();
			if(result > 0){
				if(FD_ISSET(_SOCKET_FD_CURRENT_,&exceptfds)){
					self->AsyncProcessDisconnected();
					self->m_Mutex.UnLock();
					continue;
				}

				if(FD_ISSET(_SOCKET_FD_CURRENT_,&writefds)){
					
					if(self->m_bIsConnected == FALSE){
						self->AsyncProcessConnected();
					}

					self->AsyncProcessSending();
				}

				if(FD_ISSET(_SOCKET_FD_CURRENT_,&readfds)){
					self->AsyncProcessReceive();
				}
			}
			self->m_Mutex.UnLock();

#undef _SOCKET_FD_CURRENT_
		}
		
		Sleep(30);
	}

	return 0;
}

BOOL PackageIOCPClient::Connect(char* szHostName,USHORT Port)
{
	struct hostent * serverent;


	if(m_ClientContext.sdSocketCopy != INVALID_SOCKET){
		return FALSE;
	}
	serverent = gethostbyname(szHostName);
	if (serverent == NULL)
	{
		return FALSE;
	}
	if(serverent->h_addr_list == NULL)
	{
		return FALSE;
	}
	in_addr adr;

	memcpy((char *)&adr, (char *)serverent->h_addr, serverent->h_length);
	m_ClientContext.sdSocketCopy = GetSockets();

	AsyncConnect(adr.S_un.S_addr,Port);

	return TRUE;
}
VOID PackageIOCPClient::NotifyConnectionStatus(IOCPClient_ConnectionType ConnectionType)
{

}
VOID PackageIOCPClient::NotifyReceivedFormatPackage(CIOCPBuffer* lpBuffer)
{

}

VOID PackageIOCPClient::Send(CIOCPBuffer buffer){
	NETPackage info;
	CIOCPBufferWriter Writer;

	info.length = buffer.GetLength() + sizeof(NETPackage);
	info.title = NET_PACKAGE_TITLE;

	Writer.Write(info);
	Writer.Write(buffer);

	m_Mutex.Lock();

	if(m_ClientContext.sdSocketCopy != INVALID_SOCKET){
		m_ClientContext.m_SendBufferList.Push(new CIOCPBuffer(Writer.GetBuffer()));
	}

	m_Mutex.UnLock();
}

VOID PackageIOCPClient::Send(BYTE* Buffer,DWORD Size){
	NETPackage info;
	CIOCPBufferWriter Writer;

	info.length = Size + sizeof(NETPackage);
	info.title = NET_PACKAGE_TITLE;

	Writer.Write(info);
	Writer.Write(Buffer,Size);

	m_Mutex.Lock();

	if(m_ClientContext.sdSocketCopy != INVALID_SOCKET){
		CIOCPBuffer* TempBuffer = new CIOCPBuffer(Writer.GetBuffer());
		m_ClientContext.m_SendBufferList.Push(TempBuffer);
	}

	m_Mutex.UnLock();
}