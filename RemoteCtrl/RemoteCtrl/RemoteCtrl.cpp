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

void iocp();

void initsock();
void clearsock();
void udp_server();
void udp_client(bool ishost = true);

int main(int argc, char* argv[])
{
    //示例
    if (CTool::Init() == false)
        return 1;
    if (argc == 1) {
        initsock();
        char strDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, strDir);
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(STARTUPINFO));
        memset(&pi, 0, sizeof(PROCESS_INFORMATION));
        std::string strCmd = argv[0];
        strCmd += " 1";
        bool bret = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, strDir, &si, &pi);
        if (bret) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            strCmd += " 2";

            bool bret = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, strDir, &si, &pi);
            if (bret) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                udp_server();
            }
        }
    }
    else if (argc == 2) {
        udp_client();
    }
    else {
        udp_client(false);
    }
    clearsock();

    //iocp();
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


void iocp() {
    CServer server;
    //开启监听
    server.StartService();
    system("pause");
}

void initsock() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

}

void clearsock() {
    WSACleanup();
}

void udp_server() {
    printf("(%d):%s, server\r\n", __LINE__, __FUNCTION__);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("(%d):%s, socket error\r\n", __LINE__, __FUNCTION__);
        return;
    }
    std::list<sockaddr_in> lstclients;
    sockaddr_in server, client;
    memset(&server, 0, sizeof(sockaddr_in));
    memset(&client, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(sock, (struct sockaddr*)&server, sizeof(server))==-1) {
        printf("(%d):%s, bind error\r\n", __LINE__, __FUNCTION__);
        closesocket(sock);
        return;
    }
    
    std::string buf;
    buf.resize(4096);
    memset((char*)buf.c_str(), 0, buf.size());
    int len = sizeof(client), ret = 0;
    //udp操作自己的套接字，根据对方的地址recvfrom sendto
    while (!_kbhit()) {
        ret = recvfrom(sock, (char*)buf.c_str(), buf.size(), 0, (sockaddr*)&client, &len);
        printf("(%d):%s, ret %d\r\n", __LINE__, __FUNCTION__, ret);
        if (ret > 0) {
            if (lstclients.size() <= 0) {//第一个接入
                lstclients.push_back(client);
                //CTool::Dump((BYTE*)buf.c_str(), ret);
                printf("(%d):%s, host port:%d \r\n", __LINE__, __FUNCTION__, ntohs(client.sin_port));
                ret = sendto(sock, buf.c_str(), ret, 0, (sockaddr*)&client, len);
            }
            else {
                printf("(%d):%s, another port:%d \r\n", __LINE__, __FUNCTION__, ntohs(client.sin_port));
                memset((char*)buf.c_str(), 0, buf.size());
                memcpy((void*)buf.c_str(), &lstclients.front(), sizeof(lstclients.front()));
                ret = sendto(sock, buf.c_str(), sizeof(lstclients.front()), 0, (sockaddr*)&client, len);
            }
        }
        else {
            printf("(%d):%s, recv error\r\n", __LINE__, __FUNCTION__);
        }
        Sleep(1);
    }
    closesocket(sock);
}
void udp_client(bool ishost) {
    Sleep(1000);
    sockaddr_in server, client;
    memset(&server, 0, sizeof(sockaddr_in));
    memset(&client , 0, sizeof(sockaddr_in));
    int len = sizeof(client);
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("(%d):%s, socket error\r\n", __LINE__, __FUNCTION__);
        return;
    }

    if (ishost) {
        std::string msg = "hello\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
        printf("(%d):%s,host client ret %d\r\n", __LINE__, __FUNCTION__, ret);
        if (ret > 0) {
            msg.resize(4096);
            memset((char*)msg.c_str(), 0, msg.size());
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client , &len);
            printf("(%d):%s, ret:%d\r\n", __LINE__, __FUNCTION__, ret);
            if (ret > 0) {
                printf("(%d):%s, port:%d \r\n", __LINE__, __FUNCTION__, ntohs(client.sin_port));
            }
            memset((char*)msg.c_str(), 0, msg.size());
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("(%d):%s, ret:%d\r\n", __LINE__, __FUNCTION__, ret);
            if (ret > 0) {
                printf("(%d):%s, port:%d \r\n", __LINE__, __FUNCTION__, ntohs(client.sin_port));
                printf("(%d):%s, msg:%s \r\n", __LINE__, __FUNCTION__, msg.c_str());
            }
        }
    }
    else {
        Sleep(1000);
        std::string msg = "host client to server\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
        printf("(%d):%s, client ret %d\r\n", __LINE__, __FUNCTION__, ret);
        if (ret > 0) {
            msg.resize(4096);
            memset((char*)msg.c_str(), 0, msg.size());
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("(%d):%s, ret:%d\r\n", __LINE__, __FUNCTION__, ret);
            if (ret > 0) {
                sockaddr_in addr;
                memcpy((char*)&addr, msg.c_str(), sizeof(addr));
                sockaddr_in* paddr = (sockaddr_in*)&addr;
                msg = "another client to host\n";
                ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
                printf("(%d):%s, ret%d port:%d \r\n", __LINE__, __FUNCTION__, ret, ntohs(client.sin_port));
                printf("(%d):%s, paddr port:%d \r\n", __LINE__, __FUNCTION__, ntohs(paddr->sin_port));
            }
        }
    }
}



/*
int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CCommand ccmd;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &ccmd);
            switch (ret)
            {
            case -1:
                MessageBox(NULL, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}

*/