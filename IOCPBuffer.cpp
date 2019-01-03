#include "IOCPCommon.h"
#include "IOCPBuffer.h"
#include "IOCPBufferWriter.h"
#include "IOCPBufferReader.h"


IOCPBufferMngr* IOCPBufferMngr::m_Instance = NULL;

IOCPBufferData::IOCPBufferData( DWORD dwAllocLength )
{
	m_nRefs = 0;
	m_dwAllocLength = IOCP_BUFFER_ALIGN_MEMORY( dwAllocLength, IOCP_BUFFER_ALLOC_SIZE );
	m_dwDataLength = 0;
	m_pData = (BYTE *)malloc( m_dwAllocLength );
}

IOCPBufferData::IOCPBufferData( )
{
	m_nRefs = 0;
	m_dwAllocLength = IOCP_BUFFER_ALLOC_SIZE;
	m_dwDataLength = 0;
	m_pData = (BYTE *)malloc( m_dwAllocLength );
}

IOCPBufferData::~IOCPBufferData( )
{
	if ( m_pData )
	{
		free( m_pData );
	}
}

VOID IOCPBufferData::AddRef( )
{
	InterlockedIncrement( &m_nRefs );
}

VOID IOCPBufferData::Release( )
{
	if ( InterlockedDecrement( &m_nRefs ) <= 0 )
	{
		IOCPBufferMngr::getInstance( )->Free( this );
	}
}

CIOCPBuffer::CIOCPBuffer( void )
{
	m_pData = IOCPBufferMngr::getInstance( )->Allocate( IOCP_BUFFER_ALLOC_SIZE );
	m_pData->AddRef( );
}
CIOCPBuffer::CIOCPBuffer( CIOCPBuffer* rhs )
{
	rhs->m_pData->AddRef( );
	m_pData = rhs->m_pData;
}
CIOCPBuffer::CIOCPBuffer( DWORD dwAllocLength )
{
	m_pData = IOCPBufferMngr::getInstance( )->Allocate( dwAllocLength );
	m_pData->m_dwDataLength = dwAllocLength;
	m_pData->AddRef( );
}
CIOCPBuffer::CIOCPBuffer( CIOCPBuffer& rhs )
{
	rhs.m_pData->AddRef( );
	m_pData = rhs.m_pData;
}

CIOCPBuffer::~CIOCPBuffer( void )
{
	m_pData->Release( );
}
void CIOCPBuffer::Reallocate( ULONG dwAllocLength )
{
	m_pData = IOCPBufferMngr::getInstance( )->Reallocate( m_pData, dwAllocLength );
	m_pData->m_dwDataLength = dwAllocLength;
}
CIOCPBuffer& CIOCPBuffer::operator = ( const CIOCPBuffer& rhs )
{
	rhs.m_pData->AddRef( );

	m_pData->Release( );

	m_pData = rhs.m_pData;
	return *this;
}
CIOCPBuffer& CIOCPBuffer::operator += ( const CIOCPBuffer& rhs )
{
	Append( rhs.m_pData->m_pData, rhs.m_pData->m_dwDataLength );

	return *this;
}

CIOCPBuffer CIOCPBuffer::operator + ( const CIOCPBuffer& rhs )
{
	CIOCPBuffer tmpBuffer;
	tmpBuffer += *this;
	tmpBuffer += rhs;
	return tmpBuffer;
}
void CIOCPBuffer::AddBytesAndReallocate( ULONG dwDataLength )
{
	m_pData = IOCPBufferMngr::getInstance( )->Reallocate( m_pData, dwDataLength + m_pData->m_dwDataLength );

	m_pData->m_dwDataLength += dwDataLength;
}

void CIOCPBuffer::Append( CONST BYTE* lpData, ULONG dwLength )
{
	ULONG pos = m_pData->m_dwDataLength;

	AddBytesAndReallocate( dwLength );

	memcpy( &m_pData->m_pData[pos], lpData, dwLength );
}

BYTE* CIOCPBuffer::GetBytes( )
{
	return m_pData->m_pData;
}

ULONG CIOCPBuffer::GetLength( )
{
	return m_pData->m_dwDataLength;
}

IOCPBufferMngr::IOCPBufferMngr( )
{

}

IOCPBufferMngr::~IOCPBufferMngr( )
{

}

IOCPBufferMngr* IOCPBufferMngr::getInstance( )
{
	if ( m_Instance == NULL )
	{
		m_Instance = new IOCPBufferMngr( );
	}

	return m_Instance;
}

#if 0


IOCPBufferData* IOCPBufferMngr::Allocate( DWORD dwAllocLength )
{
	IOCPBufferData *lpBufferData = new IOCPBufferData( dwAllocLength );
	return lpBufferData;
}

VOID IOCPBufferMngr::Free( IOCPBufferData* pData )
{
	delete pData;
}


#endif

IOCPBufferData* IOCPBufferMngr::Allocate( DWORD dwAllocLength )
{
	getInstance( )->m_MutexLock.Lock( );
	IOCPBufferData* pData = getInstance( )->m_BufferMemPool.Alloc( );
	getInstance( )->m_MutexLock.UnLock( );

	if ( pData )
	{
		pData->m_dwDataLength = 0;

		if ( pData->m_dwAllocLength < dwAllocLength )
		{
			pData->m_dwAllocLength = IOCP_BUFFER_ALIGN_MEMORY( dwAllocLength, IOCP_BUFFER_ALLOC_SIZE );
			free( pData->m_pData );
			pData->m_pData = (BYTE *)malloc( pData->m_dwAllocLength );
		}
	}

	return pData;
}
VOID IOCPBufferMngr::Free( IOCPBufferData* pData )
{
	getInstance( )->m_MutexLock.Lock( );
	getInstance( )->m_BufferMemPool.Free( pData );
	getInstance( )->m_MutexLock.UnLock( );
}

IOCPBufferData* IOCPBufferMngr::Reallocate( IOCPBufferData* pData, DWORD dwAllocLength )
{
	if ( pData->m_dwAllocLength < dwAllocLength )
	{
		pData->m_dwAllocLength = IOCP_BUFFER_ALIGN_MEMORY( dwAllocLength, IOCP_BUFFER_ALLOC_SIZE );
		pData->m_pData = (BYTE *)realloc( pData->m_pData, pData->m_dwAllocLength );
	}

	return pData;
}

void IOCPBufferMngr::ReleasePool( )
{
	getInstance( )->m_MutexLock.Lock( );
	getInstance( )->m_BufferMemPool.DeleteAll( );
	getInstance( )->m_MutexLock.UnLock( );
}