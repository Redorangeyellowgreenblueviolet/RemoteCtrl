// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma comment(linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:WinMainCRTStartup")


// server
// 唯一的应用程序对象

CWinApp theApp;

//开机启动时，程序的权限跟随启动用户的，当两者权限不同时，通常程序启动失败
//开机启动对环境变量有影响，如果依赖动态库，可能启动失败
//复制dll到system32或者SysWOW64下，或者采用静态库
void WriteRegisterTable(const CString& strPath) {

    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    char sPath[MAX_PATH] = "";
    char sSys[MAX_PATH] = "";
    std::string strExe = "\\RemoteCtrl.exe ";
    GetCurrentDirectoryA(MAX_PATH, sPath);
    GetSystemDirectoryA(sSys, MAX_PATH);
    std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
    int ret = system(strCmd.c_str());
    TRACE("system %d\r\n", ret);
    HKEY hKey = NULL;
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("设置自启开机启动失败\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }

    ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
    if (ret != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("设置自启开机启动失败\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    RegCloseKey(hKey);
}

void WriteStartupDir(const CString& strPath) {
    
    CString strCmd = GetCommandLine();
    strCmd.Replace(_T("\""), _T(""));
    //fopen CFile system(copy) CopyFile
    bool ret = CopyFile(strCmd, strPath, FALSE);
    if (ret == false) {
        MessageBox(NULL, _T("复制文件失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
}


void ChooseAutoInvoke() {
    //CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));
    CString strPath = _T("C:\\Users\\21843\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
    if (PathFileExists(strPath)) {
        return;
    }

    CString strInfo = _T("该程序只允许用于合法的用途\n");
    strInfo += _T("进入被监控状态\n");
    strInfo += _T("按下“是”将随系统启动而自动运行\n");
    strInfo += _T("按下“否”将是运行一次\n");
    strInfo += _T("按下“取消”将退出\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        WriteStartupDir(strPath);
    }
    else if (ret == IDCANCEL) {
        exit(0);
    }
    return;
}


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
            //ChooseAutoInvoke();
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
