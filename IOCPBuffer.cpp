#include "IOCPCommon.h"
#include "IOCPBuffer.h"


IOCPBufferMngr IOCPBufferMngr::m_Instance;

IOCPBufferData::IOCPBufferData()
{
	m_nRefs = 0;
	
	m_dwAllocLength = IOCP_BUFFER_ALLOC_SIZE;
	m_dwDataLength = 0;

	m_pData = (BYTE *)malloc(m_dwAllocLength);
}

IOCPBufferData::~IOCPBufferData()
{
	free(m_pData);
}

VOID IOCPBufferData::AddRef()
{
	InterlockedIncrement(&m_nRefs);
}

VOID IOCPBufferData::Release()
{
	if(InterlockedDecrement(&m_nRefs) <= 0)
	{
		IOCPBufferMngr::getInstance()->Free(this);
	}
}

CIOCPBuffer::CIOCPBuffer(void)
{
	m_dwReaderPosition = 0;
	m_dwWriterPosition = 0;
	m_pData = IOCPBufferMngr::getInstance()->Allocate(IOCP_BUFFER_ALLOC_SIZE);	
}
CIOCPBuffer::CIOCPBuffer(CIOCPBuffer* rhs)
{
	m_dwReaderPosition = 0;
	m_dwWriterPosition = 0;
	rhs->m_pData->AddRef();
	m_pData = rhs->m_pData;
}
CIOCPBuffer::CIOCPBuffer(DWORD dwAllocLength)
{
	m_dwReaderPosition = 0;
	m_dwWriterPosition = 0;
	m_pData = IOCPBufferMngr::getInstance()->Allocate(dwAllocLength);
	m_pData->m_dwDataLength = dwAllocLength;
}
CIOCPBuffer::CIOCPBuffer(CIOCPBuffer& rhs)
{
	rhs.m_pData->AddRef();
	m_dwReaderPosition = 0;
	m_dwWriterPosition = 0;
	m_pData = rhs.m_pData;
}

CIOCPBuffer::~CIOCPBuffer(void)
{
	m_pData->Release();
}
void CIOCPBuffer::Reallocate(ULONG dwAllocLength)
{
	m_pData = IOCPBufferMngr::getInstance()->Reallocate(m_pData,dwAllocLength);
	m_pData->m_dwDataLength = dwAllocLength;
}
CIOCPBuffer& CIOCPBuffer::operator = (const CIOCPBuffer& rhs)
{
	rhs.m_pData->AddRef();

	m_pData->Release();

	m_pData = rhs.m_pData;
	return *this;
}
CIOCPBuffer& CIOCPBuffer::operator += (const CIOCPBuffer& rhs)
{
	DWORD dwSourceSize = m_pData->m_dwDataLength;

	Reallocate(m_pData->m_dwDataLength + rhs.m_pData->m_dwDataLength);
	
	memcpy(&m_pData->m_pData[dwSourceSize], rhs.m_pData->m_pData, rhs.m_pData->m_dwDataLength);

	return *this;
}

CIOCPBuffer CIOCPBuffer::operator + (const CIOCPBuffer& rhs)
{
	CIOCPBuffer tmpBuffer;
	tmpBuffer += *this;
	tmpBuffer += rhs;
	return tmpBuffer;
}

void CIOCPBuffer::IgnoreBytes( ULONG dwSize ) throw(...)
{
	if(m_dwReaderPosition + dwSize > m_pData->m_dwDataLength)
		throw std::exception("Read data length exceeds the buffer length.");

	m_dwReaderPosition += dwSize;
}
void CIOCPBuffer::AddBytesAndReallocate(ULONG dwDataLength)
{
	m_pData = IOCPBufferMngr::getInstance()->Reallocate(m_pData,dwDataLength + m_pData->m_dwDataLength);

	m_pData->m_dwDataLength += dwDataLength;
}
void CIOCPBuffer::Read(unsigned char* pbData,ULONG dwSize) throw(...)
{
	if(m_dwReaderPosition + dwSize > m_pData->m_dwDataLength)
		throw std::exception("Read data length exceeds the buffer length.");

	memcpy(pbData,&m_pData->m_pData[m_dwReaderPosition],dwSize);

	m_dwReaderPosition += dwSize;
}
void CIOCPBuffer::ReadString(char* outVar,unsigned short max_length) throw(...)
{
	unsigned short string_length;

	strcpy(outVar,"");

	Read(string_length);

	if(((unsigned int)string_length + 1) < max_length) //detected overflow of integer
	{
		Read((unsigned char*)outVar,string_length);
		outVar[string_length] = 0;
	}
	else
	{
		throw std::exception("Read data length exceeds the buffer length.");
	}
}
void CIOCPBuffer::BeginRead()
{
	try
	{
		m_dwReaderPosition = 0;
		IgnoreBytes(sizeof(DWORD));
	}catch(...)
	{
		
	};
}
DWORD CIOCPBuffer::GetBufferLength()
{
	return m_pData->m_dwDataLength;
}
BYTE* CIOCPBuffer::GetBuffer()
{
	return m_pData->m_pData;
}
void CIOCPBuffer::Reset()
{
	m_dwReaderPosition = 0;
	m_dwWriterPosition = 0;
}
void CIOCPBuffer::BeginWrite()
{
	AddBytesAndReallocate(sizeof(DWORD));
	m_dwWriterPosition += sizeof(DWORD);
}
void CIOCPBuffer::EndWrite()
{
	*(DWORD*)m_pData->m_pData = (m_dwWriterPosition - sizeof(DWORD));
}
void CIOCPBuffer::Write(const unsigned char* pbData,ULONG dwSize)
{
	AddBytesAndReallocate(dwSize);
	memcpy(&m_pData->m_pData[m_dwWriterPosition],pbData,dwSize);

	m_dwWriterPosition += dwSize;
}

void CIOCPBuffer::WriteString(const char* inVar)
{
	unsigned short string_length = (unsigned short)strlen(inVar);

	Write(string_length);
	Write((const unsigned char*)inVar,string_length);
}
IOCPBufferMngr::IOCPBufferMngr()
{

}
IOCPBufferMngr::~IOCPBufferMngr()
{

}

IOCPBufferMngr* IOCPBufferMngr::getInstance()
{
	return &m_Instance;
}

IOCPBufferData* IOCPBufferMngr::Allocate(DWORD dwAllocLength)
{
	getInstance()->m_MutexLock.Lock();
	IOCPBufferData* pData = getInstance()->m_BufferMemPool.Alloc();
	getInstance()->m_MutexLock.UnLock();

	if(pData)
	{
		pData->AddRef();

		pData->m_dwDataLength = 0;

		if(pData->m_dwAllocLength < dwAllocLength)
		{
			pData->m_dwAllocLength = IOCP_BUFFER_ALIGN_MEMORY(dwAllocLength,IOCP_BUFFER_ALLOC_SIZE);
			free(pData->m_pData);
			pData->m_pData = (BYTE *)malloc(pData->m_dwAllocLength);
		}
	}

	return pData;
}
VOID IOCPBufferMngr::Free(IOCPBufferData* pData)
{
	getInstance()->m_MutexLock.Lock();
	getInstance()->m_BufferMemPool.Free(pData);
	getInstance()->m_MutexLock.UnLock();
}

IOCPBufferData* IOCPBufferMngr::Reallocate(IOCPBufferData* pData,DWORD dwAllocLength)
{
	if(pData->m_dwAllocLength < dwAllocLength)
	{
		pData->m_dwAllocLength = IOCP_BUFFER_ALIGN_MEMORY(dwAllocLength,IOCP_BUFFER_ALLOC_SIZE);
		pData->m_pData = (BYTE *)realloc(pData->m_pData,pData->m_dwAllocLength);
	}

	return pData;
}