#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#pragma warning(disable:4996)

#define WM_SEND_PACK (WM_USER+1) //���Ͱ�����
#define WM_SEND_PACK_ACK (WM_USER+2) //������Ӧ��

#pragma pack(push)
#pragma pack(1)
// ��Ϊ 1 �ֽڶ��� ȥ����ȫλ
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0){}

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

	CPacket(const BYTE* pData, size_t& nSize){
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

enum 
{
	CSM_AUTOCLOSED = 1, //Client Socket mode
};

typedef struct PacketData{
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}

}PACKET_DATA;



#define BUFFER_SIZE 4096000

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
	bool InitSocket();


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

	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);

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
	HANDLE m_eventInvoke;
	UINT m_nThreadId;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	int m_nIP; //��ַ
	int m_nPort; //�˿�
	SOCKET m_sock;
	bool m_bAutoClosed;
	HANDLE m_hThread;
	std::mutex m_lock;

	CPacket m_packet;
	static CClientSocket* m_instance;
	std::vector<char> m_buffer;
	std::list<CPacket> m_lstSend; //���Ͱ�����
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClosed;

	CClientSocket& operator=(const CClientSocket& s) {}
	CClientSocket(const CClientSocket& s);

	CClientSocket();

	~CClientSocket() {
		m_lstSend.clear();
		m_mapAck.clear();
		m_mapAutoClosed.clear();
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
		WaitForSingleObject(m_hThread, 100);
	}


	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) {
			return false;
		}
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(const CPacket& pack);
	void SendPacket_msg(UINT nMsg, WPARAM wParam, LPARAM lParam);

	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc_msg();
	//void threadFunc();
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

