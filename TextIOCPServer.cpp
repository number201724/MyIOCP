#include "IOCPCommon.h"
#include "IOCPBuffer.h"
#include "IOCPBufferWriter.h"
#include "IOCPBufferReader.h"
#include "BaseIOCPServer.h"
#include "TextIOCPServer.h"

UserBufferObject::UserBufferObject()
{
	m_nRefs = 0;
}
VOID UserBufferList::AllocateBufferObject(LONGLONG guid)
{
	UserBufferObject* object = new UserBufferObject();

	object->m_ulGuid = guid;
	object->m_nRefs++;

	m_stBufferLock.Lock();

	m_vBufferMap[guid] = object;

	m_stBufferLock.UnLock();
}

VOID UserBufferList::ReleaseBufferObject(LONGLONG guid)
{
	UserBufferObject* object = NULL;
	std::map <LONGLONG,UserBufferObject*>::iterator iter;

	m_stBufferLock.Lock();
	iter = m_vBufferMap.find(guid);
	if(iter != m_vBufferMap.end()){
		object = iter->second;
	}
	m_stBufferLock.UnLock();

	if(object){
		ReleaseBufferObject(object);
	}
}
VOID UserBufferList::ReleaseBufferObject(UserBufferObject* object)
{
	std::map <LONGLONG,UserBufferObject*>::iterator iter;

	if(InterlockedDecrement(&object->m_nRefs) <= 0){

		m_stBufferLock.Lock();

		iter = m_vBufferMap.find(object->m_ulGuid);
		if(iter != m_vBufferMap.end()){
			m_vBufferMap.erase(iter);
		}

		m_stBufferLock.UnLock();

		delete object;
	}
}
UserBufferObject* UserBufferList::GetBufferObject(LONGLONG guid)
{
	UserBufferObject* object = NULL;

	m_stBufferLock.Lock();

	object = m_vBufferMap[guid];

	if(object){
		InterlockedIncrement(&object->m_nRefs);
	}
	m_stBufferLock.UnLock();

	return object;
}
CTextIOCPServer::CTextIOCPServer(void)
{
}


CTextIOCPServer::~CTextIOCPServer(void)
{
}

DWORD CTextIOCPServer::GetStringLen(CONST BYTE* lpBuffer,DWORD dwBufferSize)
{
	for(DWORD i=0;i<dwBufferSize;i++)
	{
		if(lpBuffer[i] == 0){
			return (i + 1);
		}
	}
	return -1;
}

VOID CTextIOCPServer::NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	m_stUserBufferMap.AllocateBufferObject(lpPerSocketContext->m_guid);
}
VOID CTextIOCPServer::NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	m_stUserBufferMap.ReleaseBufferObject(lpPerSocketContext->m_guid);
}

VOID CTextIOCPServer::NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer)
{
	UserBufferObject* object = m_stUserBufferMap.GetBufferObject(lpPerSocketContext->m_guid);
	if(object){
		DWORD dwNewBufferLength = pBuffer->GetLength() + object->m_stBufferWriter.GetBuffer()->GetLength();

		if(dwNewBufferLength > 0x20000){		//大于128KB的文本就踢出用户
			CloseClient(lpPerSocketContext);
			m_stUserBufferMap.ReleaseBufferObject(object);
			return;
		}
		object->m_stBufferWriter.Write(pBuffer);

		DWORD dwBufferLength = object->m_stBufferWriter.GetBuffer()->GetLength();
		BYTE* lpBuffer = object->m_stBufferWriter.GetBuffer()->GetBytes();

		BYTE* lpStartPos = lpBuffer;
		DWORD dwReadingLength = 0;
		DWORD dwLastLength = dwBufferLength;

		DWORD dwStringLength = GetStringLen(lpStartPos,dwLastLength);

		if(dwStringLength != -1){
			do{
				char* lpszText = new char[dwStringLength];
				strncpy_s(lpszText,dwStringLength,(const char*)lpStartPos,dwStringLength);

				NotifyReceivedFormatPackage(lpPerSocketContext,lpszText);

				delete [] lpszText;

				dwReadingLength += dwStringLength;
				lpStartPos += dwStringLength;
				dwLastLength -= dwStringLength;

				dwStringLength = GetStringLen(lpStartPos,dwLastLength);
			}while(dwStringLength != -1);
		}

		if(dwReadingLength > 0){

			memcpy((BYTE*)object->m_stBufferWriter.GetBuffer()->GetBytes(),
				&object->m_stBufferWriter.GetBuffer()->GetBytes()[dwReadingLength],dwLastLength);
			object->m_stBufferWriter.GetBuffer()->m_pData->m_dwDataLength = dwLastLength;
		}

		m_stUserBufferMap.ReleaseBufferObject(object);
	}
}
VOID CTextIOCPServer::SendEx(PPER_SOCKET_CONTEXT lpPerSocketContext,LPCSTR lpszText)
{
	CIOCPBufferWriter Writer;
	Writer.Write((BYTE*)lpszText,strlen(lpszText) + sizeof(char));

	Send(lpPerSocketContext,Writer.GetBuffer());
}
VOID CTextIOCPServer::NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText)
{

}
