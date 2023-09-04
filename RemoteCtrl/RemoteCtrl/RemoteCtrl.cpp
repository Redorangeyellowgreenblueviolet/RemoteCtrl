﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <list>
#include <atlimage.h>

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
    result += ',';
    CPacket pack(1, (BYTE*)result.c_str(), result.size()); //打包
    Dump((BYTE*)pack.Data(), pack.Size() );
    CServerSocket::getInstance()->Send(pack);
    return 0;
}





int MakeDirectoryInfo() {
    std::string strPath;
    std::list<FILEINFO> listFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false){
        OutputDebugString(_T("获取路径错误，解析错误"));
        return -1;
    }
    TRACE("%s\r\n", strPath.c_str());
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.HasNext = false;
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
        FILEINFO finfo;
        finfo.HasNext = false;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }

    int count = 0;
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        finfo.HasNext = true;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);

        count++;

    } while (!_findnext(hfind, &fdata));
    TRACE("count: %d\r\n", count);

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
        CPacket pack(4, (BYTE*)&dataSize, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    // 先发送文件大小
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        dataSize = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&dataSize, 8);
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


int DeleteLocalFile() {//删除文件
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
    DeleteFile(sPath);

    //DeleteFileA(strPath.c_str());

    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret:%d\r\n", ret);
    return 0;
}


int MouseEvent() {

    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {

        DWORD nFlags = 0; //标识
        
        switch (mouse.nButton)
        {
        case 0: // 左键
            nFlags = 1;
            break;
        case 1: // 右键
            nFlags = 2;
            break;
        case 2: //中键
            nFlags = 4;
            break;
        case 4: //无按键
            nFlags = 8;
            break;
        }

        if(nFlags != 8 )
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);

        switch (mouse.nAction)
        {
        case 0: // 单击
            nFlags |= 0x10;
            break;
        case 1: // 双击
            nFlags |= 0x20;
            break;
        case 2: // 按下
            nFlags |= 0x40;
            break;
        case 3: // 放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }

        switch (nFlags)
        {
        case 0x21: //左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            
        case 0x11: //左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x41: //左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81: //左键松开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22: //右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12: //右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42: //右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82: //右键松开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24: //中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14: //中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44: //中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84: //中键松开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08: //单纯鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        // 发送结束信息
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);

    }
    else {
        OutputDebugString(_T("获取鼠标参数失败!"));
        return -1;
    }

    return 0;
}

int SendScreen() {
    // GDI 全局设备接口
    CImage screen; //用于存储

    HDC hScreen = ::GetDC(NULL); //屏幕
    // Ques
    // 位宽 真色彩 (255,255,255) RGB888 24bit
    // ARGB8888 32bit RGB444 RGB565
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, 1920, 1080, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);

    // 以内存流的方式发送
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hmem == NULL)return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hmem, true, &pStream);
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);
        LARGE_INTEGER bg{ 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hmem);
        SIZE_T nSize = GlobalSize(hmem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hmem);
    }

    //DWORD64 tick = GetTickCount64(); //获得当前tick 
    //screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG);
    //TRACE(_T("png %d\r\n"), GetTickCount64() - tick);
    pStream->Release();
    GlobalFree(hmem);
    screen.ReleaseDC();
    return 0;
}

#include "LockInfoDialog.h"
CLockInfoDialog dlg;
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg) {//锁机线程
    //窗口置顶
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);

    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 100;

    //全屏显示
    dlg.MoveWindow(rect);
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //隐藏Windows任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

    //限制鼠标
    ShowCursor(false);
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    dlg.GetWindowRect(rect);
    ClipCursor(rect);

    //消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg: %08X wparam:%08X lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x41) //按下 a 退出
                break;
        }
    }
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    ShowCursor(true);
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine() {//锁机
    /*
    * 无父窗口 只能用非模态对话框
    * 限制鼠标 窗口
    */
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
        //_beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
    }
 
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlocakMachine() {
    //dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0x01E0001);
    CPacket pack(8, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}
int TestConnect() {
    CPacket pack(1981, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    
    TRACE("Send ret:%d", ret);
    return 0;
}

int ExecuteCommond(int nCmd)
{
    int ret = 0;
    switch (nCmd)
    {
    case 1: //查看分区
        ret = MakeDriverInfo();
        break;
    case 2: // 查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3: //打开文件
        ret = RunFile();
        break;
    case 4: // 下载文件
        ret = DownloadFile();
        break;
    case 5: // 鼠标事件
        ret = MouseEvent();
        break;
    case 6: // 发送屏幕内容--发送屏幕截图
        ret = SendScreen();
        break;
    case 7: // 锁机
        ret = LockMachine();
        Sleep(30);
        break;
    case 8: // 解锁
        ret = UnlocakMachine();
        break;
    case 9: //删除文件
        ret = DeleteLocalFile();
        break;
    case 1981: //连接测试
        ret = TestConnect();
        break;
    }
    return ret;
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

            CServerSocket* pserver = CServerSocket::getInstance();
            int count = 0;
            if (pserver->InitSocket() == false) {
                MessageBox(NULL, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance != NULL) {
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，请重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                    count++;
                }
                /*
                * 短连接 适合小量数据 获取分区等功能
                * 长连接 效率高 适合大量数据频繁发送 传输文件
                */
                int ret = pserver->DealCommand();
                TRACE("DealCommand ret:%d\r\n", ret);
                if (ret > 0) {
                    ret = ExecuteCommond(ret);
                    if (ret != 0) {
                        TRACE(_T("处理命令失败 %d ret=%d\r\n"), pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("Commond has done\r\n");
                }
                
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
