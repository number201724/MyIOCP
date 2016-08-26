#pragma once

class UserBufferObject
{
public:
	LONGLONG m_ulGuid;
	CIOCPBufferWriter m_stBufferWriter;
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


	VOID SendEx(PPER_SOCKET_CONTEXT lpPerSocketContext,LPCSTR lpszText);
public:
	static DWORD GetStringLen(CONST BYTE* lpBuffer,DWORD dwBufferSize);
private:
	//需要调用父类
	virtual VOID NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer);


	virtual VOID NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText);
};