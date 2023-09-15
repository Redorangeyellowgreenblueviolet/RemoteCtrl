// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "WatchDialog.h"
#include "afxdialogex.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_WATCH, pParent)
{
	m_isFull = FALSE;
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteSreenPoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	
	if (!isScreen) {
		ClientToScreen(&point); //转换为全屏坐标
	}
	m_picture.ScreenToClient(&point); //转换为picture控件坐标

	// 本地坐标->远程坐标
	m_picture.GetWindowRect(clientRect);
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化

	//SetTimer(0, 40, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0) {
	//	if (isFull()) {
	//		
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);
	//		m_nObjWidth = m_image.GetWidth();
	//		m_nObjHeight = m_image.GetHeight();

	//		m_image.StretchBlt(
	//			m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL);
	//		m_image.Destroy();
	//		SetImageStatus(FALSE);
	//	}
	//}
	//CDialog::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2) {
		//TODO:错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			switch (pPacket->sCmd)
			{
			case 6:
			{
				if (m_isFull) {
					CRect rect;
					m_picture.GetWindowRect(rect);
					m_nObjWidth = m_image.GetWidth();
					m_nObjHeight = m_image.GetHeight();

					m_image.StretchBlt(
						m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
					m_picture.InvalidateRect(NULL);
					m_image.Destroy();
					SetImageStatus(FALSE);
				}
				break;
			}
			case 5:
			case 7:
			case 8:
			default:
				break;
			}
		}
	}

	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{// 左键双击
	//坐标转换
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 1;
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	//左键按下
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		TRACE("x=%d y=%d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 2;
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// 左键弹起
	
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 3;

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 1;

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 2;

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 3;

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// 鼠标移动
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		CPoint remote = UserPoint2RemoteSreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 4; //TODO 设置4 服务端设置无反应

		// TODO 网络通信与对话框耦合 每次需要获得父类创建套接字
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnMouseMove(nFlags, point);
}



void CWatchDialog::OnStnClickedWatch()
{//TODO 未收到点击事件
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		//坐标转换
		CPoint point;
		GetCursorPos(&point);
		CPoint remote = UserPoint2RemoteSreenPoint(point, TRUE);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 0; //单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, TRUE, (BYTE*)&event, sizeof(event));

	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}


void CWatchDialog::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码

	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);

}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码

	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);

}
