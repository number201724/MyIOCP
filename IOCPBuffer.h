#pragma once


class IOCPBufferData
{
public:
	ULONG m_nRefs;
	BYTE* m_pData;
	DWORD m_dwAllocLength;
	DWORD m_dwDataLength;

	IOCPBufferData();
	virtual ~IOCPBufferData();

	VOID AddRef();
	VOID Release();
};

class CIOCPBuffer
{
public:
	IOCPBufferData* m_pData;
	DWORD m_dwReaderPosition;
	DWORD m_dwWriterPosition;
public:
	CIOCPBuffer(void);
	CIOCPBuffer(DWORD dwAllocLength);
	CIOCPBuffer(CIOCPBuffer& rhs);
	CIOCPBuffer(CIOCPBuffer* rhs);
	~CIOCPBuffer(void);
	
	CIOCPBuffer& operator = (const CIOCPBuffer& rhs);
	CIOCPBuffer& operator += (const CIOCPBuffer& rhs);
	CIOCPBuffer operator + (const CIOCPBuffer& rhs);

	void AddBytesAndReallocate(ULONG dwDataLength);
	void Reallocate(ULONG dwAllocLength);

	//读取特定数据类型的数据,超过数据长度则会抛出异常std::exception异常
	template <class templateType>
	void __inline Read(templateType &outTemplateVar) throw (...);

	template <class templateType>
	void __inline Peek(templateType &outTemplateVar) throw (...);

	void Read(unsigned char* pbData,ULONG dwSize) throw(...);

	//超过数据长度则会抛出异常std::exception异常
	void ReadString(char* outVar,unsigned short max_length) throw (...);

	//写入特定数据类型的数据
	template <class templateType>
	void __inline Write(const templateType &inTemplateVar);

	template <class templateType>
	void __inline WritePtr(templateType *inTemplateVar);
	void WriteString(const char* inVar);
	void Write(const unsigned char* pbData,ULONG dwSize);

	void BeginRead();
	void Reset();
	void BeginWrite();
	void EndWrite();

	DWORD GetBufferLength();
	BYTE* GetBuffer();

	//跳过读取dwSize大小的数据,超过数据长度则会抛出异常std::exception异常
	void IgnoreBytes( ULONG dwSize ) throw (...);
};

class IOCPBufferMngr
{
private:
	IOCPMutex m_MutexLock;
	IOCPMemPool <IOCPBufferData> m_BufferMemPool;
	static IOCPBufferMngr m_Instance;
private:
	IOCPBufferMngr();
	virtual ~IOCPBufferMngr();
public:
	static IOCPBufferMngr* getInstance();

	static IOCPBufferData* Allocate(DWORD dwAllocLength);
	static VOID Free(IOCPBufferData* pData);
	static IOCPBufferData* Reallocate(IOCPBufferData* pData,DWORD dwAllocLength);
};




template <class templateType>
void __inline CIOCPBuffer::Write(const templateType &inTemplateVar)
{
	AddBytesAndReallocate(sizeof(templateType));

	memcpy(&m_pData->m_pData[m_dwWriterPosition],&inTemplateVar,sizeof(templateType));

	m_dwWriterPosition += sizeof(templateType);
}
template <class templateType>
void __inline CIOCPBuffer::WritePtr(templateType *inTemplateVar)
{
	AddBytesAndReallocate(sizeof(templateType));

	memcpy(&m_pData->m_pData[m_dwWriterPosition],inTemplateVar,sizeof(templateType));

	m_dwWriterPosition += sizeof(templateType);
}
template <class templateType>
void __inline CIOCPBuffer::Read(templateType &outTemplateVar) throw (...)
{
	if(m_dwReaderPosition + sizeof(templateType) > m_pData->m_dwDataLength)
		throw std::exception("Read data length exceeds the buffer length.");

	memcpy(&outTemplateVar,&m_pData->m_pData[m_dwReaderPosition],sizeof(templateType));

	m_dwReaderPosition += sizeof(templateType);
}

template <class templateType>
void __inline CIOCPBuffer::Peek(templateType &outTemplateVar) throw (...)
{
	if(m_dwReaderPosition + sizeof(templateType) > m_pData->m_dwDataLength)
		throw std::exception("Read data length exceeds the buffer length.");

	memcpy(&outTemplateVar,&m_pData->m_pData[m_dwReaderPosition],sizeof(templateType));
}
