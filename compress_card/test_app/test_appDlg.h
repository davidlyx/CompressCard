
// test_appDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// Ctest_appDlg 对话框
class Ctest_appDlg : public CDialogEx
{
// 构造
public:
	Ctest_appDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TEST_APP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	HANDLE hHandle;
public:
	afx_msg void OnBnClickedButtonOpen();
	CButton cbtOpen;
};
