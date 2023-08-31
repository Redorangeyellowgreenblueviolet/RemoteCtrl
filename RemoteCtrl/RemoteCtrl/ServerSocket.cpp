#include "pch.h"
#include "ServerSocket.h"

// 静态变量的定义
CServerSocket* CServerSocket::m_instance = NULL;

// 辅助类
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pServer = CServerSocket::getInstance();