#include "IOCPCommon.h"
#include "IOCPBuffer.h"
#include "IOCPBufferWriter.h"
#include "IOCPBufferReader.h"
#include "BaseIOCPServer.h"
#include "PackageIOCPClient.h"
#include "TextIOCPClient.h"
#include "TextIOCPServer.h"
#pragma comment(lib,"ws2_32.lib")

CTextIOCPClient::CTextIOCPClient()
{
	WSAStartup(MAKEWORD(2,2),&m_stWSAData);

	m_bIsConnected = FALSE;
	m_bShutdown = false;

	m_hThread = (HANDLE)_beginthreadex(NULL,0,AsyncProcessThread,this,0,NULL);
}

CTextIOCPClient::~CTextIOCPClient()
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
VOID CTextIOCPClient::CloseConnect(){
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
SOCKET CTextIOCPClient::GetSockets(){
	SOCKET sd = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,NULL,0);

	if(sd != INVALID_SOCKET){
		u_long mode = 1;
		ioctlsocket(sd,FIONBIO,&mode);
	}

	return sd;
}
VOID CTextIOCPClient::AsyncConnect(u_long adr,u_short port)
{
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr = adr;
	sin.sin_port = htons(port);

	connect(m_ClientContext.sdSocketCopy,(sockaddr *)&sin,sizeof(sin));
}

VOID CTextIOCPClient::AsyncProcessReceive(){
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
	CIOCPBuffer* lpReceivedBuffer = m_ClientContext.m_ReceivedBytes.GetBuffer();

	DWORD dwBufferLength = lpReceivedBuffer->GetLength();
	BYTE* lpBuffer = lpReceivedBuffer->GetBytes();

	BYTE* lpStartPos = lpBuffer;
	DWORD dwReadingLength = 0;
	DWORD dwLastLength = dwBufferLength;

	DWORD dwStringLength = CTextIOCPServer::GetStringLen(lpStartPos,dwLastLength);

	if(dwStringLength != -1){
		do{
			char* lpszText = new char[dwStringLength];
			strncpy_s(lpszText,dwStringLength,(const char*)lpStartPos,dwStringLength);

			NotifyReceivedFormatPackage(lpszText);

			///NotifyReceivedStrings(lpPerSocketContext,lpszText);

			delete [] lpszText;

			dwReadingLength += dwStringLength;
			lpStartPos += dwStringLength;
			dwLastLength -= dwStringLength;

			dwStringLength = CTextIOCPServer::GetStringLen(lpStartPos,dwLastLength);
		}while(dwStringLength != -1);
	}

	if(dwReadingLength > 0){
		memcpy(lpReceivedBuffer->m_pData->m_pData,&lpReceivedBuffer->m_pData->m_pData[dwReadingLength],dwLastLength);
		lpReceivedBuffer->m_pData->m_dwDataLength = dwLastLength;
	}
}
VOID CTextIOCPClient::AsyncProcessSending(){
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
VOID CTextIOCPClient::AsyncProcessConnected(){
	m_bIsConnected = TRUE;
	NotifyConnectionStatus(IOCPClient_ConnectionType_Connected);

}
VOID CTextIOCPClient::AsyncProcessDisconnected(){
	if(m_bIsConnected == FALSE){
		NotifyConnectionStatus(IOCPClient_ConnectionType_ConnectFailed);
	} else {
		NotifyConnectionStatus(IOCPClient_ConnectionType_Disconnected);
		m_bIsConnected = FALSE;
	}

	CloseConnect();
}

UINT WINAPI CTextIOCPClient::AsyncProcessThread(LPVOID Param){
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	timeval timeout = {0,0};

	CTextIOCPClient* self = (CTextIOCPClient*)Param;

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

BOOL CTextIOCPClient::Connect(char* szHostName,USHORT Port)
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
VOID CTextIOCPClient::NotifyConnectionStatus(IOCPClient_ConnectionType ConnectionType)
{

}
VOID CTextIOCPClient::NotifyReceivedFormatPackage(const char* lpszBuffer)
{

}
VOID CTextIOCPClient::Send(const char* lpszText){
	CIOCPBufferWriter Writer;

	Writer.Write((const unsigned char*)lpszText,strlen(lpszText) + sizeof(char));

	m_Mutex.Lock();

	if(m_ClientContext.sdSocketCopy != INVALID_SOCKET){
		CIOCPBuffer* TempBuffer = new CIOCPBuffer(Writer.GetBuffer());
		m_ClientContext.m_SendBufferList.Push(TempBuffer);
	}

	m_Mutex.UnLock();
}