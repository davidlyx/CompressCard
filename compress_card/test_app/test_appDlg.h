
// test_appDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include <WinIoCtl.h>


#define IOCTL_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x800 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_RESET CTL_CODE(FILE_DEVICE_UNKNOWN , 0x801 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_MODE CTL_CODE(FILE_DEVICE_UNKNOWN , 0x802 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANT CTL_CODE(FILE_DEVICE_UNKNOWN , 0x803 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_RESOLUTION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x804 , METHOD_BUFFERED , FILE_WRITE_ACCESS)

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
	CButton m_cbReadVersion;
	CString m_csVersion;
	afx_msg void OnBnClickedButtonReadVersion();
};
