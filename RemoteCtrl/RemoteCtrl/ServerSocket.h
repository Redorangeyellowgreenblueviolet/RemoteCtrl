#pragma once
#include "pch.h"
#include "framework.h"
#define BUFFER_SIZE 4096

class CPacket {

public:
	CPacket():sHead(0),nLength(0), sCmd(0), sSum(0){}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) { //Qu
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	CPacket(const BYTE* pData, size_t& nSize) {
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
			sum += BYTE(strData[i]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}



	~CPacket() {}

public:
	WORD sHead; // �̶�ͷ�� FEFF
	DWORD nLength; // ������ ��������--У���
	WORD sCmd; // ��������
	std::string strData; //������
	WORD sSum; //��У��

};


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		// ��̬����û�� this ָ��
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	// ��ʼ���׽���
	bool InitSocket() {
		if (m_sock == -1) return false;

		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9527);
		// bind
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
			return false;
		}
		if (listen(m_sock, 1) == -1) {
			return false;
		}
		return true;
	}

	// ��������
	bool AcceptClient() {
		sockaddr_in client_addr;
		int client_size = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &client_size);
		if (m_client == -1) {
			return false;
		}
		return true;
	}

	// ��������
	int DealCommand() {
		if (m_client == -1) {
			return -1;
		}
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			//len��ȡ�Ĵ�С ���ر�Ϊ�ѽ����ĳ���
			len = index;
			m_packet =  CPacket((BYTE*)buffer, len);
			if (len > 0) { //Qu ����ȡǰ�ƶ� �������� 
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}

		}
	}

	/*
	* �������
	* ��ͷ ���� ���� ���� У��
	* ��ͷ FFFE FEFE 1.��ֹ�𻵵����� 2.��̽�� 3.����Ӧ�õ���
	* 
	* У�� ��/CRC
	*/
	bool Send(const char* pData, int nSize) {
		if (m_client == -1) {
			return false;
		}
		return send(m_client, pData, nSize, 0) > 0;
	}



private:

	static CServerSocket* m_instance;
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket& s) {}
	CServerSocket(const CServerSocket& s) {
		m_sock = s.m_sock;
		m_client = s.m_client;
	}
	CServerSocket(){
		m_client = INVALID_SOCKET;
		if (!InitSockEnv())
		{
			//Qu
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���."), _T("��ʼ������."), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

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
			CServerSocket* tmp = m_instance;
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
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};
