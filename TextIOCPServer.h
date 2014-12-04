#pragma once

class UserBufferObject
{
public:
	LONGLONG m_ulGuid;
	CIOCPBuffer m_stBuffer;
	LONG m_nRefs;
	UserBufferObject();
};
class UserBufferList
{
private:
	std::map <LONGLONG,UserBufferObject*> m_vBufferMap;
	IOCPMutex m_stBufferLock;

public:
	UserBufferObject* GetBufferObject(LONGLONG guid);
	VOID AllocateBufferObject(LONGLONG guid);
	VOID ReleaseBufferObject(LONGLONG guid);
	VOID ReleaseBufferObject(UserBufferObject* object);
};
class CTextIOCPServer : public CBaseIOCPServer
{
public:
	CTextIOCPServer(void);
	virtual ~CTextIOCPServer(void);
	UserBufferList m_stUserBufferMap;
private:
	DWORD GetStringLen(BYTE* lpBuffer,DWORD dwBufferSize);
private:
	virtual VOID NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer);
	virtual VOID NotifyReceivedStrings(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText);
};