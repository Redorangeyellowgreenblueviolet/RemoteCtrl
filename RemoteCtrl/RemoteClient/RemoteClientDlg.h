
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

// 发送数据包信息
#define WM_SEND_PACKET (WM_USER + 1)

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	bool isFull() const {
		return m_isFull;
	}
	void SetImageStatus(bool isFull = FALSE) {
		m_isFull = isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
private:
	CImage m_image; //缓存
	bool m_isFull; //true 表示有数据
private:
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
	int SendCommandPacket(int nCmd, bool bAutoClose = true,BYTE* pData = NULL, size_t nLength = 0);

	//下载文件线程函数
	static void threadEntryForDownFile(void* arg);
	void threadDownFile();

	//监控数据线程 静态函数实现框架 成员函数具体实现
	static void threadEntryForWatchData(void* arg);//静态函数不能使用this
	void threadWatchData(); //成员函数this

	void LoadFileCurrent();
	void LoadFileInfo();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_serv_addr;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	// Question 消息函数声明
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
};
