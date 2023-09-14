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

bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET)
		CloseSocket();

	TRACE("addr:%08x, nIP:%08X, nPort:%d\r\n", inet_addr("127.0.0.1"), m_nIP, m_nPort);
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) return false;

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);

	if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox(_T("ָ����IP������!"));
		return false;
	}

	// connect
	if (connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		// Question
		AfxMessageBox(_T("����ʧ��"));
		TRACE("����ʧ��: %d %s.\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, BOOL isAutoClosed)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("start client socket thread.\r\n");
	}
	TRACE("cmd:%d event:%08X threadID:%d\r\n", pack.sCmd, pack.m_hEvent, GetCurrentThreadId());
	//�̰߳�ȫ
	m_lock.lock();
	m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.m_hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.m_hEvent, isAutoClosed));
	m_lstSend.push_back(pack);
	m_lock.unlock();
	TRACE("scmd:%d\r\n", pack.sCmd);
	WaitForSingleObject(pack.m_hEvent, INFINITE);
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.m_hEvent);
	if (it != m_mapAck.end()) {
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
	}
	return false;
}

void CClientSocket::threadFunc_msg()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}

	}
}

void CClientSocket::SendPacket_msg(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//TODO: ��Ϣ���ݽṹ: ���� ���� ģʽ  �ص���Ϣ�����ݽṹ:HWND MESSAGE
	if (InitSocket()) {
		int ret = send(m_sock, (char*)wParam, (int)lParam, 0);
		if (ret > 0) {

		}
		else
		{

		}
	}
	else {

	}
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
	_endthread();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) { //TODO: ���ش��ļ���������
		if (m_lstSend.size() > 0) {
			CPacket head = m_lstSend.front();
			TRACE("head.shead %x\r\n", head.sHead);
			if (Send(head) == false) {
				TRACE(_T("����ʧ��\r\n"));
				continue;
			}
			//Ӧ�����
			std::map<HANDLE, std::list<CPacket>&>::iterator it_ack 
				= m_mapAck.find(head.m_hEvent);
			//�Զ��ر�
			std::map<HANDLE, bool>::iterator it_auto =
				m_mapAutoClosed.find(head.m_hEvent);
			if (it_ack != m_mapAck.end()) {
				do {
					int len = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("read len:%d, index:%d\r\n", len, index);
					if ((len > 0) || (index > 0)) {
						index += len;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);

						if (size > 0) {
							TRACE("size:%d, len:%d, index:%d\r\n", size, len, index);
							//֪ͨ��Ӧ�¼�
							pack.m_hEvent = head.m_hEvent;
							TRACE("recv.shead:%x, cmd:%d\r\n", pack.sHead, pack.sCmd);
							it_ack->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it_auto->second) {//����
								CloseSocket();
								SetEvent(head.m_hEvent);
								break;
							}
						}
					}
					else if (len <= 0 && index <= 0) {
						TRACE("&& len:%d, index:%d\r\n", len, index);
						CloseSocket();
						SetEvent(head.m_hEvent); //�ȴ�����˹رգ���֪ͨ���
						
						break;
					}
				} while (it_auto->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.m_hEvent);
			m_lock.unlock();
			InitSocket();
		}
		Sleep(1);
	}
	CloseSocket();
}
