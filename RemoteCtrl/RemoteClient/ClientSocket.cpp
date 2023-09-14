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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, BOOL isAutoClosed)
{
	if (m_sock == INVALID_SOCKET) {
		/*if (InitSocket() == FALSE)
			return FALSE;*/
		_beginthread(&CClientSocket::threadEntry, 0, this);
	}

	m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(pack.m_hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.m_hEvent, isAutoClosed));
	//问题 不是原子操作
	m_lstSend.push_back(pack);
	TRACE("scmd:%d\r\n", pack.sCmd);
	WaitForSingleObject(pack.m_hEvent, INFINITE);
	std::map<HANDLE, std::list<CPacket>>::iterator it;
	it = m_mapAck.find(pack.m_hEvent);
	if (it != m_mapAck.end()) {
		std::list<CPacket>::iterator i;
		for (i = it->second.begin(); i != it->second.end(); i++) {
			lstPacks.push_back(*i);
		}
		m_mapAck.erase(it);
		return TRUE;
	}
	return FALSE;
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock %d\r\n", m_sock);
	//Dump((BYTE*)pack.Data(), pack.Size());
	if (m_sock == -1)
		return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) { //TODO: 下载大文件存在问题
		if (m_lstSend.size() > 0) {
			CPacket head = m_lstSend.front();
			TRACE("head.shead %x\r\n", head.sHead);
			if (Send(head) == false) {
				TRACE(_T("发送失败\r\n"));
				continue;
			}

			//应答队列
			std::map<HANDLE, std::list<CPacket>>::iterator it_ack 
				= m_mapAck.find(head.m_hEvent);
			//自动关闭
			std::map<HANDLE, bool>::iterator it_auto = 
				m_mapAutoClosed.find(head.m_hEvent);

			do {
				int len = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				TRACE("|| len:%d, index:%d\r\n", len, index);
				if (len > 0 || index > 0) {
					index += len;
					size_t size = (size_t)index;
					CPacket pack((BYTE*)pBuffer, size);

					if (size > 0) {
						TRACE("size:%d, len:%d, index:%d\r\n", size, len, index);
						//通知对应事件
						pack.m_hEvent = head.m_hEvent;
						TRACE("recv.shead:%x, cmd:%d\r\n", pack.sHead, pack.sCmd);
						it_ack->second.push_back(pack);
						memmove(pBuffer, pBuffer + size, index - size);
						index -= size;
						if (it_auto->second) {
							SetEvent(head.m_hEvent);
						}
					}
				}
				else if (len <= 0 && index <= 0) {
					TRACE("&& len:%d, index:%d\r\n", len, index);
					CloseSocket();
					SetEvent(head.m_hEvent); //等待服务端关闭，再通知完成
				}
			} while (it_auto->second == false);
			m_lstSend.pop_front();
			InitSocket();
		}
		Sleep(1);
	}
	CloseSocket();
}
