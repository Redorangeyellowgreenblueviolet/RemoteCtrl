#include "pch.h"
#include "Server.h"

#pragma warning(disable:4407)

CClient::CClient() 
    : m_flags(0), m_isbusy(false),
    m_overlapped(new ACCEPTOVERLAPPED()),
    m_recv(new RECVOVERLAPPED()),
    m_send(new SENDOVERLAPPED)
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(sockaddr_in));
    memset(&m_raddr, 0, sizeof(sockaddr_in));
}

void CClient::SetOverlapped(PCLIENT& ptr)
{
    m_overlapped->m_client = ptr;
    m_recv->m_client = ptr;
    m_send->m_client = ptr;
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
    if (*(LPDWORD)*m_client.get() > 0) { //ÎÊÌâ
        GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength, //µØÖ·
            (sockaddr**)m_client->GetRemoteAddr(), &rLength);
        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), *m_client, NULL);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            //TODO ´íÎó
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
    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
    if (NewAccept() == false) return false;
    return true;
}

int CServer::threadIocp()
{
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
