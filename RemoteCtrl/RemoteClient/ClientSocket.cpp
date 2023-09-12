#include "pch.h"
#include "ClientSocket.h"



// 静态变量的定义
CClientSocket* CClientSocket::m_instance = NULL;

// 辅助类
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pServer = CClientSocket::getInstance();



std::string GetErrInfo(int wsaErrno) { //获得错误信息
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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	if (InitSocket() == FALSE)
		return;
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	while (m_sock != INVALID_SOCKET) { //TODO: 下载大文件存在问题
		CPacket& head = m_lstSend.front();
		if (Send(head) == FALSE)
			continue;
		auto pr = m_mapAck.insert(
			std::pair<HANDLE, std::list<CPacket>>(head.m_hEvent, std::list<CPacket>()));
		int len = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
		if (len > 0 || index > 0) {
			index += len;
			size_t size = (size_t)index;
			CPacket pack((BYTE*)pBuffer, size);

			if (size > 0) {
				//通知对应事件
				pack.m_hEvent = head.m_hEvent;
				pr.first->second.push_back(pack);
				SetEvent(head.m_hEvent);
			}
			continue;
		}
		else if (len <= 0 && index <= 0) {
			CloseSocket();
		}
		m_lstSend.pop_front();
	}
}
