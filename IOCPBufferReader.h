#ifndef _IOCPBUFFER_READER_H_
#define _IOCPBUFFER_READER_H_

class CIOCPBufferReader
{
private:
	CIOCPBuffer* m_lpBuffer;
public:
	DWORD m_nPosition;
public:
	CIOCPBufferReader(CIOCPBuffer* lpBuffer){
		m_nPosition = 0;
		this->m_lpBuffer = OP_NEW_1<CIOCPBuffer,CIOCPBuffer*>(_FILE_AND_LINE_,lpBuffer);
	}
	CIOCPBufferReader(CIOCPBuffer& Buffer){
		m_nPosition = 0;

		this->m_lpBuffer = OP_NEW_1<CIOCPBuffer,CIOCPBuffer&>(_FILE_AND_LINE_,Buffer);
	}

	virtual ~CIOCPBufferReader(){
		OP_DELETE<CIOCPBuffer>(this->m_lpBuffer,_FILE_AND_LINE_);
	}

	CIOCPBuffer* GetBuffer(){
		return this->m_lpBuffer;
	}

	BYTE* GetBytes(){
		return this->m_lpBuffer->GetBytes();
	}
	DWORD GetLength(){
		return this->m_lpBuffer->m_pData->m_dwDataLength;
	}



	template <class templateType>
	void __inline Read(templateType &outTemplateVar) throw (...);

	template <class templateType>
	void __inline Peek(templateType &outTemplateVar) throw (...);


	void Read(unsigned char* pbData,ULONG dwSize) throw(...)
	{
		if(m_nPosition + dwSize > m_lpBuffer->GetLength())
			throw std::exception("Read data length exceeds the buffer length.");

		memcpy(pbData,&m_lpBuffer->GetBytes()[m_nPosition],dwSize);

		m_nPosition += dwSize;
	}

	void ReadString(char* outVar,unsigned short max_length) throw (...)
	{
		unsigned short nStringLength;

		strcpy_s(outVar,max_length,"");

		Read(nStringLength);

		if(((unsigned int)nStringLength + 1) < max_length) //detected overflow of integer
		{
			Read((unsigned char*)outVar,nStringLength);
			outVar[nStringLength] = 0;
		}
		else
		{
			throw std::exception("Read data length exceeds the buffer length.");
		}
	}

	void IgnoreBytes( ULONG dwSize ) throw (...)
	{
		if(m_nPosition + dwSize > m_lpBuffer->GetLength())
			throw std::exception("Read data length exceeds the buffer length.");

		m_nPosition += dwSize;
	}
};

template <class templateType>
void __inline CIOCPBufferReader::Read(templateType &outTemplateVar) throw (...)
{
	if(m_nPosition + sizeof(templateType) > m_lpBuffer->GetLength())
		throw std::exception("Read data length exceeds the buffer length.");

	memcpy(&outTemplateVar,&m_lpBuffer->GetBytes()[m_nPosition],sizeof(templateType));

	m_nPosition += sizeof(templateType);
}

template <class templateType>
void __inline CIOCPBufferReader::Peek(templateType &outTemplateVar) throw (...)
{
	if(m_nPosition + sizeof(templateType) > m_lpBuffer->GetLength())
		throw std::exception("Read data length exceeds the buffer length.");

	memcpy(&outTemplateVar,&m_lpBuffer->GetBytes()[m_nPosition],sizeof(templateType));
}
#endif