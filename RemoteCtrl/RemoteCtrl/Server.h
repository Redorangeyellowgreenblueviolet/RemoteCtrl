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

class EOverlappered {
public:
    OVERLAPPED m_overlapped; //�ص��ṹ
    DWORD m_operator; // enum EOperator
    std::vector<char> m_buffer;
    ThreadWorker m_worker; //��������
    CServer* m_server; //������
};

template<EOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

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
    sockaddr_in* GetLocalAddr() {
        return &m_laddr;
    }
    sockaddr_in* GetRemoteAddr() {
        return &m_raddr;
    }

private:
    SOCKET m_sock;
    DWORD m_recvlen;
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::vector<char> m_buffer;
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isbusy;
};



//Accept
template<EOperator>
class AcceptOverlapped :public EOverlappered, ThreadFuncBase
{//����
public:
    AcceptOverlapped();
    int AcceptWorker();
public:
    PCLIENT m_client;
};




//Recv
template<EOperator>
class RecvOverlapped :public EOverlappered, ThreadFuncBase
{//����
public:
    RecvOverlapped() :m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024*256);
    }
    int RecvWorker() {
        //TODO
    }
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;


template<EOperator>
class SendOverlapped :public EOverlappered, ThreadFuncBase
{//����
public:
    SendOverlapped() :m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024*256);
    }
    int SendWorker() {
        //TODO
    }
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EOperator>
class ErrorOverlapped :public EOverlappered, ThreadFuncBase
{//����
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker() {
        //TODO
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
        //�첽�׽��� �ص��ṹ
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        m_addr.sin_port = htons(port);
    }
    ~CServer() {}

    bool StartService() {
        CreateSocket();
        if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        if (listen(m_sock, 5) == -1) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }

        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        if (m_hIOCP == NULL) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
        m_pool.Invoke();
        m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
        if (NewAccept() == false) return false;
        return true;
    }
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
        //������
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    }
    

    int threadIocp() {
        DWORD transferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* lpOverlapped = NULL;
        if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
            if (transferred > 0 && (CompletionKey != 0)) {
                EOverlappered* pOverlapped = CONTAINING_RECORD(lpOverlapped, EOverlappered, m_overlapped);
                switch (pOverlapped->m_operator)
                {
                case EAccept: 
                {
                    ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
                    m_pool.DispatchWorker(pAccept->m_worker);
                }
                    break;
                case ERecv:
                {
                    RECVOVERLAPPED* pRecv = (RECVOVERLAPPED*)pOverlapped;
                    m_pool.DispatchWorker(pRecv->m_worker);
                }
                    break;
                case ESend:
                {
                    SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
                    m_pool.DispatchWorker(pSend->m_worker);
                }
                    break;
                case EError:
                {
                    ERROROVERLAPPED* pError = (ERROROVERLAPPED*)pOverlapped;
                    m_pool.DispatchWorker(pError->m_worker);
                }
                    break;
                }
            }
            else
            {
                return -1;
            }
        }
        return 0;
    }
private:
    ThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<CClient>> m_client;
};
