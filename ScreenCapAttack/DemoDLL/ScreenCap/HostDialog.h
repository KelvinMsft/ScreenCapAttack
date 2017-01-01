#pragma once


// HostDialog dialog

class HostDialog : public CDialogEx
{
	DECLARE_DYNAMIC(HostDialog)

public:
	HostDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~HostDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

	DECLARE_MESSAGE_MAP()


public:
	virtual BOOL Create();
	virtual BOOL OnInitDialog();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	UINT nID;
	CWnd* parentWnd;
};
