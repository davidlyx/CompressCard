
// test_appDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// Ctest_appDlg �Ի���
class Ctest_appDlg : public CDialogEx
{
// ����
public:
	Ctest_appDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TEST_APP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
