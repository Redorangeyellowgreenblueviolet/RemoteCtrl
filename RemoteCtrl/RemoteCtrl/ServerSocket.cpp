#include "pch.h"
#include "ServerSocket.h"

// ��̬�����Ķ���
CServerSocket* CServerSocket::m_instance = NULL;

// ������
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pServer = CServerSocket::getInstance();