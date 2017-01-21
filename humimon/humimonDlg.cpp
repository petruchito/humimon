
// humimonDlg.cpp : implementation file
//

#include "stdafx.h"
#include "humimon.h"
#include "humimonDlg.h"
#include "afxdialogex.h"
#include "littleWire.h"
#include "littleWire_util.h"

unsigned char version;
dht_reading val;
littleWire *lw = NULL;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ChumimonDlg dialog



ChumimonDlg::ChumimonDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_HUMIMON_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ChumimonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ChumimonDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
END_MESSAGE_MAP()





 void ChumimonDlg::OnTimer(UINT uTime)
{
	 if (lw == NULL)
	 {
		 lw = littleWire_connect();
	 }

	if (lw == NULL) 
	{
		SetDlgItemText(IDC_STATIC, L"Little Wire could not be found!\n");
	}
	else 
	{
		val = dht_read(lw, DHT22);
				
		if (!val.error && !littleWire_error())
		{
			TCHAR buffer[255];
			_stprintf_s(buffer, L"humidity: %f, temp %f\n", (float)val.humid / 10.0, (float)val.temp / 10.0);
			SetDlgItemText(IDC_STATIC, buffer);
		}
		else 
		{
			//SetDlgItemText(IDC_STATIC, L"Error Reading sensor!");
			SetDlgItemText(IDC_STATIC, CA2W(littleWire_errorName()));
			
			lw = NULL;
		}
	}
}

// ChumimonDlg message handlers

BOOL ChumimonDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	
	OnTimer(0);
	SetTimer(ID_TIMER1, 1000, NULL);



	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void ChumimonDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR ChumimonDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

