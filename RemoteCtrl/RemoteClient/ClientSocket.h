#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#pragma warning(disable:4996)


#pragma pack(push)
#pragma pack(1)
// ��Ϊ 1 �ֽڶ��� ȥ����ȫλ
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0), m_hEvent(INVALID_HANDLE_VALUE) {}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += strData[j] & 0xFF;
		}
		m_hEvent = hEvent;
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
		m_hEvent = pack.m_hEvent;
	}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //Qu
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
			m_hEvent = pack.m_hEvent;
		}
		return *this;
	}

	CPacket(const BYTE* pData, size_t& nSize):m_hEvent(INVALID_HANDLE_VALUE){
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				// ��ͷ
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {// �����ݿ��ܲ�ȫ
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) { //��δ��ȫ�յ�
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}

	int Size() {//�����ݴ�С
		return nLength + 6;
	}

	const char* Data(std::string& strOut) const {//������
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		// ��ֵ
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}


	~CPacket() {}

public:
	WORD sHead; // �̶�ͷ�� FEFF
	DWORD nLength; // ������ ��������--У���
	WORD sCmd; // ��������
	std::string strData; //������
	WORD sSum; //��У��
	HANDLE m_hEvent;
};

#pragma pack(pop)

typedef struct MOUSEEVENT { // �������
	MOUSEEVENT() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // ��� �ƶ� ˫��
	WORD nButton; // ��� �Ҽ� �м�
	POINT ptXY; // ����
}MOUSEEV, * PMOUSEEV;



typedef struct file_info {
	file_info() {
		IsInvalid = 0;
		IsDirectory = 0;
		HasNext = 1;
		memset(szFileName, 0, sizeof(szFileName));
	}
	char szFileName[256]; //�ļ���
	bool IsInvalid; //�Ƿ���Ч 0��Ч
	bool IsDirectory; //�Ƿ�ΪĿ¼ 0���� 1��
	bool HasNext; //�Ƿ��к��� 0 û�� 1��
}FILEINFO, * PFILEINFO;

#define BUFFER_SIZE 2048000

std::string GetErrInfo(int wsaErrno);


class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		// ��̬����û�� this ָ��
		if (m_instance == NULL)
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	// ��ʼ���׽���
	bool InitSocket() {
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


	// �ڴ��ͷ� �����յ�����˵Ķ���� ����ÿ��delete
	int DealCommand() { //��������
		if (m_sock == -1) {
			return -1;
		}
		//Question ֮ǰ���յ�ֻ������һ��������� index��0 ���¶���
		//memset(buffer, 0, BUFFER_SIZE);
		//ÿ����Ҫ�����ϴε�λ�� �����þ�̬����
		//TODO: ���߳̽��ճ�������
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			//TRACE("len:%d index:%d\r\n", len, index);
			if ((int)len <= 0 && (int)index<=0) {
				return -1;
			}
			index += len;
			//len��ȡ�Ĵ�С ���ر�Ϊ�ѽ����ĳ���
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) { //Question ����ȡǰ�ƶ� ��������
				TRACE("len:%d index:%d\r\n", len, index);
				memmove(buffer, buffer + len, index - len);
				//memset(buffer + index - len, 0, len);
				index -= len;
				TRACE("client strData: [%s]\r\n", m_packet.strData.c_str());
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	/*
	* �������
	* ��ͷ ���� ���� ���� У��
	* ��ͷ FFFE FEFE 1.��ֹ�𻵵����� 2.��̽�� 3.����Ӧ�õ���
	*
	* У�� ��/CRC
	*/

	bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, BOOL isAutoClosed = TRUE);

	bool GetFilePath(std::string strPath) {
		if (m_packet.sCmd == 2 || m_packet.sCmd == 3 || m_packet.sCmd == 4) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	void UpdateAddress(int nIP, int nPort) {
		if (m_nIP != nIP || m_nPort != nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

private:
	bool m_bAutoClosed;
	std::list<CPacket> m_lstSend; //���Ͱ�����
	std::map<HANDLE, std::list<CPacket>> m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClosed;
	int m_nIP; //��ַ
	int m_nPort; //�˿�

	std::vector<char> m_buffer;
	static CClientSocket* m_instance;
	SOCKET m_sock;
	CPacket m_packet;

	CClientSocket& operator=(const CClientSocket& s) {}
	CClientSocket(const CClientSocket& s)
	{
		m_bAutoClosed = s.m_bAutoClosed;
		m_sock = s.m_sock;
		m_nIP = s.m_nIP;
		m_nPort = s.m_nPort;
	}
	CClientSocket() : m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClosed(TRUE)
	{
		if (!InitSockEnv())
		{
			//Question
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���."), _T("��ʼ������."), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}


	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) {
			return false;
		}
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(const CPacket& pack);


	static void threadEntry(void* arg);
	void threadFunc();

	// ��ʼ���׽��ֻ���
	bool InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return false;
		}
		return true;
	}


	static void releaseInstance() {
		if (m_instance) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			// �ڲ� delete ���Ե�����������
			delete tmp;
		}
	}

	class CHelper {
	public:
		CHelper()
		{
			//����ڲ��з���Ȩ��
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

