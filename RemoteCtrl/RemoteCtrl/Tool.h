#pragma once
#include "framework.h"

class CTool
{
public:
    static void Dump(BYTE* pData, size_t nSize) {

        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0))
                strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

    static bool isAdmin() {//判断权限
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve;
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
            ShowError();
            return false;
        }
        CloseHandle(hToken);
        if (len == sizeof(eve)) {
            return eve.TokenIsElevated;
        }
        TRACE("length of token information is %d\r\n", len);
        return false;
    }

    static bool RunAsAdmin() {//获取管理员权限 使用该权限创建进程
    //本地策略组 开启Administrator账户 禁止空密码只能控制台登录
        STARTUPINFO si{ 0 };
        PROCESS_INFORMATION pi{ 0 };
        //TCHAR sPath[MAX_PATH] = _T("D:\\nan\\study_project\\remote_control\\RemoteCtrl\\Debug\\RemoteCtrl.exe");
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        bool ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL,
            sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
        if (!ret) {
            ShowError();
            MessageBox(NULL, sPath, _T("创建进程失败"), 0);
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    static void ShowError()
    {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno); //标准C语言库
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf, 0, NULL
        );
        OutputDebugString(lpMessageBuf);
        MessageBox(NULL, lpMessageBuf, _T("程序错误"), 0);
        LocalFree(lpMessageBuf);
    }

    //开机启动时，程序的权限跟随启动用户的，当两者权限不同时，通常程序启动失败
    //开机启动对环境变量有影响，如果依赖动态库，可能启动失败
    //复制dll到system32或者SysWOW64下，或者采用静态库
    static bool WriteRegisterTable(const CString& strPath) {
        CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        bool ret = CopyFile(sPath, strPath, FALSE);
        if (ret == false) {
            MessageBox(NULL, _T("复制文件失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
        HKEY hKey = NULL;
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自启开机启动失败\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }

        ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自启开机启动失败\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
        RegCloseKey(hKey);
        return true;
    }

    static bool WriteStartupDir(const CString& strPath) {//开机启动
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        //fopen CFile system(copy) CopyFile
        return CopyFile(sPath, strPath, FALSE);
    }

    static bool Init()
    {//用户mfc命令行程序初始化
        HMODULE hModule = ::GetModuleHandle(nullptr);
        if (hModule == nullptr) {
            wprintf(L"错误: GetModuleHandle 失败\n");
            return false;
        }
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            wprintf(L"错误: MFC 初始化失败\n");
            return false;
        }
        return true;
    }
};

