#pragma once
#include "pch.h"
#include "framework.h"
#define BUFFER_SIZE 4096

void Dump(BYTE* pData, size_t nSize);

#pragma pack(push)
#pragma pack(1)
// ��Ϊ 1 �ֽڶ��� ȥ����ȫλ
class CPacket {
public:
	CPacket():sHead(0),nLength(0), sCmd(0), sSum(0){}
	
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
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
	}

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

	const char* Data() {//������
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
	std::string strOut; // ȫ��
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
}MOUSEEV, *PMOUSEEV;


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
		TRACE("Enter AcceptClient\r\n");
		sockaddr_in client_addr;
		int client_size = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &client_size);
		TRACE("m_client = %d \r\n", m_client);
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
		if (buffer == NULL) {
			TRACE("memory not enough");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) { //Question ƴ��
			size_t len = recv(m_client, buffer + index, sizeof(buffer), 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			TRACE("len:%d\r\n", len);
			index += len;
			//lenʹ�û��������� ���ر�Ϊ�ѽ����ĳ���
			len = index;
			m_packet =  CPacket((BYTE*)buffer, len);
			if (len > 0) { //Question ����ȡǰ�ƶ� �������� 
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
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
	bool Send(const char* pData, int nSize) {
		if (m_client == -1) {
			return false;
		}
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_client == -1)
			return false;
		//OutputDebugStringA(pack.Data());
		Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string &strPath) {
		if (m_packet.sCmd == 2 || m_packet.sCmd ==3 || m_packet.sCmd == 4) {
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

	void CloseClient() { //�رտͻ�������
		closesocket(m_client);
		m_client = INVALID_SOCKET;
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

