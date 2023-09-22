#include "pch.h"
#include "Server.h"
#include "Tool.h"

#pragma warning(disable:4407)

CClient::CClient() 
    : m_flags(0), m_isbusy(false),
    m_overlapped(new ACCEPTOVERLAPPED()),
    m_recv(new RECVOVERLAPPED()),
    m_send(new SENDOVERLAPPED()),
    m_vecSend(this, (SENDCALLBACK)&CClient::SendData)
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(sockaddr_in));
    memset(&m_raddr, 0, sizeof(sockaddr_in));
}

void CClient::SetOverlapped(PCLIENT& ptr)
{
    m_overlapped->m_client = ptr.get();
    m_recv->m_client = ptr.get();
    m_send->m_client = ptr.get();
}

CClient::operator LPOVERLAPPED()
{
    return &m_overlapped->m_overlapped;
}

LPWSABUF CClient::RecvWSABuffer()
{
    return &m_recv->m_wsabuffer;
}

LPWSABUF CClient::SendWSABuffer()
{
    return &m_send->m_wsabuffer;
}

LPOVERLAPPED CClient::RecvOverlapped()
{
    return &m_recv->m_overlapped;
}

LPOVERLAPPED CClient::SendOverlapped()
{
    return &m_send->m_overlapped;
}

int CClient::Recv()
{
    int ret = recv(m_sock, m_buffer.data() + m_usedbuf, m_buffer.size() - m_usedbuf, 0);
    if (ret <= 0)return -1;
    m_usedbuf += (size_t)ret;
    //TODO解析数据
    CTool::Dump((BYTE*)m_buffer.data(), ret);
    return 0;
}

int CClient::Send(void* buffer, size_t nSize)
{
    std::vector<char> data(nSize);
    memcpy(data.data(), buffer, nSize);
    if (m_vecSend.PushBack(data)) {
        return 0;
    }
    return -1;
}

int CClient::SendData(std::vector<char>& data)
{
    if (m_vecSend.Size() > 0) {
        int ret = WSASend(m_sock, SendWSABuffer(), 1,
            &m_recvlen, m_flags, &m_send->m_overlapped, NULL);
        if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)){
            CTool::ShowError();
            return -1;
        }
    }
    return 0;
}

template<EOperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
    m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
    m_operator = EAccept;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024);
    m_server = NULL;
}

template<EOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
    int lLength = 0, rLength = 0;
    if (m_client->GetBufferSize() > 0) { //问题
        sockaddr* plocal = NULL, * premote = NULL;
        GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)&plocal, &lLength, //地址
            (sockaddr**)&premote, &rLength);

        memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
        memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
        m_server->BindNewSocket(*m_client);
        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(),
            m_client->RecvOverlapped(), NULL);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            //TODO 错误
            TRACE("WSARecv ret %d, errno %d\r\n", ret, WSAGetLastError());
        }
        
        if (m_server->NewAccept() == FALSE)
        {
            return -2;
        }
    }
    return -1;
}

template<EOperator op>
inline SendOverlapped<op>::SendOverlapped()
{
    m_operator = op;
    m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}

template<EOperator op>
inline RecvOverlapped<op>::RecvOverlapped()
{
    m_operator = op;
    m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped::RecvWorker);
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}



CServer::~CServer()
{
    closesocket(m_sock);
    std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
    for (; it != m_client.end(); it++) {
        it->second.reset();
    }
    m_client.clear();
    CloseHandle(m_hIOCP);
    m_pool.Stop();
    WSACleanup();
}

bool CServer::StartService()
{
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
    //从线程池分配一个线程用于threadIocp
    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
    if (NewAccept() == false) return false;
    return true;
}

bool CServer::NewAccept()
{
    PCLIENT pClient(new CClient());
    pClient->SetOverlapped(pClient);
    m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
    if (AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16,
        sizeof(sockaddr_in) + 16, *pClient, *pClient) == FALSE)
    {
        TRACE("%d\r\n", GetLastError());
        if (WSAGetLastError() != WSA_IO_PENDING) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
    }
    return true;
}

bool CServer::BindNewSocket(SOCKET s)
{
    CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
    return false;
}

void CServer::CreateSocket()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    //可重用
    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int CServer::threadIocp()
{
    DWORD transferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
        if (CompletionKey != 0) {
            EOverlappered* pOverlapped = CONTAINING_RECORD(lpOverlapped, EOverlappered, m_overlapped);
            TRACE("pOverlapped->m_operator:%d\r\n", pOverlapped->m_operator);
            pOverlapped->m_server = this;
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
