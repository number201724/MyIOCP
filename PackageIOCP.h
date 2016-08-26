#pragma once

#pragma pack(1)

#define NET_PACKAGE_TITLE 0x55E0

struct NETPackage
{
	unsigned int length;
	unsigned short title;
};
#pragma pack()
class PackageUserContext
{
public:
	PackageUserContext(){
		m_lpBufferWriter = OP_NEW<CIOCPBufferWriter>(_FILE_AND_LINE_);
		lpPerSocketContext = NULL;
		m_nUserID = NULL;
	}
	virtual ~PackageUserContext(){
		OP_DELETE<CIOCPBufferWriter>(m_lpBufferWriter,_FILE_AND_LINE_);
		m_lpBufferWriter = NULL;
	}

	PPER_SOCKET_CONTEXT lpPerSocketContext;
	LONGLONG m_nUserID;
	CIOCPBufferWriter* m_lpBufferWriter;
};

typedef std::map<LONGLONG,PackageUserContext*> PackageContextMap_t;
class PackageIOCPManager
{
private:
	IOCPMutex m_critical;
	PackageContextMap_t m_ContextMap;
public:
	PackageUserContext* GetContext(LONGLONG nUserID);
	VOID AddContext(PPER_SOCKET_CONTEXT lpPerSocketContext);
	VOID DelContext(LONGLONG nUserID);
};
class PackageIOCP:
	public CBaseIOCPServer
{
public:
	PackageIOCP(void);
	~PackageIOCP(void);

	PackageIOCPManager m_manager;


	VOID SendEx(PPER_SOCKET_CONTEXT lpPerSocketContext,CIOCPBuffer* lpBuffer);
	VOID SendEx(LONGLONG guid,CIOCPBuffer* lpBuffer);

	virtual VOID NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer);

public:
	virtual VOID NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer);
};

