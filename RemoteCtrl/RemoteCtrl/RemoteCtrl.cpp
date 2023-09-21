// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "Tool.h"
#include "Queue.h"
#include <MSWSock.h>
#include <conio.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "Server.h"

//#pragma comment(linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:WinMainCRTStartup")


// server
// 唯一的应用程序对象

#define INVOKE_PATH _T("C:\\Users\\21843\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")
//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")

CWinApp theApp;

bool ChooseAutoInvoke(const CString& strPath) {
    if (PathFileExists(strPath)) {
        return true;
    }

    CString strInfo = _T("该程序只允许用于合法的用途\n");
    strInfo += _T("进入被监控状态\n");
    strInfo += _T("按下“是”将随系统启动而自动运行\n");
    strInfo += _T("按下“否”将是运行一次\n");
    strInfo += _T("按下“取消”将退出\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        if (CTool::WriteStartupDir(strPath) == false) {
            MessageBox(NULL, _T("复制文件失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}


void test() 
{
    //创建iocp 通过线程处理
    CQueue<std::string> lstStrings;
    ULONGLONG tick = GetTickCount64(), tick0 = GetTickCount64(), total = GetTickCount64();
    while (GetTickCount64() - total <= 1000) {
        //if (GetTickCount64() - tick0 > 5) 
        {
            lstStrings.PushBack("hello");
            tick0 = GetTickCount64();
        }
        //Sleep(1);
    }
    printf("size:%d, exit done.\r\n", lstStrings.Size());
    total = GetTickCount64();
    while (GetTickCount64() - total <= 1000) {
        //if (GetTickCount64() - tick > 5) 
        {
            std::string str;
            lstStrings.PopFront(str);
            tick = GetTickCount64();
            //printf("pop from queue:%s.\r\n", str.c_str());
        }
        //Sleep(1);
    }
    printf("size:%d, exit done.\r\n", lstStrings.Size());
    lstStrings.Clear();
}


class COverlappered {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    char m_buffer[4096];
    COverlappered() {
        m_operator = 0;
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        memset(m_buffer, 0, sizeof(m_buffer));
    }
};

void iocp() {
    CServer server;
    server.StartService();
    getchar();
}

int main()
{
    //示例
    if (CTool::Init() == false)
        return 1;
    iocp();


    /*
    if (CTool::isAdmin()) {
        if (CTool::Init() == false)
            return 1;

        OutputDebugString(L"current is run as administrator.\r\n");
        if (ChooseAutoInvoke(INVOKE_PATH) == false) {
            return 1;
        }
        CCommand ccmd;
        int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &ccmd);
        switch (ret)
        {
        case -1:
            MessageBox(NULL, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            break;
        case -2:
            MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            break;
        }
    }
    else {
        OutputDebugString(L"current is run as normal user.\r\n");
        if (CTool::RunAsAdmin() == false) {
            CTool::ShowError();
            return 1;
        }
    }*/
    return 0;
}