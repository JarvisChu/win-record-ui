
// WinRecordDlg.h : header file
//

#pragma once
#include <vector>

// CWinRecordDlg dialog
class CWinRecordDlg : public CDialogEx
{
// Construction
public:
	CWinRecordDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WINRECORD_DIALOG };
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
public:
	afx_msg void OnBnClickedBtnStartRecord();
	afx_msg void OnBnClickedBtnStopRecord();
	afx_msg void OnBnClickedBtnBrowserFile();
	afx_msg void OnBnClickedBtnConvert2wave();
private:
	std::vector<CString> m_vecPCMFiles;
};
