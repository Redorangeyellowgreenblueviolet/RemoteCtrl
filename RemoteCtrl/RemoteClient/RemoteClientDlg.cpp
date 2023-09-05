
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_serv_addr(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_serv_addr);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	//从控件得到IP Port
	UpdateData(true);

	//控件通知处理程序代码
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_serv_addr, _ttoi(m_nPort));
	if (ret == false) {
		AfxMessageBox(_T("网络连接失败!"));
		return -1;
	}


	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("ret:%d\r\n", ret);
	int ncmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", ncmd);

	if (bAutoClose) {
		pClient->CloseSocket();
	}
	return nCmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	//Question 消息注册
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	//虚拟机 10.0.2.15
	m_serv_addr = 0xC0A800C0;
	//m_serv_addr = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(false);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);

	m_isFull = FALSE;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// 在此添加控件通知处理程序代码
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();

	OutputDebugStringA(drivers.c_str());
	TRACE("size %d\r\n", drivers.size());
	for (size_t i = 0; i<drivers.size(); i++) {//处理磁盘分区信息
		if (drivers[i] == ',') { //Question
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}

}



CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + "\\" + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while(hTree!=NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	//Question 删除目录
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub)
			m_Tree.DeleteItem(hSub);

	} while (hSub != NULL);
}


void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{//下载文件

	// 如何得到参数，在List中选中
	int nListSelected = m_List.GetSelectionMark();
	//得到文件名
	CString strFile = m_List.GetItemText(nListSelected, 0);
	//文件对话框 选择保存的位置
	CFileDialog dlg(FALSE, NULL, strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地创建文件失败."));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		// 得到目录路径 
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("strFile:%s\r\n", LPCSTR(strFile));
		// 发送命令 不关闭连接
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, FALSE, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			// thiz窗口接收消息
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			
			if (ret < 0) {
				AfxMessageBox("执行下载命令失败.");
				TRACE("执行下载命令失败 ret=%d\r\n", ret);
				break;
			}
			// 处理服务端返回的消息

			// 先接收长度
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0) {
				AfxMessageBox("文件长度为0或无法读取文件");
				break;
			}
			//再写文件
			long long nCount = 0;
			while (nCount < nLength) {
				ret = pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输失败");
					TRACE("传输失败 ret=%d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}
		} while (FALSE);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成"),_T("完成"));
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
		Sleep(1);
	} while (pClient == NULL);

	while (m_isWatchClosed == FALSE) {//TODO 存在线程结束导致的问题
		if (m_isFull == FALSE) { //更新数据到缓存
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			TRACE("ret:%d\r\n", ret);
			if (ret == 6) {
				// 存入CImage
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL) {
					TRACE(_T("内存不足\r\n"));
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK) {
					ULONG ulength = 0;
					pStream->Write(pData, pClient->GetPacket().strData.size(), &ulength);
					LARGE_INTEGER bg{ 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);

					if ((HBITMAP)m_image != NULL)
						m_image.Destroy();

					m_image.Load(pStream);
					m_isFull = TRUE;
				}
			}
			else {//网络突然断开 实际休眠时间有CPU确定
				Sleep(1);
			}

		}
		else { //缓存有数据
			Sleep(1);
		}
	}

}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	//发送命令
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();

	int count = 0;
	while (pInfo->HasNext) {
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory){
			m_List.InsertItem(0, pInfo->szFileName);
		}
		count++;
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)
			break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	TRACE("Client count %d\r\n", count);

	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	//获取鼠标位置
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse); //Question
	// 获取所选树节点
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) {//无子目录
		return;
	}

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	//获取路径
	CString strPath = GetPath(hTreeSelected);
	//发送命令
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	
	int count = 0;
	while (pInfo->HasNext) {
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {//目录
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0)break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else { //文件
			m_List.InsertItem(0, pInfo->szFileName);
		}
		
		count++;

		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	TRACE("Client count %d\r\n", count);

	pClient->CloseSocket();
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	//获取当前鼠标位置
	GetCursorPos(&ptMouse);
	//将屏幕坐标->当前控件中的位置
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0)return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPopup = menu.GetSubMenu(0);
	if (pPopup != NULL) {
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}


void CRemoteClientDlg::OnDownloadFile()
{

	// 添加线程 防止大文件卡死
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	Sleep(50);
	// 大文件传输 避免卡死，通过线程  用户知道进度
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}


void CRemoteClientDlg::OnDeleteFile()
{
	// 删除文件
	HTREEITEM hSelectedNode = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelectedNode);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, TRUE, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败");
	}
	LoadFileCurrent(); //TODO 文件显示有遗漏
}


void CRemoteClientDlg::OnRunFile()
{
	// 打开文件
	HTREEITEM hSelectedNode = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelectedNode);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, TRUE, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败");
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{//自定义消息响应函数
	int ret = 0;
	int nCmd = wParam >> 1;
	switch (nCmd)
	{
	case 4: {
		CString strFile = (LPCSTR)lParam;
		// 31位表示nCmd 1位表示autoClose
		ret = SendCommandPacket(nCmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	}
		break;
	case 5:{
		ret = SendCommandPacket(nCmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
	}
	break;
	case 6:
	case 7:
	case 8:{
		ret = SendCommandPacket(nCmd, wParam & 1);
	}
		  break;
	default:
		ret = -1;
		break;
	}

	
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	m_isWatchClosed = FALSE;
	// 按下监控按钮
	CWatchDialog dlg(this);
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
	m_isWatchClosed = TRUE;
	WaitForSingleObject(hThread, 500);
}


