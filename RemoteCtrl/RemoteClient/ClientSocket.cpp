#include "pch.h"
#include "ClientSocket.h"



// ��̬�����Ķ���
CClientSocket* CClientSocket::m_instance = NULL;

// ������
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pServer = CClientSocket::getInstance();



std::string GetErrInfo(int wsaErrno) { //��ô�����Ϣ
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrno,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}