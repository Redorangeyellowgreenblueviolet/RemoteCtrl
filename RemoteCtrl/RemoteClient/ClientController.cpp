#include "pch.h"
#include "ClientController.h"


//定义静态变量
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
    if (m_instance == NULL) {
        m_instance = new CClientController();
        struct {
            UINT nMsg;
            MSGFUNC func;
        }MsgFuncs[] = 
        {

            {WM_SHOW_STATUS, &CClientController::OnShowStatus},
            {WM_SHOW_WATCH, &CClientController::OnShowWatcher},
            {(UINT)-1,NULL}
        };

        for (int i = 0; MsgFuncs[i].func != NULL; i++) {
            m_mapFunc.insert(std::pair<UINT, MSGFUNC>(
                MsgFuncs[i].nMsg, MsgFuncs[i].func));
        }

    }
    return m_instance;
}

int CClientController::InitController()
{
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
    m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
    return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
    pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

//LRESULT CClientController::_SendMessage(MSG msg)
//{
//    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//    if (hEvent == NULL) return -2;
//    MSGINFO info(msg);
//    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
//    WaitForSingleObject(hEvent, INFINITE);
//    CloseHandle(hEvent);
//    return info.result;
//}

bool CClientController::SendCommandPacket(
    HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
    TRACE("ncmd:%d, datalen:%d, start:%lld\r\n", nCmd, nLength, GetTickCount64());
    CClientSocket* pClient = CClientSocket::getInstance();   
    CPacket pack(nCmd, pData, nLength);
    bool ret = pClient->SendPacket(hWnd, pack, bAutoClose, wParam);
    return ret;
}

int CClientController::DownFile(CString strPath)
{
    CFileDialog dlg(FALSE, NULL, strPath,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
    if (dlg.DoModal() == IDOK) {
        // 添加线程 防止大文件卡死
        m_strRemote = strPath;
        m_strLocal = dlg.GetPathName();

        FILE* pFile = fopen(m_strLocal, "wb+");
        if (pFile == NULL) {
            AfxMessageBox(_T("本地创建文件失败."));
            return -1;
        }

        int ret = SendCommandPacket(
            m_remoteDlg, 4, FALSE, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
        // 大文件传输 避免卡死，通过线程  用户知道进度
        m_remoteDlg.BeginWaitCursor();
        m_statusDlg.m_info.SetWindowText(_T("命令正在执行"));
        m_statusDlg.ShowWindow(SW_SHOW);
        m_statusDlg.CenterWindow(&m_remoteDlg);
        m_statusDlg.SetActiveWindow();

    }

    return 0;
}

void CClientController::DownloadEnd()
{
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
    //m_remoteDlg.LoadFileInfo();
}


void CClientController::threadWatchScreen()
{
    Sleep(50);
    ULONGLONG nTick = GetTickCount64();
    while (m_isWatchClosed == FALSE) {
        ULONGLONG curTick = GetTickCount64();
        if (curTick - nTick < 100) {
            Sleep(100 - DWORD(curTick - nTick));
        }
        nTick = GetTickCount64();
        bool ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, TRUE, NULL, 0);
        TRACE("ret:%d, tick:%lld\r\n", ret,GetTickCount64());
        if (ret == true) {
            TRACE(_T("成功发送请求命令屏幕\r\n"));
        }
    }
}

void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
}

void CClientController::StartWatchScreen()
{
    m_isWatchClosed = FALSE;
    // 按下监控按钮
    //m_watchDlg.SetParent(&m_remoteDlg);
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
    m_watchDlg.DoModal();
    m_isWatchClosed = TRUE;
    WaitForSingleObject(m_hThreadWatch, 500);
}


unsigned __stdcall CClientController::threadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}

void CClientController::threadFunc()
{
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_SEND_MESSAGE) {
            MSGINFO* pmsg = (MSGINFO*)msg.wParam;
            HANDLE hEvent = (HANDLE)msg.lParam;
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);
            if (it != m_mapFunc.end()) {
                pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
            }
            else {
                pmsg->result = -1;
            }
            SetEvent(hEvent);
        }
        else {
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                (this->*it->second)(msg.message, msg.wParam, msg.lParam);
            }
        }

    }
}



LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();
}


