#pragma once
#include "pch.h"
#include "Thread.h"
#include "Queue.h"
#include <map>
#include <MSWSock.h>

enum EOperator
{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};


class CServer;
class CClient;
typedef std::shared_ptr<CClient> PCLIENT;

template<EOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template<EOperator> class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EOperator> class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;



class EOverlappered {
public:
    OVERLAPPED m_overlapped; //重叠结构
    DWORD m_operator; // enum EOperator
    std::vector<char> m_buffer;
    ThreadWorker m_worker; //处理函数
    CServer* m_server; //服务器
    PCLIENT m_client; //对应的客户端
    WSABUF m_wsabuffer;
};

class CClient {
public:
    CClient();
    ~CClient() {closesocket(m_sock);}
    void SetOverlapped(PCLIENT& ptr);

    operator SOCKET() {
        return m_sock;
    }
    operator PVOID() {
        return &m_buffer[0];
    }
    operator LPOVERLAPPED();
    operator LPDWORD() {
        return &m_recvlen;
    }
    LPWSABUF RecvWSABuffer();
    LPWSABUF SendWSABuffer();

    DWORD& flags() {
        return m_flags;
    }
    sockaddr_in* GetLocalAddr() {
        return &m_laddr;
    }
    sockaddr_in* GetRemoteAddr() {
        return &m_raddr;
    }
    size_t GetBufferSize()const {
        return m_buffer.size();
    }

    int Recv() {
        int ret = recv(m_sock, m_buffer.data() + m_usedbuf, m_buffer.size() - m_usedbuf, 0);
        if (ret <= 0)return -1;
        m_usedbuf += (size_t)ret;
        //TODO解析数据
        return 0;
    }


private:
    SOCKET m_sock;
    DWORD m_recvlen;
    DWORD m_flags;
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recv;
    std::shared_ptr<SENDOVERLAPPED> m_send;
    std::vector<char> m_buffer;
    size_t m_usedbuf; //已使用的缓冲区
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isbusy;
};



//Accept
template<EOperator>
class AcceptOverlapped :public EOverlappered, ThreadFuncBase
{//问题
public:
    AcceptOverlapped();
    int AcceptWorker();
public:
    PCLIENT m_client;
};




//Recv
template<EOperator>
class RecvOverlapped :public EOverlappered, ThreadFuncBase
{//问题
public:
    RecvOverlapped();
    int RecvWorker() {
        int ret = m_client->Recv();
        return ret;
    }

};



template<EOperator>
class SendOverlapped :public EOverlappered, ThreadFuncBase
{//问题
public:
    SendOverlapped();
    int SendWorker() {
        //TODO
        return - 1;
    }
};


template<EOperator>
class ErrorOverlapped :public EOverlappered, ThreadFuncBase
{//问题
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker() {
        //TODO
        return -1;
    }
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;



class CServer :
    public ThreadFuncBase
{
public:
    CServer(const std::string& ip="0.0.0.0", short port = 9527):m_pool(10)
    {

        m_hIOCP = INVALID_HANDLE_VALUE;
        //异步套接字 重叠结构
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        m_addr.sin_port = htons(port);
    }
    ~CServer() {}

    bool StartService();
    bool NewAccept() {
        PCLIENT pClient(new CClient());
        pClient->SetOverlapped(pClient);
        m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
        if (AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16, *pClient, *pClient) == FALSE)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }

private:
    void CreateSocket() {
        m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        //可重用
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    }
    

    int threadIocp();
private:
    ThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<CClient>> m_client;
};