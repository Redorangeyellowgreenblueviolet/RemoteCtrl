#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include <list>

#define BUFFER_SIZE 4096




typedef void(*SOCK_CALLBACK)(void*, int, std::list<CPacket>&, CPacket&);


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		// 静态函数没有 this 指针
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	int Run(SOCK_CALLBACK callback, void* arg, short port=9527) {
		bool ret = InitSocket(port);
		if (ret == FALSE) return -1;
		std::list<CPacket> lstPacket;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (TRUE) {
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			//短连接 适合小量数据 获取分区等功能
			//长连接 效率高 适合大量数据频繁发送 传输文件
			// 接收数据
			int ret = DealCommand();
			TRACE("DealCommand ret:%d\r\n", ret);
			if (ret > 0) {
				m_callback(m_arg, ret, lstPacket, m_packet);
				while (lstPacket.size() > 0) {
					Send(lstPacket.front());
					lstPacket.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}	

protected:
	// 初始化套接字
	bool InitSocket(short port) {
		if (m_sock == -1) return false;

		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		// bind
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
			return false;
		}
		if (listen(m_sock, 1) == -1) {
			return false;
		}
		return true;
	}

	// 接收连接
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

	// 处理命令
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
		while (true) { //Question 拼接
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			//TRACE("len:%d\r\n", len);
			index += len;
			//len使用缓冲区长度 返回变为已解析的长度
			len = index;
			m_packet =  CPacket((BYTE*)buffer, len);
			if (len > 0) { //Question 缓冲取前移动 覆盖问题 
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;
			}

		}
		return -1;
	}
	/*
	* 包的设计
	* 包头 长度 命令 数据 校验
	* 包头 FFFE FEFE 1.防止损坏的数据 2.嗅探包 3.其他应用的误发
	* 
	* 校验 和/CRC
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
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	void CloseClient() 
	{ //关闭客户端连接
		if (m_client != INVALID_SOCKET) 
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}

private:
	SOCK_CALLBACK m_callback;
	void* m_arg;

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
			//Question
			MessageBox(NULL, _T("无法初始化套接字环境."), _T("初始化错误."), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

	// 初始化套接字环境
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
			// 内部 delete 可以调用析构函数
			delete tmp;
		}
	}

	class CHelper {
	public:
		CHelper()
		{
			//类的内部有访问权限
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

