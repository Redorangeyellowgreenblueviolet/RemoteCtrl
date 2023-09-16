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



CClientSocket::CClientSocket(const CClientSocket& s)
{
	m_bAutoClosed = s.m_bAutoClosed;
	m_sock = s.m_sock;
	m_nIP = s.m_nIP;
	m_nPort = s.m_nPort;
	m_hThread = INVALID_HANDLE_VALUE;
	for (auto it = s.m_mapFunc.begin(); it != s.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}

}

CClientSocket::CClientSocket()
	: m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClosed(TRUE), m_hThread(INVALID_HANDLE_VALUE)
{
	if (!InitSockEnv())
	{
		//Question
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���."), _T("��ʼ������."), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_lstSend.clear();
	m_mapAck.clear();
	m_mapAutoClosed.clear();
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	struct
	{
		UINT message;
		MSGFUNC func;
	}funcs[]{
		{WM_SEND_PACK,&CClientSocket::SendPacket_msg},
		{(UINT)0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(
			funcs[i].message, funcs[i].func));
	}

	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadId);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("������Ϣ�����߳�����ʧ��\r\n");
	}
	CloseHandle(m_eventInvoke);
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{

	UINT nMode = isAutoClosed ? CSM_AUTOCLOSED : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadId, WM_SEND_PACK,
		(WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;
	}
	return ret;
}

void CClientSocket::threadFunc_msg()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message:%08X\r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}

	}
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc_msg();
	_endthreadex(0);
	return 0;
}


void CClientSocket::SendPacket_msg(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//TODO: ��Ϣ���ݽṹ: ���� ���� ģʽ  �ص���Ϣ�����ݽṹ:HWND MESSAGE
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	if (InitSocket()) {
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBufffer = (char*)strBuffer.c_str();
			while (m_sock!=INVALID_SOCKET)
			{
				int length = recv(m_sock, pBufffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || index>0) {//��ȡ�ɹ�
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBufffer, nLen);
					if (nLen > 0) {//����ɹ�
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSED) {
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBufffer, pBufffer + index, nLen);
				}
				else {//�Է��ر����׽��֣������쳣
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}

			}
		}
		else
		{
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
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

