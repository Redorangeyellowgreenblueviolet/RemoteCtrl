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
    CClient* m_client; //对应的客户端
    WSABUF m_wsabuffer;

    virtual ~EOverlappered() {
        m_buffer.clear();
    }
};

class CClient:public ThreadFuncBase
{
public:
    CClient();
    ~CClient() {
        closesocket(m_sock);
        m_recv.reset();
        m_send.reset();
        m_overlapped.reset();
        m_buffer.clear();
        m_vecSend.Clear();
    }
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
    LPOVERLAPPED RecvOverlapped();
    LPOVERLAPPED SendOverlapped();

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

    int Recv();
    int Send(void* buffer, size_t nSize);
    int SendData(std::vector<char>& data);
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
    CSendQueue<std::vector<char>> m_vecSend; //发送数据队列
};



//Accept
template<EOperator>
class AcceptOverlapped :public EOverlappered, public ThreadFuncBase
{//问题
public:
    AcceptOverlapped();
    int AcceptWorker();

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
        //send可能不会立刻完成

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
    ~CServer();

    bool StartService();
    bool NewAccept();
    bool BindNewSocket(SOCKET s);

private:
    void CreateSocket();
    int threadIocp();
private:
    ThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<CClient>> m_client;
};