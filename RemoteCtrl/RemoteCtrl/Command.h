#pragma once
#include <map>
#include <list>
#include <atlimage.h>
#include <direct.h>
#include <io.h>

#include "Tool.h"
#include "Packet.h"
#include "LockInfoDialog.h"
#include "Resource.h"
//#pragma warning(disable:4996) //fopen sprintf strcpy strstr

class CCommand
{
public:
	CCommand();
	~CCommand() = default;
	int ExecuteCommond(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);

    static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) {
        CCommand* thiz = (CCommand*)arg;
        if (status > 0) {
            int ret = thiz->ExecuteCommond(status, lstPacket, inPacket);
            if (ret != 0) {
                TRACE(_T("��������ʧ�� %d ret=%d\r\n"), status, ret);
            }
        }
        else {
            MessageBox(NULL, _T("�޷����������û���������"), _T("�����û�ʧ��"), MB_OK | MB_ICONERROR);
        }
    }


protected:
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); //��Ա����ָ��
    std::map<int, CMDFUNC> m_mapFunction; //�����-���ܺ�����ӳ��

    unsigned threadid;
    CLockInfoDialog dlg;

protected:

    int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {//���̷��� 1->A 2->B 3->C

        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0) { //�����л��÷���
                if (result.size() > 0)
                    result += ',';
                result += 'A' + i - 1;
            }
        }
        result += ',';
        lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
        return 0;
    }

    int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath = inPacket.strData;
        TRACE("%s\r\n", strPath.c_str());
        if (_chdir(strPath.c_str()) != 0) {
            FILEINFO finfo;
            finfo.HasNext = false;
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("��Ȩ�޷���Ŀ¼"));
            return -2;
        }

        // ����Ŀ¼
        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            OutputDebugString(_T("û���ҵ��ļ�"));
            FILEINFO finfo;
            finfo.HasNext = false;
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            return -3;
        }

        int count = 0;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            finfo.HasNext = true;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            count++;
        } while (!_findnext(hfind, &fdata));
        TRACE("count: %d\r\n", count);

        // ������Ϣ�����ƶˣ������ļ��ܶ࣬��ò��Ϸ�����Ϣ
        FILEINFO finfo;
        finfo.HasNext = false;
        lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        return 0;
    }


    int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath = inPacket.strData;
        //shellִ��
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        // �������
        lstPacket.push_back(CPacket(3, NULL, 0));
        return 0;
    }

    int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {//���ļ����͵��ͻ���
        std::string strPath = inPacket.strData;

        long long dataSize = 0;
        // �����ƴ��ļ�
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0) {
            lstPacket.push_back(CPacket(4, (BYTE*)&dataSize, 8));
            return -1;
        }
        // �ȷ����ļ���С
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END);
            dataSize = _ftelli64(pFile);
            lstPacket.push_back(CPacket(4, (BYTE*)&dataSize, 8));
            fseek(pFile, 0, SEEK_SET);

            char buffer[1024]{};
            // ѭ�������ļ�
            size_t rlen = 0;
            do {
                memset(buffer, 0, sizeof(buffer));
                rlen = fread(buffer, 1, 1024, pFile);
                lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
            } while (rlen >= 1024);
            fclose(pFile);
        }
        else {
            lstPacket.push_back(CPacket(4, (BYTE*)&dataSize, 0));
        }
        return 0;
    }


    int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {//ɾ���ļ�
        std::string strPath = inPacket.strData;
        TCHAR sPath[MAX_PATH] = _T("");
        MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFile(sPath);

        //DeleteFileA(strPath.c_str());
        lstPacket.push_back(CPacket(9, NULL, 0));
        return 0;
    }


    int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {

        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

        DWORD nFlags = 0; //��ʶ
        switch (mouse.nButton)
        {
        case 0: // ���
            nFlags = 1;
            break;
        case 1: // �Ҽ�
            nFlags = 2;
            break;
        case 2: //�м�
            nFlags = 4;
            break;
        case 4: //�ް���
            nFlags = 8;
            break;
        }

        if (nFlags != 8)
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);

        switch (mouse.nAction)
        {
        case 0: // ����
            nFlags |= 0x10;
            break;
        case 1: // ˫��
            nFlags |= 0x20;
            break;
        case 2: // ����
            nFlags |= 0x40;
            break;
        case 3: // �ſ�
            nFlags |= 0x80;
            break;
        default:
            break;
        }

        switch (nFlags)
        {
        case 0x21: //���˫��
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());

        case 0x11: //�������
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x41: //�������
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81: //����ɿ�
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22: //�Ҽ�˫��
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12: //�Ҽ�����
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42: //�Ҽ�����
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82: //�Ҽ��ɿ�
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24: //�м�˫��
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14: //�м�����
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44: //�м�����
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84: //�м��ɿ�
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08: //��������ƶ�
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        // ���ͽ�����Ϣ
        lstPacket.push_back(CPacket(5, NULL, 0));

        return 0;
    }

    int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        // GDI ȫ���豸�ӿ�
        CImage screen; //���ڴ洢

        HDC hScreen = ::GetDC(NULL); //��Ļ
        // Ques
        // λ�� ��ɫ�� (255,255,255) RGB888 24bit
        // ARGB8888 32bit RGB444 RGB565
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel);
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hScreen);

        // ���ڴ����ķ�ʽ����
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
            lstPacket.push_back(CPacket(6, pData, nSize));
            GlobalUnlock(hmem);
        }

        //DWORD64 tick = GetTickCount64(); //��õ�ǰtick 
        //screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG);
        //TRACE(_T("png %d\r\n"), GetTickCount64() - tick);
        pStream->Release();
        GlobalFree(hmem);
        screen.ReleaseDC();
        return 0;
    }


    static unsigned __stdcall threadLockDlg(void* arg) {//�����߳�
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }

    void threadLockDlgMain() {
        //�����ö�
        dlg.Create(IDD_DIALOG_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);

        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 100;

        //ȫ����ʾ
        dlg.MoveWindow(rect);
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWidth = rtText.Width();
            int x = (rect.right - nWidth) / 2;
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, nWidth, nHeight);
        }

        //�ö�
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        //����Windows������
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

        //�������
        ShowCursor(false);
        rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;
        dlg.GetWindowRect(rect);
        ClipCursor(rect);

        //��Ϣѭ��
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg: %08X wparam:%08X lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x41) //���� a �˳�
                    break;
            }
        }
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
        ClipCursor(NULL);
        ShowCursor(true);
        dlg.DestroyWindow();
    }

    int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {//����
    /*
    * �޸����� ֻ���÷�ģ̬�Ի���
    * ������� ����
    */
        if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
            //_beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
        }

        lstPacket.push_back(CPacket(7, NULL, 0));
        return 0;
    }

    int UnlocakMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        //dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0x01E0001);
        lstPacket.push_back(CPacket(8, NULL, 0));
        return 0;
    }
    int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        lstPacket.push_back(CPacket(1981, NULL, 0));
        return 0;
    }


};
