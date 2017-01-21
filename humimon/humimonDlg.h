
// humimonDlg.h : header file
//

#pragma once
#define ID_TIMER1 100

// ChumimonDlg dialog
class ChumimonDlg : public CDialogEx
{
// Construction
public:
	ChumimonDlg(CWnd* pParent = NULL);	// standard constructor
	afx_msg void OnTimer(UINT);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HUMIMON_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
