
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

float TemperatureRead()
{
	unsigned char scratch, temphigh, templow = 0;
	
	if (!onewire_resetPulse(lw)) return -404;
	onewire_writeByte(lw, 0xCC); // skip rom
	onewire_writeByte(lw, 0x44); // convert T command
	delay(750); // wait for conversion

	if (!onewire_resetPulse(lw)) return -404;
	onewire_writeByte(lw, 0xCC); // skip rom
	onewire_writeByte(lw, 0xBE); // read scratchpad

	for (int i = 0; i<9; i++) //read 9 bytes from SCRATCHPAD
	{
		scratch = onewire_readByte(lw);

		switch (i) 
		{
		case 0:
			templow = scratch;
		case 1:
			temphigh = scratch;
		}
	}
	
	float temperature = ((temphigh & 0x07) << 4);
	
	if (temphigh & (0x01 << 11))
		temperature = -temperature;
	
	for (int i = 0; i < 8; i++)
	{
		if(templow & (1 << i))
			temperature += powf(2,(i - 4));	
	}	
	
	return temperature;
}

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
		float temperature = TemperatureRead();
				
		if (!val.error && !littleWire_error())
		{
			TCHAR buffer[255];
			_stprintf_s(buffer, L"humidity: %f, temp %f\n sensor: %f", (float)val.humid / 10.0, (float)val.temp / 10.0, temperature);
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

