#include "pch.h"
#include "ClientSocket.h"



// 静态变量的定义
CClientSocket* CClientSocket::m_instance = NULL;

// 辅助类
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pServer = CClientSocket::getInstance();