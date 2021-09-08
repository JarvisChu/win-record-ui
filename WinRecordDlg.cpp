
// WinRecordDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WinRecord.h"
#include "WinRecordDlg.h"
#include "afxdialogex.h"
#include "record.h"
#include <shellapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "shell32")

// CWinRecordDlg dialog

CWinRecordDlg::CWinRecordDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_WINRECORD_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinRecordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWinRecordDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START_RECORD, &CWinRecordDlg::OnBnClickedBtnStartRecord)
	ON_BN_CLICKED(IDC_BTN_STOP_RECORD, &CWinRecordDlg::OnBnClickedBtnStopRecord)
END_MESSAGE_MAP()


// CWinRecordDlg message handlers

BOOL CWinRecordDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWinRecordDlg::OnPaint()
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
HCURSOR CWinRecordDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWinRecordDlg::OnBnClickedBtnStartRecord()
{
	LOGGER;
	Record::GetInstance().Start();
	auto btnStart = this->GetDlgItem(IDC_BTN_START_RECORD);
	auto btnStop = this->GetDlgItem(IDC_BTN_STOP_RECORD);
	btnStart->EnableWindow(false);
	btnStop->EnableWindow(true);
}


void CWinRecordDlg::OnBnClickedBtnStopRecord()
{
	LOGGER;
	Record::GetInstance().Stop();
	auto btnStart = this->GetDlgItem(IDC_BTN_START_RECORD);
	auto btnStop = this->GetDlgItem(IDC_BTN_STOP_RECORD);
	btnStart->EnableWindow(true);
	btnStop->EnableWindow(false);

	// 打开当前目录
	ShellExecuteA(NULL, "open", ".", NULL, NULL, SW_SHOW);
}
