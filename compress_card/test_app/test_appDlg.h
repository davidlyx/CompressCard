
// test_appDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <WinIoCtl.h>


#define IOCTL_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x800 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_RESET CTL_CODE(FILE_DEVICE_UNKNOWN , 0x801 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_MODE CTL_CODE(FILE_DEVICE_UNKNOWN , 0x802 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANT CTL_CODE(FILE_DEVICE_UNKNOWN , 0x803 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_RESOLUTION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x804 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANTIZATION_1 CTL_CODE(FILE_DEVICE_UNKNOWN , 0x805 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANTIZATION_2 CTL_CODE(FILE_DEVICE_UNKNOWN , 0x806 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_READ_QUANTIZATION_1 CTL_CODE(FILE_DEVICE_UNKNOWN , 0x807 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_READ_QUANTIZATION_2 CTL_CODE(FILE_DEVICE_UNKNOWN , 0x808 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_READ_ADDR CTL_CODE(FILE_DEVICE_UNKNOWN , 0x809 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_WRITE_ADDR CTL_CODE(FILE_DEVICE_UNKNOWN , 0x80A , METHOD_BUFFERED , FILE_WRITE_ACCESS)



static const ULONG WORK_MODE_JPEG_90 = 0x00;
static const ULONG WORK_MODE_JPEG_80 = 0x01;
static const ULONG WORK_MODE_JPEG_70 = 0x02;
static const ULONG WORK_MODE_JPEG_60 = 0x03;
static const ULONG WORK_MODE_PNG = 0x04;

static const ULONG QUANTIZATION_LEN = 1024;

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
	CButton m_cbReadVersion;
	CString m_csVersion;
	afx_msg void OnBnClickedButtonReadVersion();
	afx_msg void OnBnClickedButtonReset();
	CButton m_cbReset;
	CButton m_cbSetMode;
	CButton m_cbReadMode;
	CComboBox m_cbMode;
	CString m_nMode;
	CButton m_cbSetRes;
	CButton m_cbReadRes;
	CString m_csResHor;
	CString m_csResVer;
	afx_msg void OnBnClickedButtonSetMode();
	afx_msg void OnBnClickedButtonReadMode();
	afx_msg void OnBnClickedButtonSetRes();
	afx_msg void OnBnClickedButtonReadRes();
	CString m_csModeCur;
	CString m_csResHorCur;
	CString m_csResVerCur;
	afx_msg void OnBnClickedButtonSetQuant1();
	afx_msg void OnBnClickedButtonReadQuant1();
	afx_msg void OnBnClickedButtonSetQuant2();
	afx_msg void OnBnClickedButtonReadQuant2();
	CString m_csQuant;
	CButton m_cbSetQuant1;
	CButton m_cbReadQuant1;
	CButton m_cbSetQuant2;
	CButton m_cbReadQuant2;
	CButton m_cbReadAddr;
	CButton m_cbWriteAddr;
	CString m_csReadAddr;
	CString m_csWriteAddr;
	CString m_csReadData;
	CString m_csWriteData;
	afx_msg void OnBnClickedButtonReadAddr();
	afx_msg void OnBnClickedButtonWriteAddr();
};
