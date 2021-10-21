
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
	ON_BN_CLICKED(IDC_BTN_BROWSER_FILE, &CWinRecordDlg::OnBnClickedBtnBrowserFile)
	ON_BN_CLICKED(IDC_BTN_CONVERT2WAVE, &CWinRecordDlg::OnBnClickedBtnConvert2wave)
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
	CComboBox* pComboBoxSampleRate = (CComboBox*)GetDlgItem(IDC_COMBO_SAMPLE_RATE);
	pComboBoxSampleRate->InsertString(0, L"8000");
	pComboBoxSampleRate->InsertString(1, L"16000");
	pComboBoxSampleRate->InsertString(2, L"24000");
	pComboBoxSampleRate->InsertString(3, L"32000");
	pComboBoxSampleRate->InsertString(4, L"44100");
	pComboBoxSampleRate->InsertString(5, L"48000");
	pComboBoxSampleRate->SetCurSel(0);

	CComboBox* pComboBoxSampleBits = (CComboBox*)GetDlgItem(IDC_COMBO_SAMPLE_BITS);
	pComboBoxSampleBits->InsertString(0, L"8");
	pComboBoxSampleBits->InsertString(1, L"16");
	pComboBoxSampleBits->InsertString(2, L"24");
	pComboBoxSampleBits->InsertString(3, L"32");
	pComboBoxSampleBits->SetCurSel(1);

	CComboBox* pComboBoxChannels = (CComboBox*)GetDlgItem(IDC_COMBO_CHANNELS);
	pComboBoxChannels->InsertString(0, L"1");
	pComboBoxChannels->InsertString(1, L"2");
	pComboBoxChannels->InsertString(2, L"4");
	pComboBoxChannels->InsertString(3, L"8");
	pComboBoxChannels->SetCurSel(0);

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


void CWinRecordDlg::OnBnClickedBtnBrowserFile()
{
	LOGGER;
	const TCHAR szFilter[] = _T("PCM Files (*.pcm)|*.pcm|All Files (*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("pcm"), NULL, OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT, szFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString sFilePath;
		m_vecPCMFiles.clear();
		POSITION pos(dlg.GetStartPosition());
		while (pos)
		{
			CString filename = dlg.GetNextPathName(pos);
			sFilePath += filename;
			sFilePath += CString(";");
			m_vecPCMFiles.push_back(filename);
		}

		//CString sFilePath = dlg.GetPathName();
		SetDlgItemText(IDC_EDIT_PCM_FILE, sFilePath);
	}
}


void CWinRecordDlg::OnBnClickedBtnConvert2wave()
{
	LOGGER;
	if (m_vecPCMFiles.size() == 0 ) {
		AfxMessageBox(L"请先选择要转码的PCM文件！");
		return;
	}

	CString cstrSampleRate;
	CString cstrSampleBits;
	CString cstrChannels;
	GetDlgItemText(IDC_COMBO_SAMPLE_RATE, cstrSampleRate);
	GetDlgItemText(IDC_COMBO_SAMPLE_BITS, cstrSampleBits);
	GetDlgItemText(IDC_COMBO_CHANNELS, cstrChannels);

	int sampleRate = _wtoi(cstrSampleRate);
	int sampleBits = _wtoi(cstrSampleBits);
	int channels = _wtoi(cstrChannels);

	LINFO(L"sampleRate: %d, sampleBits: %d, channels: %d", sampleRate, sampleBits, channels);

	CString tip;
	

	int failedCnt = 0;
	for (int i = 0; i < m_vecPCMFiles.size(); i++){
		CString pcmFile = m_vecPCMFiles[i];
		CString wavFile = pcmFile + L".wav";

		LINFO(L"Converting %s to %s", pcmFile, wavFile);
		tip.Format(L"转码中 %d/%d ...", i + 1, m_vecPCMFiles.size());
		SetDlgItemText(IDC_BTN_CONVERT2WAVE, tip);

		FILE* in;
		errno_t err = _wfopen_s(&in, pcmFile, L"rb");
		if (err != 0) {
			LERROR(L"open pcm file failed, %s", pcmFile);
			failedCnt++;
			continue;
		}

		CWaveFile out;
		out.Open(Wide2UTF8(wavFile.GetString()), sampleRate, sampleBits, channels);

		while (true) {
			char buf[2048] = { 0 };
			int nBytesRead = fread_s(buf, 2048, 1, 2048, in);
			if (nBytesRead == 0) {
				break;
			}

			std::vector<BYTE> data;
			data.insert(data.end(), buf, buf + nBytesRead);
			out.Write(data, data.size());
		}

		fclose(in);
		out.Close();
	}

	SetDlgItemText(IDC_BTN_CONVERT2WAVE, L"转成WAV格式");
	tip.Format(L"转换完成：成功 %d 个，失败 %d 个\n\nWAV音频存储在PCM同目录下", m_vecPCMFiles.size() - failedCnt, failedCnt);

	AfxMessageBox(tip);
}