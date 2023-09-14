#include "pch.h"
#include "ClientController.h"


//���徲̬����
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

LRESULT CClientController::_SendMessage(MSG msg)
{
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL) return -2;
    MSGINFO info(msg);
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
    WaitForSingleObject(hEvent, INFINITE);
    CloseHandle(hEvent);
    return info.result;
}

int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, std::list<CPacket>* plstPacks)
{
    CClientSocket* pClient = CClientSocket::getInstance();
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    //TODO: ��ֱ�ӷ��͸�Ϊ�ŵ�����
    std::list<CPacket> lstPacks; //Ӧ������
    if (plstPacks == NULL) {
        plstPacks = &lstPacks;
    }

    TRACE("ncmd:%d, datalen:%d\r\n", nCmd, nLength);
    CPacket pack(nCmd, pData, nLength, hEvent);
    TRACE("head %x\r\n", pack.sHead);
    pClient->SendPacket(pack, *plstPacks, bAutoClose);
    if(hEvent!=NULL)
        CloseHandle(hEvent);

    if (plstPacks->size() > 0) {
        return plstPacks->front().sCmd;
    }
    return -1;
}

int CClientController::DownFile(CString strPath)
{
    CFileDialog dlg(FALSE, NULL, strPath,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
    if (dlg.DoModal() == IDOK) {
        // ����߳� ��ֹ���ļ�����
        m_strRemote = strPath;
        m_strLocal = dlg.GetPathName();
        m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadFileEntry, 0, this);
        if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
            return -1;
        }
        // ���ļ����� ���⿨����ͨ���߳�  �û�֪������
        m_remoteDlg.BeginWaitCursor();
        m_statusDlg.m_info.SetWindowText(_T("��������ִ��"));
        m_statusDlg.ShowWindow(SW_SHOW);
        m_statusDlg.CenterWindow(&m_remoteDlg);
        m_statusDlg.SetActiveWindow();

    }

    return 0;
}

void CClientController::StartWatchScreen()
{
    m_isWatchClosed = FALSE;
    // ���¼�ذ�ť
    //m_watchDlg.SetParent(&m_remoteDlg);
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
    m_watchDlg.DoModal();
    m_isWatchClosed = TRUE;
    WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadDownloadFile()
{
    FILE* pFile = fopen(m_strLocal, "wb+");
    if (pFile == NULL) {
        AfxMessageBox(_T("���ش����ļ�ʧ��."));
        m_statusDlg.ShowWindow(SW_HIDE);
        m_remoteDlg.EndWaitCursor();
        return;
    }
    // �������� ���ر�����
	CClientSocket* pClient = CClientSocket::getInstance();
    do {
        int ret = SendCommandPacket(4, FALSE, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
        if (ret < 0) {
            AfxMessageBox("ִ����������ʧ��.");
            TRACE("ִ����������ʧ�� ret=%d\r\n", ret);
            break;
        }
        // �������˷��ص���Ϣ

        // �Ƚ��ճ���
        long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
        if (nLength == 0) {
            AfxMessageBox("�ļ�����Ϊ0���޷���ȡ�ļ�");
            break;
        }
        //��д�ļ�
        long long nCount = 0;
        while (nCount < nLength) {
            ret = pClient->DealCommand();
            if (ret < 0) {
                AfxMessageBox("����ʧ��");
                TRACE("����ʧ�� ret=%d\r\n", ret);
                break;
            }
            fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
            nCount += pClient->GetPacket().strData.size();
        }

    } while (FALSE);
    fclose(pFile);
    pClient->CloseSocket();
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("�������"), _T("���"));

}

void CClientController::threadDownloadFileEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadDownloadFile();
    _endthread();
}

void CClientController::threadWatchScreen()
{
    Sleep(50);
    while (m_isWatchClosed == FALSE) {//TODO �����߳̽������µ�����
        if (m_watchDlg.isFull() == FALSE) { //�������ݵ�����
            std::list<CPacket> lstPacks;
            int ret = SendCommandPacket(6, TRUE, NULL, 0, &lstPacks);
            TRACE("ret:%d\r\n", ret);
            if (ret == 6) {
                if (CTool::Bytes2Image(m_watchDlg.GetImage(), lstPacks.front().strData) == 0) {
                    m_watchDlg.SetImageStatus(TRUE);
                    TRACE(_T("�ɹ�����ͼƬ\r\n"));
                }
                else
                {
                    TRACE(_T("��ȡͼ��ʧ��\r\n"));
                }
            }
        }
        //����ͻȻ�Ͽ� ʵ������ʱ����CPUȷ��
        //����������
        Sleep(1);
    }
}

void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
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


