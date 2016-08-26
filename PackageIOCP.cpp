#include "IOCPCommon.h"
#include "IOCPBuffer.h"
#include "IOCPBufferWriter.h"
#include "IOCPBufferReader.h"
#include "BaseIOCPServer.h"
#include "PackageIOCP.h"


PackageUserContext* PackageIOCPManager::GetContext(LONGLONG nUserID)
{
	PackageUserContext* lpPackageContext = NULL;

	m_critical.Lock();
	lpPackageContext = m_ContextMap[nUserID];
	m_critical.UnLock();

	return lpPackageContext;
}

VOID PackageIOCPManager::AddContext(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	PackageUserContext* lpPackageContext = new PackageUserContext();
	lpPackageContext->lpPerSocketContext = lpPerSocketContext;
	lpPackageContext->m_nUserID = lpPerSocketContext->m_guid;
	m_critical.Lock();

	m_ContextMap[lpPackageContext->lpPerSocketContext->m_guid] = lpPackageContext;

	m_critical.UnLock();
}

VOID PackageIOCPManager::DelContext(LONGLONG nUserID)
{
	m_critical.Lock();

	auto iter = m_ContextMap.find(nUserID);

	if(iter != m_ContextMap.end())
	{
		delete iter->second;
		m_ContextMap.erase(iter);
	}

	m_critical.UnLock();
}
PackageIOCP::PackageIOCP(void)
{
}


PackageIOCP::~PackageIOCP(void)
{
}
VOID PackageIOCP::NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	m_manager.AddContext(lpPerSocketContext);
}

VOID PackageIOCP::NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	m_manager.DelContext(lpPerSocketContext->m_guid);
}

VOID PackageIOCP::NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext,CIOCPBuffer* inBuffer){
	PackageUserContext* lpPackageContext = m_manager.GetContext(lpPerSocketContext->m_guid);
	if(lpPackageContext){
		lpPackageContext->m_lpBufferWriter->Write(inBuffer);

		if(lpPackageContext->m_lpBufferWriter->GetBuffer()->GetLength() > IOCP_MAX_PACKETS_SIZE){
			CloseClient(lpPerSocketContext);
			return;
		}

		while(true){
			CIOCPBuffer* lpBuffer = lpPackageContext->m_lpBufferWriter->GetBuffer();
		
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
				CloseClient(lpPerSocketContext);
				return;
			}

			
			CIOCPBuffer NotifBuffer;
			NotifBuffer.Append(&lpBuffer->GetBytes()[sizeof(NETPackage)],(Package->length - sizeof(NETPackage)));

			NotifyReceivedFormatPackage(lpPerSocketContext,&NotifBuffer);

			memcpy(lpBuffer->GetBytes(),
				&lpBuffer->GetBytes()[Package->length],
				lpBuffer->GetLength() - Package->length);
			lpBuffer->m_pData->m_dwDataLength = lpBuffer->GetLength() - Package->length;
		}
	}
}

VOID PackageIOCP::SendEx(PPER_SOCKET_CONTEXT lpPerSocketContext,CIOCPBuffer* lpBuffer)
{
	NETPackage info;
	CIOCPBufferWriter Writer;
	info.title = NET_PACKAGE_TITLE;
	info.length = lpBuffer->GetLength() + sizeof(NETPackage);

	Writer.Write(info);
	Writer.Write(lpBuffer);

	Send(lpPerSocketContext,Writer.GetBuffer());
}

VOID PackageIOCP::SendEx(LONGLONG guid,CIOCPBuffer* lpBuffer)
{
	NETPackage info;
	CIOCPBufferWriter Writer;
	info.title = NET_PACKAGE_TITLE;
	info.length = lpBuffer->GetLength() + sizeof(NETPackage);

	Writer.Write(info);
	Writer.Write(lpBuffer);

	Send(guid,Writer.GetBuffer());
}

VOID PackageIOCP::NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer)
{
	//printf("PackageIOCP:%s\n",pBuffer->GetBytes());

	//SendEx(lpPerSocketContext,pBuffer);
}