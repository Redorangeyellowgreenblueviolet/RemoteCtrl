// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "Tool.h"
#include "Queue.h"
#include<conio.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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


enum 
{
    IocpListEmpty,
    IocpListPush,
    IocpListPop
};

typedef struct IocpParam {
    int nOperator; //操作
    std::string strData; //数据
    _beginthread_proc_type cbFunc; //回调
    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
        nOperator = op;
        strData = sData;
        cbFunc = cb;
    }
    IocpParam() {
        nOperator = -1;
    }
}IOCP_PARAM;


void threadQueue(HANDLE hIOCP)
{
    std::list<std::string> lstString;
    DWORD dwTransferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* pOverlapped = NULL;
    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
    {
        if (dwTransferred == 0 || CompletionKey == NULL) {
            printf("thread is prepare to exit\r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOperator == IocpListPush) {
            lstString.push_back(pParam->strData);
        }
        else if (pParam->nOperator == IocpListPop) {
            std::string* pStr = NULL;
            if (lstString.size() > 0) {
                pStr = new std::string(lstString.front());
                lstString.pop_front();
            }
            if (pParam->cbFunc) {
                pParam->cbFunc(pStr);
            }
        }
        else if (pParam->nOperator == IocpListEmpty)
        {
            lstString.clear();
        }
        delete pParam; //释放内存
    }
}

void threadQueueEntry(HANDLE hIOCP)
{
    threadQueue(hIOCP);
    _endthread();
}


void func(void* arg)
{
    std::string* pstr = (std::string*)arg;
    if(pstr != NULL){
        printf("pop from list:%s\r\n", pstr->c_str());
        delete pstr;
    }
    else
    {
        printf("list is empty.\r\n");
    }

}

int main()
{
    //示例
    if (CTool::Init() == false)
        return 1;
    //创建iocp 通过线程处理
    printf("press and key to exit...\r\n");
    CQueue<std::string> lstStrings;
    ULONGLONG tick = GetTickCount64(), tick0 = GetTickCount64();
    while (_kbhit() == 0) {
        if (GetTickCount64() - tick0 > 1300) {
            lstStrings.PushBack("hello");
            tick0 = GetTickCount64();
        }

        if (GetTickCount64() - tick > 2000) {
            std::string str{};
            lstStrings.PopFront(str);
            tick = GetTickCount64();
            printf("pop from queue:%s\r\n.", str.c_str());
        }
        Sleep(1);
    }
    printf("size:%d, exit done.\r\n", lstStrings.Size());
    lstStrings.Clear();
    printf("size:%d, exit done.\r\n", lstStrings.Size());
    exit(0);


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