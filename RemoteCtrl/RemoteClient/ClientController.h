#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "Resource.h"
#include "ClientSocket.h"
#include "Tool.h"
#include <map>


#define WM_SHOW_STATUS (WM_USER+3) //展示状态
#define WM_SHOW_WATCH (WM_USER+4) //远程监控
#define WM_SEND_MESSAGE (WM_USER + 0x1000) //自定义消息处理

class CClientController
{
public:
	// 获取全局唯一对象
	static CClientController* getInstance();
	// 初始化操作
	int InitController();
	// 启动
	int Invoke(CWnd*& pMainWnd);
	// 发送消息
	LRESULT _SendMessage(MSG msg);
	// 更新地址
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}


	/*
	* 1 查看磁盘分区
	* 2 查看指定目录下的文件
	* 3 打开文件
	* 4 下载文件
	* 9 删除文件
	* 5 鼠标操作
	* 6 发送屏幕内容
	* 7 锁机
	* 8 解锁
	* 1981 测试连接
	* 返回值 命令号 错误-1
	*/
	int SendCommandPacket(int nCmd, bool bAutoClose = TRUE, BYTE* pData = NULL, size_t nLength = 0, std::list<CPacket>* plstPacks = NULL);

	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	int DownFile(CString strPath);
	void StartWatchScreen();


protected:
	void threadDownloadFile();
	static void threadDownloadFileEntry(void* arg);

	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);

	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)
	{
		m_isWatchClosed = TRUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
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
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	BOOL m_isWatchClosed;
	CString m_strRemote; //下载文件的远程路径
	CString m_strLocal;	//下载文件的本地保存路径
	unsigned int m_nThreadID;

	static CClientController* m_instance;
	

	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

