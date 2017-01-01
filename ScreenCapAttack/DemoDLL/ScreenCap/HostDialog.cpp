// HostDialog.cpp : implementation file
//

#include "stdafx.h"
#include "ScreenCap.h"
#include "HostDialog.h"
#include "afxdialogex.h"


// HostDialog dialog

IMPLEMENT_DYNAMIC(HostDialog, CDialogEx)

HostDialog::HostDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{
	nID = IDD_DIALOG1;
	parentWnd = pParent;
}

HostDialog::~HostDialog()
{
}

void HostDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(HostDialog, CDialogEx)
END_MESSAGE_MAP()

// CHostDlg message handlers
BOOL HostDialog::Create()
{
	CDialogEx::Create(nID, parentWnd);
	return TRUE;
}


BOOL HostDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Get screen resolution
	RECT rect;
	HWND hDesktop = ::GetDesktopWindow();
	::GetWindowRect(hDesktop, &rect);

	// Set window position
	SetWindowPos(NULL, 0, 0, rect.right, rect.bottom, SWP_HIDEWINDOW);

	// Set window layered attribute
	SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE, GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(0, 255, LWA_ALPHA);

	return TRUE;
}


// HostDialog message handlers
