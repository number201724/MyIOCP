#include "IOCPCommon.h"
#include "IOCPBuffer.h"
#include "IOCPBufferWriter.h"
#include "IOCPBufferReader.h"
#include "BaseIOCPServer.h"

#pragma comment(lib,"ws2_32.lib")


static GUID WSAID_AcceptEx = WSAID_ACCEPTEX;
static GUID WSAID_GetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;

#define BASE_IOCP_RECEIVE_BUFFER_SIZE 0x1000

int myprintf(const char *lpFormat, ...) {

	int nLen = 0;
	int nRet = 0;
	char cBuffer[512];
	va_list arglist;
	HANDLE hOut = NULL;

	ZeroMemory(cBuffer, sizeof(cBuffer));

	va_start(arglist, lpFormat);

	nLen = lstrlenA(lpFormat);
	nRet = _vsnprintf(cBuffer, 512, lpFormat, arglist);

	if (nRet != -1)
	{
		if (nRet >= nLen || GetLastError() == 0) {
			hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hOut != INVALID_HANDLE_VALUE)
				WriteConsoleA(hOut, cBuffer, lstrlenA(cBuffer), (LPDWORD)&nLen, NULL);
		}
		return nLen;
	}

	char *buff = 0;
	size_t buffSize = 8096;
	buff = (char *)malloc(buffSize);
	while (1)
	{
		buff = (char*)realloc(buff, buffSize);
		nRet = _vsnprintf(buff, buffSize, lpFormat, arglist);

		if (nRet != -1)
		{
			break;
		}
		buffSize *= 2;
	}

	if (nRet >= nLen || GetLastError() == 0) {
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut != INVALID_HANDLE_VALUE)
			WriteConsoleA(hOut, buff, lstrlenA(buff), (LPDWORD)&nLen, NULL);
	}

	free(buff);


	return nLen;
}
_PER_IO_CONTEXT::_PER_IO_CONTEXT() {
	memset(&Overlapped, 0, sizeof(Overlapped));
	memset(&wsabuf, 0, sizeof(wsabuf));
	SocketAccept = INVALID_SOCKET;
	IOCPBuffer = NULL;
	BufferReader = NULL;
}
_PER_SOCKET_CONTEXT::_PER_SOCKET_CONTEXT() {
	QueryPerformanceCounter((LARGE_INTEGER*)&m_guid);
	m_NumberOfPendingIO = 0;
	m_Socket = INVALID_SOCKET;
}
_PER_SOCKET_CONTEXT::~_PER_SOCKET_CONTEXT()
{
	if (m_RecvContext.IOCPBuffer != NULL) {
		OP_DELETE<CIOCPBuffer>(m_RecvContext.IOCPBuffer, _FILE_AND_LINE_);
		m_RecvContext.IOCPBuffer = NULL;
	}

	if (m_SendContext.BufferReader != NULL) {
		OP_DELETE<CIOCPBufferReader>(m_SendContext.BufferReader, _FILE_AND_LINE_);
		m_SendContext.BufferReader = NULL;
	}

	while (!m_SendBufferList.Empty()) {
		OP_DELETE<CIOCPBuffer>(m_SendBufferList.Pop(), _FILE_AND_LINE_);
	}
}
void _PER_SOCKET_CONTEXT_LIST::AddContext(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	m_ContextLock.Lock();

	m_vContextMap[lpPerSocketContext->m_guid] = lpPerSocketContext;

	lpPerSocketContext->pos = m_vContextList.insert(m_vContextList.end(), lpPerSocketContext);

	m_ContextLock.UnLock();
}

void _PER_SOCKET_CONTEXT_LIST::DeleteContext(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	std::map<LONGLONG, PPER_SOCKET_CONTEXT>::iterator map_iter;
	m_ContextLock.Lock();
	map_iter = m_vContextMap.find(lpPerSocketContext->m_guid);
	if (map_iter != m_vContextMap.end()) {
		m_vContextMap.erase(map_iter);
		m_vContextList.erase(lpPerSocketContext->pos);
	}
	m_ContextLock.UnLock();
}
void _PER_SOCKET_CONTEXT_LIST::ClearAll()
{
	m_ContextLock.Lock();
	m_vContextMap.clear();
	m_vContextList.clear();
	m_ContextLock.UnLock();
}
PPER_SOCKET_CONTEXT _PER_SOCKET_CONTEXT_LIST::GetContext(LONGLONG guid)
{
	PPER_SOCKET_CONTEXT lpPerSocketContext = NULL;
	std::map<LONGLONG, PPER_SOCKET_CONTEXT>::iterator map_iter;

	map_iter = m_vContextMap.find(guid);
	if (map_iter != m_vContextMap.end()) {
		lpPerSocketContext = map_iter->second;
	}

	return lpPerSocketContext;
}
CBaseIOCPServer::CBaseIOCPServer(void)
{
	WSAStartup(MAKEWORD(2, 2), &m_WSAData);
	m_AcceptEx = NULL;
	m_Listen = INVALID_SOCKET;
	m_IOCP = INVALID_HANDLE_VALUE;
	m_ThreadHandles = NULL;
	m_ThreadHandleCount = 0;
	m_CurrentConnectCount = 0;
	m_LimitConnectCount = 0;
}


CBaseIOCPServer::~CBaseIOCPServer(void) {
	Shutdown();
}

ULONG CBaseIOCPServer::EnterIOLoop(PPER_SOCKET_CONTEXT lpPerSocketContext) {
	return InterlockedIncrement(&lpPerSocketContext->m_NumberOfPendingIO);
}
ULONG CBaseIOCPServer::ExitIOLoop(PPER_SOCKET_CONTEXT lpPerSocketContext) {
	return InterlockedDecrement(&lpPerSocketContext->m_NumberOfPendingIO);
}
VOID CBaseIOCPServer::UpdateCompletionPort(SOCKET hSocket, DWORD_PTR lpCompletionKey)
{
	m_IOCP = CreateIoCompletionPort((HANDLE)hSocket, m_IOCP, (DWORD_PTR)lpCompletionKey, 0);
}
VOID CBaseIOCPServer::UpdateSocket(SOCKET sd) {
	int nRet = setsockopt(
		sd,
		SOL_SOCKET,
		SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&m_Listen,
		sizeof(m_Listen)
	);

	if (nRet == SOCKET_ERROR) {
		//
		//just warn user here.
		//
		myprintf("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket failed: %d\n", WSAGetLastError());
	}
}
FARPROC CBaseIOCPServer::GetExtensionProcAddress(GUID& Guid)
{
	FARPROC lpfn = NULL;

	DWORD dwBytes = 0;

	WSAIoctl(
		m_Listen,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&Guid,
		sizeof(Guid),
		&lpfn,
		sizeof(lpfn),
		&dwBytes,
		NULL,
		NULL);


	return lpfn;
}
SOCKET CBaseIOCPServer::CreateSocket(void)
{
	int nRet = 0;
	int nZero = 0;
	SOCKET sdSocket = INVALID_SOCKET;

	sdSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sdSocket == INVALID_SOCKET) {
		myprintf("WSASocket(sdSocket) failed: %d\n", WSAGetLastError());
		return sdSocket;
	}

	nZero = 0;
	nRet = setsockopt(sdSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	if (nRet == SOCKET_ERROR) {
		myprintf("setsockopt(SNDBUF) failed: %d\n", WSAGetLastError());
		return sdSocket;
	}

	return sdSocket;
}

BOOL CBaseIOCPServer::CreateListenSocket(USHORT nPort) {
	int nRet = 0;
	sockaddr_in hints = { 0 };

	m_Listen = CreateSocket();
	if (m_Listen == INVALID_SOCKET) {
		return FALSE;
	}

	hints.sin_family = AF_INET;
	hints.sin_addr.S_un.S_addr = 0;
	hints.sin_port = htons(nPort);


	nRet = bind(m_Listen, (sockaddr *)&hints, sizeof(sockaddr_in));
	if (nRet == SOCKET_ERROR) {
		closesocket(m_Listen);
		m_Listen = INVALID_SOCKET;
		myprintf("bind() failed: %d\n", WSAGetLastError());
		return FALSE;
	}

	nRet = listen(m_Listen, SOMAXCONN);
	if (nRet == SOCKET_ERROR) {
		closesocket(m_Listen);
		m_Listen = INVALID_SOCKET;
		myprintf("listen() failed: %d\n", WSAGetLastError());
		return FALSE;
	}

	*(FARPROC*)&m_AcceptEx = GetExtensionProcAddress(WSAID_AcceptEx);
	return TRUE;
}


BOOL CBaseIOCPServer::CreateAcceptSocket(PPER_IO_CONTEXT lpPerIOContext) {
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;

#define ACCEPTEX_BUFFER_SIZE ((sizeof(sockaddr_in) + 16) * 2)

	if (!lpPerIOContext) {
		lpPerIOContext = OP_NEW<PER_IO_CONTEXT>(_FILE_AND_LINE_);
		lpPerIOContext->IOCPBuffer = OP_NEW_1<CIOCPBuffer, DWORD>(_FILE_AND_LINE_, ACCEPTEX_BUFFER_SIZE);
		m_AcceptIOContext.push_back(lpPerIOContext);
	}
	//AcceptEx和普通的accept函数不一样
	//需要先准备好SOCKET
	lpPerIOContext->SocketAccept = CreateSocket();
	lpPerIOContext->IOOperation = ClientIoAccept;

	memset(lpPerIOContext->IOCPBuffer->m_pData->m_pData, 0, lpPerIOContext->IOCPBuffer->m_pData->m_dwDataLength);
	memset(&lpPerIOContext->Overlapped, 0, sizeof(lpPerIOContext->Overlapped));

	nRet = m_AcceptEx(m_Listen,
		lpPerIOContext->SocketAccept,
		lpPerIOContext->IOCPBuffer->m_pData->m_pData,
		0,
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		&dwRecvNumBytes,
		&lpPerIOContext->Overlapped);

	if (nRet == FALSE && (ERROR_IO_PENDING != WSAGetLastError())) {
		myprintf("AcceptEx() failed: %d\n", WSAGetLastError());
		return FALSE;
	}

#undef ACCEPTEX_BUFFER_SIZE
	return TRUE;
}

PPER_SOCKET_CONTEXT CBaseIOCPServer::AllocateSocketContext()
{
	PPER_SOCKET_CONTEXT lpPerSocketContext = OP_NEW<PER_SOCKET_CONTEXT>(_FILE_AND_LINE_);

	EnterIOLoop(lpPerSocketContext);

	return lpPerSocketContext;
}
VOID CBaseIOCPServer::ReleaseSocketContext(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	if (ExitIOLoop(lpPerSocketContext) <= 0)
	{
		OP_DELETE<PER_SOCKET_CONTEXT>(lpPerSocketContext, _FILE_AND_LINE_);
		m_ContextList.DeleteContext(lpPerSocketContext);
	}
}
VOID CBaseIOCPServer::CloseClient(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	lpPerSocketContext->m_Lock.Lock();
	if (lpPerSocketContext->m_Socket != INVALID_SOCKET)
	{
		NotifyDisconnectedClient(lpPerSocketContext);

		InterlockedDecrement(&m_CurrentConnectCount);

		LINGER  lingerStruct;
		lingerStruct.l_onoff = 1;
		lingerStruct.l_linger = 0;
		//强制关闭用户连接
		setsockopt(lpPerSocketContext->m_Socket, SOL_SOCKET, SO_LINGER,
			(char *)&lingerStruct, sizeof(lingerStruct));

		closesocket(lpPerSocketContext->m_Socket);
		lpPerSocketContext->m_Socket = INVALID_SOCKET;
	}
	lpPerSocketContext->m_Lock.UnLock();
}

VOID CBaseIOCPServer::PostClientIoRead(PPER_SOCKET_CONTEXT lpPerSocketContext, PPER_IO_CONTEXT lpPerIOContext, IO_POST_RESULT& PostResult)
{
	DWORD dwFlags = 0;
	DWORD dwNumberRecvd = 0;

	lpPerSocketContext->m_Lock.Lock();

	PostResult = PostIoSuccess;

	if (lpPerSocketContext->m_Socket == INVALID_SOCKET)
	{
		PostResult = PostIoInvalicSocket;
		goto _End;
	}

	lpPerIOContext->IOOperation = ClientIoRead;						//定义消息类型
	lpPerIOContext->SocketAccept = lpPerSocketContext->m_Socket;

	//投递接收操作
	if (lpPerIOContext->IOCPBuffer->GetLength() < IOCP_SWAP_BUFFER_SIZE) {
		lpPerIOContext->IOCPBuffer->Reallocate(IOCP_SWAP_BUFFER_SIZE);
	}
	lpPerIOContext->wsabuf.len = IOCP_SWAP_BUFFER_SIZE;
	lpPerIOContext->wsabuf.buf = (CHAR*)lpPerIOContext->IOCPBuffer->GetBytes();

	if (WSARecv(lpPerIOContext->SocketAccept, &lpPerIOContext->wsabuf, 1, &dwNumberRecvd, &dwFlags, &lpPerIOContext->Overlapped, NULL) == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		PostResult = PostIoFailed;
	}

_End:
	lpPerSocketContext->m_Lock.UnLock();
}

VOID CBaseIOCPServer::PostClientIoWrite(PPER_SOCKET_CONTEXT lpPerSocketContext, PPER_IO_CONTEXT lpPerIOContext, IO_POST_RESULT& PostResult)
{
	DWORD dwFlags = 0;
	DWORD dwNumberSent = 0;

	lpPerSocketContext->m_Lock.Lock();

	PostResult = PostIoSuccess;

	if (lpPerSocketContext->m_Socket == INVALID_SOCKET)
	{
		PostResult = PostIoInvalicSocket;
		goto _End;
	}

	//设置IO对象
	lpPerIOContext->IOOperation = ClientIoWrite;
	lpPerIOContext->SocketAccept = lpPerSocketContext->m_Socket;

	//计算出应该发送多少字节
	DWORD nTotalBytes = lpPerIOContext->BufferReader->GetBuffer()->GetLength();
	DWORD nSentBytes = nTotalBytes - lpPerIOContext->BufferReader->m_nPosition;

	//计算到发送缓冲区的位置并且设置长度
	lpPerIOContext->wsabuf.len = nSentBytes;
	lpPerIOContext->wsabuf.buf = (CHAR*)&lpPerIOContext->BufferReader->GetBuffer()->GetBytes()[lpPerIOContext->BufferReader->m_nPosition];
	//投递发送消息
	if (WSASend(lpPerIOContext->SocketAccept, &lpPerIOContext->wsabuf, 1, &dwNumberSent, dwFlags, &lpPerIOContext->Overlapped, NULL) == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		PostResult = PostIoFailed;
	}

_End:
	lpPerSocketContext->m_Lock.UnLock();
}

VOID CBaseIOCPServer::OnClientIoAccept(PPER_SOCKET_CONTEXT lpPerSocketContext, PPER_IO_CONTEXT lpPerIOContext, DWORD dwIoSize)
{
	/*这是WinSock2的一个坑!!!*/
	UpdateSocket(lpPerIOContext->SocketAccept);		//一定要先UpdateSocket,否则下次AcceptEx会有几率出现莫名其妙的问题……

	if (InterlockedIncrement(&m_CurrentConnectCount) > m_LimitConnectCount)
	{
		InterlockedDecrement(&m_CurrentConnectCount);
		closesocket(lpPerIOContext->SocketAccept);
		CreateAcceptSocket(lpPerIOContext);
		return;
	}

	IO_POST_RESULT PostResult;


	//句柄保存出来,并且立即投递下一个AcceptEx操作
	SOCKET SocketAccept = lpPerIOContext->SocketAccept;

	//立刻投递下一个AcceptEx
	CreateAcceptSocket(lpPerIOContext);

	//用户连接处理
	PPER_SOCKET_CONTEXT lpPerAcceptSocketContext = AllocateSocketContext();
	lpPerAcceptSocketContext->m_Socket = SocketAccept;
	m_ContextList.AddContext(lpPerAcceptSocketContext);
	UpdateCompletionPort(lpPerAcceptSocketContext->m_Socket, (DWORD_PTR)lpPerAcceptSocketContext);

	NotifyNewConnection(lpPerAcceptSocketContext);

	PPER_IO_CONTEXT lpPerRecvIOContext = &lpPerAcceptSocketContext->m_RecvContext;
	lpPerRecvIOContext->IOCPBuffer = OP_NEW_1<CIOCPBuffer, DWORD>(_FILE_AND_LINE_, BASE_IOCP_RECEIVE_BUFFER_SIZE);

	PostClientIoRead(lpPerAcceptSocketContext, lpPerRecvIOContext, PostResult);

	//如果投递成功,则直接返回即可
	if (PostResult == PostIoSuccess) {
		return;
	}

	//这里投递失败的话,直接关闭SOCKET且释放PER_SOCKET_CONTEXT结构
	if (PostResult == PostIoFailed) {
		CloseClient(lpPerAcceptSocketContext);
		ReleaseSocketContext(lpPerAcceptSocketContext);
	}
}

VOID CBaseIOCPServer::OnClientIoRead(PPER_SOCKET_CONTEXT lpPerSocketContext, PPER_IO_CONTEXT lpPerIOContext, DWORD dwIoSize)
{
	IO_POST_RESULT PostResult;

	//收到数据,设置缓冲区,并且通知用户已经收到了数据
	lpPerIOContext->IOCPBuffer->m_pData->m_dwDataLength = dwIoSize;

	NotifyReceivedPackage(lpPerSocketContext, lpPerIOContext->IOCPBuffer);
	//更换缓冲区
	OP_DELETE<CIOCPBuffer>(lpPerIOContext->IOCPBuffer, _FILE_AND_LINE_);
	lpPerIOContext->IOCPBuffer = OP_NEW_1<CIOCPBuffer, DWORD>(_FILE_AND_LINE_, BASE_IOCP_RECEIVE_BUFFER_SIZE);

	PostClientIoRead(lpPerSocketContext, lpPerIOContext, PostResult);

	//如果投递成功,则直接返回即可
	if (PostResult == PostIoSuccess) {
		return;
	}

	//这里投递失败的话,直接关闭SOCKET且释放PER_SOCKET_CONTEXT结构
	if (PostResult == PostIoFailed || PostResult == PostIoInvalicSocket) {
		CloseClient(lpPerSocketContext);
		ReleaseSocketContext(lpPerSocketContext);
	}
}
VOID CBaseIOCPServer::OnClientIoWrite(PPER_SOCKET_CONTEXT lpPerSocketContext, PPER_IO_CONTEXT lpPerIOContext, DWORD dwIoSize)
{
	lpPerIOContext->BufferReader->m_nPosition += dwIoSize;		//增加已经写入的缓冲区长度

	NotifyWritePackage(lpPerSocketContext, lpPerIOContext->BufferReader->GetBuffer());

	if (lpPerIOContext->BufferReader->m_nPosition >= lpPerIOContext->BufferReader->GetLength())		//缓冲区发送完毕,通知用户回调,且更换下一块数据继续发送
	{
		NotifyWriteCompleted(lpPerSocketContext, lpPerIOContext->BufferReader->GetBuffer());

		//最小锁定时间
		lpPerSocketContext->m_Lock.Lock();

		OP_DELETE<CIOCPBufferReader>(lpPerIOContext->BufferReader, _FILE_AND_LINE_);
		lpPerIOContext->BufferReader = NULL;

		//切换下一块缓冲区
		CIOCPBuffer* lpBuffer = lpPerSocketContext->m_SendBufferList.Pop();
		if (lpBuffer) {

			lpPerIOContext->BufferReader = OP_NEW_1<CIOCPBufferReader, CIOCPBuffer*>(_FILE_AND_LINE_, lpBuffer);

			OP_DELETE<CIOCPBuffer>(lpBuffer, _FILE_AND_LINE_);
		}

		lpPerSocketContext->m_Lock.UnLock();
	}

	if (lpPerIOContext->BufferReader)
	{
		IO_POST_RESULT PostResult;

		PostClientIoWrite(lpPerSocketContext, lpPerIOContext, PostResult);

		//如果投递成功,则直接返回即可
		if (PostResult == PostIoSuccess) {
			return;
		}

		//这里投递失败的话,直接关闭SOCKET且释放PER_SOCKET_CONTEXT结构
		if (PostResult == PostIoFailed || PostResult == PostIoInvalicSocket) {
			CloseClient(lpPerSocketContext);
			ReleaseSocketContext(lpPerSocketContext);
		}
	}
	else
	{
		ReleaseSocketContext(lpPerSocketContext);
	}
}

DWORD WINAPI CBaseIOCPServer::WorkerThread(LPVOID Param)
{
	BOOL bSuccess;
	DWORD dwIoSize;
	DWORD_PTR lpCompletionKey;
	PPER_IO_CONTEXT lpPerIOContext = NULL;

	CBaseIOCPServer* pThis = (CBaseIOCPServer*)Param;
	while (TRUE)
	{
		bSuccess = GetQueuedCompletionStatus(
			pThis->m_IOCP,
			&dwIoSize,
			(PDWORD_PTR)&lpCompletionKey,
			(LPOVERLAPPED *)&lpPerIOContext,
			INFINITE
		);


		//如果投递的是NULL消息则为退出消息
		if (lpCompletionKey == NULL) {
			return 0;
		}

		//断开用户的连接
		if (!bSuccess || (bSuccess && (dwIoSize == 0)) && lpCompletionKey != (DWORD_PTR)pThis) {
			pThis->CloseClient((PPER_SOCKET_CONTEXT)lpCompletionKey);
			pThis->ReleaseSocketContext((PPER_SOCKET_CONTEXT)lpCompletionKey);

			continue;
		}

		//分发不同的IOCP处理函数
		switch (lpPerIOContext->IOOperation) {
		case ClientIoAccept:
			pThis->OnClientIoAccept((PPER_SOCKET_CONTEXT)lpCompletionKey, lpPerIOContext, dwIoSize);
			break;
		case ClientIoRead:
			pThis->OnClientIoRead((PPER_SOCKET_CONTEXT)lpCompletionKey, lpPerIOContext, dwIoSize);
			break;
		case ClientIoWrite:
			pThis->OnClientIoWrite((PPER_SOCKET_CONTEXT)lpCompletionKey, lpPerIOContext, dwIoSize);
			break;
		}
	}
	return 0;
}

BOOL CBaseIOCPServer::Startup(USHORT nPort, DWORD dwWorkerThreadCount, DWORD dwMaxConnection)
{
	if (m_Listen != INVALID_SOCKET) {
		return FALSE;
	}
	//创建一个用于监听的SOCKET
	if (CreateListenSocket(nPort) == FALSE) {
		return FALSE;
	}

	m_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	UpdateCompletionPort(m_Listen, (DWORD_PTR)this);

	//创建一些AcceptEx消息并投递到IOCP
	//AcceptEx是IOCP模型下的高效接收函数
	//支持多线程接收客户的连接
	for (DWORD dwThreadIndex = 0; dwThreadIndex < dwWorkerThreadCount; dwThreadIndex++) {
		CreateAcceptSocket(NULL);
	}

	//服务器最大连接数
	m_LimitConnectCount = dwMaxConnection;

	//创建IOCP线程
	m_ThreadHandles = OP_NEW_ARRAY<HANDLE>(dwWorkerThreadCount, _FILE_AND_LINE_);
	m_ThreadHandleCount = dwWorkerThreadCount;

	for (DWORD dwThreadIndex = 0; dwThreadIndex < dwWorkerThreadCount; dwThreadIndex++) {
		m_ThreadHandles[dwThreadIndex] = CreateThread(NULL, NULL, WorkerThread, this, NULL, NULL);
	}

	return TRUE;
}
BOOL CBaseIOCPServer::Shutdown()
{
	if (m_Listen == INVALID_SOCKET) {
		return FALSE;
	}

	CancelIo((HANDLE)m_Listen);

	//等待系统IO操作完成
	for (size_t i = 0; i < m_AcceptIOContext.size(); i++) {
		PPER_IO_CONTEXT lpPerIOContext = m_AcceptIOContext[i];

		while (!HasOverlappedIoCompleted(&lpPerIOContext->Overlapped))
			Sleep(1);

		//释放资源
		closesocket(lpPerIOContext->SocketAccept);
		lpPerIOContext->SocketAccept = INVALID_SOCKET;

		OP_DELETE<CIOCPBuffer>(lpPerIOContext->IOCPBuffer, _FILE_AND_LINE_);
		OP_DELETE<PER_IO_CONTEXT>(lpPerIOContext, _FILE_AND_LINE_);
	}

	//取消客户的所有IO
	for (std::list<PPER_SOCKET_CONTEXT>::iterator iter = m_ContextList.m_vContextList.begin();
		iter != m_ContextList.m_vContextList.end(); iter++) {
		DWORD dwNumberBytes = 0;
		PPER_SOCKET_CONTEXT lpPerSocketContext = (*iter);

		//关闭客户端的连接并取消所有IO
		CloseClient(lpPerSocketContext);
	}

	//判断所有SOCKET的IO是否已经完成
	for (std::list<PPER_SOCKET_CONTEXT>::iterator iter = m_ContextList.m_vContextList.begin();
		iter != m_ContextList.m_vContextList.end(); iter++) {
		DWORD dwNumberBytes = 0;
		PPER_SOCKET_CONTEXT lpPerSocketContext = (*iter);

		//等待系统IO操作完成,某些系统IO操作没有完成会蓝屏
		//IO操作没有完成的话会导致堆损坏
		//这里如果没有投递IOCP的话,HasOverlappedIoCompleted会一直返回失败,所以检查IOCPBuffer是否有值,有则代表已经投递
		while (lpPerSocketContext->m_RecvContext.IOCPBuffer && !HasOverlappedIoCompleted(&lpPerSocketContext->m_RecvContext.Overlapped))
			Sleep(1);

		while (lpPerSocketContext->m_SendContext.IOCPBuffer && !HasOverlappedIoCompleted(&lpPerSocketContext->m_SendContext.Overlapped))
			Sleep(1);

		OP_DELETE<PER_SOCKET_CONTEXT>(lpPerSocketContext, _FILE_AND_LINE_);
	}

	//安全的让线程退出
	for (DWORD i = 0; i < m_ThreadHandleCount; i++) {
		PostQueuedCompletionStatus(m_IOCP, 0, NULL, NULL);
	}

	WaitForMultipleObjects(m_ThreadHandleCount, m_ThreadHandles, TRUE, INFINITE);

	for (DWORD i = 0; i < m_ThreadHandleCount; i++) {
		if (m_ThreadHandles[i] != INVALID_HANDLE_VALUE)
			CloseHandle(m_ThreadHandles[i]);
		m_ThreadHandles[i] = INVALID_HANDLE_VALUE;
	}

	OP_DELETE_ARRAY<HANDLE>(m_ThreadHandles, _FILE_AND_LINE_);
	m_ThreadHandles = NULL;

	m_AcceptIOContext.clear();
	m_ContextList.ClearAll();



	//关闭监听套接字
	if (m_Listen != INVALID_SOCKET) {
		closesocket(m_Listen);
		m_Listen = INVALID_SOCKET;
	}

	CloseHandle(m_IOCP);
	m_IOCP = INVALID_HANDLE_VALUE;


	//重置所有信息
	m_AcceptEx = NULL;
	m_Listen = INVALID_SOCKET;
	m_IOCP = INVALID_HANDLE_VALUE;
	m_ThreadHandles = NULL;
	m_ThreadHandleCount = 0;
	m_CurrentConnectCount = 0;
	m_LimitConnectCount = 0;

	return TRUE;
}

BOOL CBaseIOCPServer::Send(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* lpIOCPBuffer)
{
	IO_POST_RESULT PostResult;
	BOOL bRet = FALSE;
	lpPerSocketContext->m_Lock.Lock();

	if (lpPerSocketContext->m_Socket != INVALID_SOCKET) {
		if (lpPerSocketContext->m_SendContext.BufferReader) {
			lpPerSocketContext->m_SendBufferList.Push(OP_NEW_1<CIOCPBuffer, CIOCPBuffer*>(_FILE_AND_LINE_, lpIOCPBuffer));
			bRet = TRUE;
		}
		else {
			lpPerSocketContext->m_SendContext.BufferReader = OP_NEW_1<CIOCPBufferReader, CIOCPBuffer*>(_FILE_AND_LINE_, lpIOCPBuffer);

			EnterIOLoop(lpPerSocketContext);

			PostClientIoWrite(lpPerSocketContext, &lpPerSocketContext->m_SendContext, PostResult);

			if (PostResult == PostIoSuccess) {
				bRet = TRUE;
			}
			else {
				CloseClient(lpPerSocketContext);
				ReleaseSocketContext(lpPerSocketContext);
			}
		}
	}

	lpPerSocketContext->m_Lock.UnLock();

	return bRet;
}
BOOL CBaseIOCPServer::Send(LONGLONG guid, CIOCPBuffer* lpIOCPBuffer)
{
	BOOL bRet = FALSE;
	PPER_SOCKET_CONTEXT lpPerSocketContext;

	m_ContextList.m_ContextLock.Lock();
	lpPerSocketContext = m_ContextList.GetContext(guid);
	if (lpPerSocketContext) {
		bRet = Send(lpPerSocketContext, lpIOCPBuffer);
	}

	m_ContextList.m_ContextLock.UnLock();

	return bRet;
}
VOID CBaseIOCPServer::NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext)
{

}
VOID CBaseIOCPServer::NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext)
{

}
VOID CBaseIOCPServer::NotifyWriteCompleted(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* lpIOCPBuffer)
{

}
VOID CBaseIOCPServer::NotifyWritePackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* lpIOCPBuffer)
{

}
VOID CBaseIOCPServer::NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer)
{

}