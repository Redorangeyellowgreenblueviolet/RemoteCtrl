#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "Resource.h"
#include <map>

#define WM_SEND_PACK (WM_USER+1) //���Ͱ�����
#define WM_SEND_DATA (WM_USER+2) //��������
#define WM_SHOW_STATUS (WM_USER+3) //չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4) //Զ�̼��
#define WM_SEND_MESSAGE (WM_USER + 0x1000) //�Զ�����Ϣ����

class CClientController
{
public:
	// ��ȡȫ��Ψһ����
	static CClientController* getInstance();
	// ��ʼ������
	int InitController();
	// ����
	int Invoke(CWnd*& pMainWnd);
	// ������Ϣ
	LRESULT _SendMessage(MSG msg);

protected:
	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}

	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}

	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();

	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}

	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo{
		MsgInfo(MSG m){
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo() {
			result = 0;
			memset(&msg, 0, sizeof(MSG));
		}
		MsgInfo(const MsgInfo &m) {
			result = m.result;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m, sizeof(MSG));
			}
			return *this;
		}

		MSG msg;
		LRESULT result;
	}MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, 
		WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;

	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	unsigned int m_nThreadID;

	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};

};
