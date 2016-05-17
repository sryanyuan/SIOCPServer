#ifndef _INC_SIOCPBUFFER_
#define _INC_SIOCPBUFFER_
//////////////////////////////////////////////////////////////////////////
#include <Windows.h>
//////////////////////////////////////////////////////////////////////////
class SIOCPBuffer
{
public:
	SIOCPBuffer()
	{
		m_uBufferLen = 0;
		m_pBuffer = NULL;
		m_uDataLength = 0;
		m_uReadOffset = 0;
	}
	~SIOCPBuffer()
	{
		FreeBuffer();
	}

public:
	void AllocBuffer(size_t _uSize)
	{
		FreeBuffer();
		m_pBuffer = (char*)malloc(_uSize);
		m_uBufferLen = _uSize;
	}
	void FreeBuffer()
	{
		if(NULL != m_pBuffer)
		{
			free(m_pBuffer);
			m_pBuffer = NULL;
			m_uBufferLen = 0;
			m_uReadOffset = 0;
		}
	}
	//	double the buffer size
	void ReallocBuffer()
	{
		m_uBufferLen *= 2;
		m_pBuffer = (char*)realloc(m_pBuffer, m_uBufferLen);

		if(NULL == m_pBuffer)
		{
			m_uBufferLen = 0;
			m_uDataLength = 0;
			m_uReadOffset = 0;
		}
	}

	size_t Write(const char* _pData, size_t _uLen)
	{
		if(m_uBufferLen - m_uDataLength < _uLen)
		{
			ReallocBuffer();
		}

		if(NULL == m_pBuffer)
		{
			return 0;
		}

		memcpy(m_pBuffer + m_uDataLength, _pData, _uLen);
		m_uDataLength += _uLen;

		return _uLen;
	}

	int Read(char* _pBuffer, size_t _uLen)
	{
		if(_uLen > m_uDataLength - m_uReadOffset)
		{
			_uLen = m_uDataLength - m_uReadOffset;
		}

		if(NULL != _pBuffer)
		{
			memcpy(_pBuffer, m_pBuffer + m_uReadOffset, _uLen);
		}

		//	offset
		m_uReadOffset += _uLen;

		return _uLen;
	}

	size_t GetBufferSize()
	{
		return m_uBufferLen;
	}

	size_t GetReadableSize()
	{
		return m_uDataLength - m_uReadOffset;
	}

	void Rewind()
	{
		m_uReadOffset = 0;
	}

	char* GetFreeBufferPtr()
	{
		return m_pBuffer + m_uDataLength;
	}

	char* GetReadableBufferPtr()
	{
		return m_pBuffer + m_uReadOffset;
	}

	char* GetDataPtr()
	{
		return m_pBuffer;
	}

	void Reset()
	{
		m_uDataLength = 0;
		m_uReadOffset = 0;
	}

	size_t GetAvailableSize()
	{
		return m_uBufferLen - m_uDataLength;
	}

	void SetDataLength(unsigned int _uLength)
	{
		m_uDataLength = _uLength;
	}

private:
	char* m_pBuffer;
	size_t m_uBufferLen;
	size_t m_uDataLength;
	size_t m_uReadOffset;
};
//////////////////////////////////////////////////////////////////////////
#endif