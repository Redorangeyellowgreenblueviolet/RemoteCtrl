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
		AfxMessageBox(_T("指定的IP不存在!"));
		return false;
	}

	// connect
	if (connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		// Question
		AfxMessageBox(_T("连接失败"));
		TRACE("连接失败: %d %s.\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed)
{
	if (m_hThread == INVALID_HANDLE_VALUE) {
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadId);
	}
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSED : 0;
	std::string strOut;
	pack.Data(strOut);
	return PostThreadMessage(m_nThreadId, WM_SEND_PACK, 
		(WPARAM)new PACKET_DATA(strOut.c_str(), strOut.size(), nMode), (LPARAM)hWnd);

}
/*
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, BOOL isAutoClosed)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("start client socket thread.\r\n");
	}
	TRACE("cmd:%d event:%08X threadID:%d\r\n", pack.sCmd, pack.m_hEvent, GetCurrentThreadId());
	//线程安全
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
*/

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
		MessageBox(NULL, _T("无法初始化套接字环境."), _T("初始化错误."), MB_OK | MB_ICONERROR);
		exit(0);
	}
	//m_lstSend.clear();
	//m_mapAck.clear();
	//m_mapAutoClosed.clear();
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
{//TODO: 消息数据结构: 数据 长度 模式  回调消息的数据结构:HWND MESSAGE
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
				if (length > 0 || index>0) {//读取成功
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBufffer, nLen);
					if (nLen > 0) {//解包成功
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), 0);
						if (data.nMode & CSM_AUTOCLOSED) {
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBufffer, pBufffer + index, nLen);
				}
				else {//对方关闭了套接字，网络异常
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

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc_msg();
	_endthreadex(0);
	return 0;
}
/*
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
			std::map<HANDLE, std::list<CPacket>&>::iterator it_ack 
				= m_mapAck.find(head.m_hEvent);
			//自动关闭
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
							//通知对应事件
							pack.m_hEvent = head.m_hEvent;
							TRACE("recv.shead:%x, cmd:%d\r\n", pack.sHead, pack.sCmd);
							it_ack->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it_auto->second) {//问题
								CloseSocket();
								SetEvent(head.m_hEvent);
								break;
							}
						}
					}
					else if (len <= 0 && index <= 0) {
						TRACE("&& len:%d, index:%d\r\n", len, index);
						CloseSocket();
						SetEvent(head.m_hEvent); //等待服务端关闭，再通知完成
						
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
*/