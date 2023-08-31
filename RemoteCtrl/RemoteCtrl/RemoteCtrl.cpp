// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>


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


void Dump(BYTE* pData, size_t nSize) {

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


int MakeDriverInfo() {//磁盘分区 1->A 2->B 3->C

    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) { //可以切换该分区
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;
        }
    }

    CPacket pack(1, (BYTE*)result.c_str(), result.size()); //打包
    Dump((BYTE*)pack.Data(), pack.Size() );
    //CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <io.h>
#include <list>
typedef struct file_info{
    file_info() {
        IsInvalid = 0;
        IsDirectory = 0;
        HasNext = 1;
        memset(szFileName, 0, sizeof(szFileName));
    }
    char szFileName[256]; //文件名
    bool IsInvalid; //是否无效 0有效
    bool IsDirectory; //是否为目录 0不是 1是
    bool HasNext; //是否有后续 0 没有 1有
}FILEINFO, *PFILEINFO;


int MakeDirectoryInfo() {
    std::string strPath;
    std::list<FILEINFO> listFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false){
        OutputDebugString(_T("获取路径错误，解析错误"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.IsInvalid = true;
        finfo.IsDirectory = true;
        finfo.HasNext = false;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("无权限访问目录"));
        return -2;
    }

    // 遍历目录
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到文件"));
        return -3;
    }

    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);

    } while (!_findnext(hfind, &fdata));

    // 发送信息到控制端，可能文件很多，最好不断发送信息
    FILEINFO finfo;
    finfo.HasNext = false;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);

    return 0;
}


int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    //shell执行
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    // 回发消息
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);

    return 0;
}

//#pragma warning(disable:4996) //fopen sprintf strcpy strstr

int DownloadFile() {//将文件发送到客户端
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);

    long long dataSize = 0;
    // 二进制打开文件
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0) {
        CPacket pack(4, (BYTE*)dataSize, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    // 先发送文件大小
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        dataSize = _ftelli64(pFile);
        CPacket head(4, (BYTE*)dataSize, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);

        char buffer[1024]{};
        // 循环发送文件
        size_t rlen = 0;
        do {
            memset(buffer, 0, sizeof(buffer));
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    // 发送结束消息
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
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

            //CServerSocket* pserver = CServerSocket::getInstance();
            //int count = 0;
            //if (pserver->InitSocket() == false) {
            //    MessageBox(NULL, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //while (CServerSocket::getInstance != NULL) {
            //    if (pserver->AcceptClient() == false) {
            //        if (count >= 3) {
            //            MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，请重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO
            //}
            
            int nCmd = 1;
            switch (nCmd)
            {

            case 1: //查看分区
                MakeDriverInfo();
                break;
            case 2: // 查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3: //打开文件
                RunFile();
                break;
            case 4: // 下载文件
                DownloadFile();
                break;
            default:
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
