#ifndef _IOCPBUFFER_WRITER_H_
#define _IOCPBUFFER_WRITER_H_

class CIOCPBufferWriter
{
private:
	CIOCPBuffer* m_lpBuffer;
public:
	CIOCPBufferWriter(){
		m_lpBuffer = OP_NEW<CIOCPBuffer>(_FILE_AND_LINE_);
	}

	virtual ~CIOCPBufferWriter(){
		delete this->m_lpBuffer;
	}

	CIOCPBuffer* GetBuffer(){
		return m_lpBuffer;
	}

	template <class templateType>
		void __inline Write(const templateType &inTemplateVar);

	template <class templateType>
		void __inline WritePtr(templateType *inTemplateVar);

	void Write(CIOCPBuffer* lpBuffer){
		*this->m_lpBuffer += *lpBuffer;
	}
	void Write(CIOCPBuffer Buffer){
		*this->m_lpBuffer += Buffer;
	}

	void Write(const unsigned char* pbData,ULONG dwSize){
		m_lpBuffer->Append(pbData,dwSize);
	}

	void WriteString(const char* inVar){
		unsigned short uLength = (unsigned short)strlen(inVar);

		Write(uLength);
		Write((const unsigned char*)inVar,uLength);
	}
};


template <class templateType>
void __inline CIOCPBufferWriter::Write(const templateType &inTemplateVar){
	m_lpBuffer->Append((CONST BYTE*)&inTemplateVar,sizeof(templateType));
}
template <class templateType>
void __inline CIOCPBufferWriter::WritePtr(templateType *inTemplateVar){
	m_lpBuffer->Append((CONST BYTE*)inTemplateVar,sizeof(templateType));
}
#endif