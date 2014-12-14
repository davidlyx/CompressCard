
// test_appDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "test_app.h"
#include "test_appDlg.h"
#include "afxdialogex.h"
#include <Setupapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Ctest_appDlg 对话框




Ctest_appDlg::Ctest_appDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Ctest_appDlg::IDD, pParent)
	, m_csVersion(_T(""))
	, m_nMode(_T(""))
	, m_csResHor(_T(""))
	, m_csResVer(_T(""))
	, m_csModeCur(_T(""))
	, m_csResHorCur(_T(""))
	, m_csResVerCur(_T(""))
	, m_csQuant(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	hHandle = INVALID_HANDLE_VALUE;
}

void Ctest_appDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_OPEN, cbtOpen);
	DDX_Control(pDX, IDC_BUTTON_READ_VERSION, m_cbReadVersion);
	DDX_Text(pDX, IDC_EDIT_VERSION, m_csVersion);
	DDX_Control(pDX, IDC_BUTTON_RESET, m_cbReset);
	DDX_Control(pDX, IDC_BUTTON_SET_MODE, m_cbSetMode);
	DDX_Control(pDX, IDC_BUTTON_READ_MODE, m_cbReadMode);
	DDX_Control(pDX, IDC_COMBO_MODE, m_cbMode);
	DDX_CBString(pDX, IDC_COMBO_MODE, m_nMode);
	DDX_Control(pDX, IDC_BUTTON_SET_RES, m_cbSetRes);
	DDX_Control(pDX, IDC_BUTTON5, m_cbReadRes);
	DDX_Text(pDX, IDC_EDIT_HOR, m_csResHor);
	DDX_Text(pDX, IDC_EDIT_VER, m_csResVer);
	DDX_Text(pDX, IDC_EDIT_MODE, m_csModeCur);
	DDX_Text(pDX, IDC_EDIT_HOR_CUR, m_csResHorCur);
	DDX_Text(pDX, IDC_EDIT_VER_CUR, m_csResVerCur);
	DDX_Text(pDX, IDC_EDIT_QUANT, m_csQuant);
	DDX_Control(pDX, IDC_BUTTON_SET_QUANT_1, m_cbSetQuant1);
	DDX_Control(pDX, IDC_BUTTON_READ_QUANT1, m_cbReadQuant1);
	DDX_Control(pDX, IDC_BUTTON_SET_QUANT_2, m_cbSetQuant2);
	DDX_Control(pDX, IDC_BUTTON_READ_QUANT2, m_cbReadQuant2);
}

BEGIN_MESSAGE_MAP(Ctest_appDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &Ctest_appDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_READ_VERSION, &Ctest_appDlg::OnBnClickedButtonReadVersion)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &Ctest_appDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_SET_MODE, &Ctest_appDlg::OnBnClickedButtonSetMode)
	ON_BN_CLICKED(IDC_BUTTON_READ_MODE, &Ctest_appDlg::OnBnClickedButtonReadMode)
	ON_BN_CLICKED(IDC_BUTTON_SET_RES, &Ctest_appDlg::OnBnClickedButtonSetRes)
	ON_BN_CLICKED(IDC_BUTTON_READ_RES, &Ctest_appDlg::OnBnClickedButtonReadRes)
	ON_BN_CLICKED(IDC_BUTTON_SET_QUANT_1, &Ctest_appDlg::OnBnClickedButtonSetQuant1)
	ON_BN_CLICKED(IDC_BUTTON_READ_QUANT1, &Ctest_appDlg::OnBnClickedButtonReadQuant1)
	ON_BN_CLICKED(IDC_BUTTON_SET_QUANT_2, &Ctest_appDlg::OnBnClickedButtonSetQuant2)
	ON_BN_CLICKED(IDC_BUTTON_READ_QUANT2, &Ctest_appDlg::OnBnClickedButtonReadQuant2)
END_MESSAGE_MAP()


// Ctest_appDlg 消息处理程序

BOOL Ctest_appDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_cbReadVersion.EnableWindow(FALSE);
	m_cbReset.EnableWindow(FALSE);
	m_cbSetMode.EnableWindow(FALSE);
	m_cbReadMode.EnableWindow(FALSE);
	m_cbSetRes.EnableWindow(FALSE);
	m_cbReadRes.EnableWindow(FALSE);
	m_cbReadQuant1.EnableWindow(FALSE);
	m_cbReadQuant2.EnableWindow(FALSE);
	m_cbSetQuant1.EnableWindow(FALSE);
	m_cbSetQuant2.EnableWindow(FALSE);

	m_cbMode.InsertString(0 , _T("JPEG 90%"));
	m_cbMode.InsertString(1 , _T("JPEG 80%"));
	m_cbMode.InsertString(2 , _T("JPEG 70%"));
	m_cbMode.InsertString(3 , _T("JPEG 60%"));
	m_cbMode.InsertString(4 , _T("PNG"));

	m_cbMode.SetCurSel(0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Ctest_appDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Ctest_appDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Ctest_appDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Ctest_appDlg::OnBnClickedButtonOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	GUID devGuid={0xAC1F07, 0x61bb, 0x4a65, {0x9c, 0x83, 0x68, 0x98, 0xb3, 0x80, 0x2d, 0xd0 } };
	HDEVINFO hDevInfo;
	SP_INTERFACE_DEVICE_DATA IfDevData;
	SP_INTERFACE_DEVICE_DETAIL_DATA *IfDevDetail = NULL;
	DWORD ReqLen;

	if(hHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hHandle);
		hHandle = INVALID_HANDLE_VALUE;
		cbtOpen.SetWindowTextW(_T("打开设备"));
		m_cbReadVersion.EnableWindow(FALSE);
		m_cbReset.EnableWindow(FALSE);
		m_cbSetMode.EnableWindow(FALSE);
		m_cbReadMode.EnableWindow(FALSE);
		m_cbSetRes.EnableWindow(FALSE);
		m_cbReadRes.EnableWindow(FALSE);
		m_cbReadQuant1.EnableWindow(FALSE);
		m_cbReadQuant2.EnableWindow(FALSE);
		m_cbSetQuant1.EnableWindow(FALSE);
		m_cbSetQuant2.EnableWindow(FALSE);
		return;
	}

	// HDEVINFO as all source device is generated
	hDevInfo = SetupDiGetClassDevs(&devGuid,
		0, // Enumerator
		0, // 
		DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		// Error processing
		MessageBox(_T("SetupDiGetClassDevs error"));
		return;
	}

	// All device is enumerated
	IfDevData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

	if(!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &devGuid, 0, &IfDevData))
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBox(_T("SetupDiEnumDeviceInterfaces error"));
		return;
	}

	// A necessary amount of the memory in the buffer is obtained
	SetupDiGetDeviceInterfaceDetail(hDevInfo ,&IfDevData, NULL, 0, &ReqLen, NULL);
	// Memory allocation to acquire detailed information
	IfDevDetail = (SP_INTERFACE_DEVICE_DETAIL_DATA *)(new char[ReqLen]);
	IfDevDetail->cbSize=sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

	// Detailed information (pass) is actually acquired
	if(!SetupDiGetDeviceInterfaceDetail(hDevInfo,&IfDevData, IfDevDetail, ReqLen, NULL, NULL))
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBox(_T("SetupDiGetDeviceInterfaceDetail error"));
		return;
	}

	hHandle=CreateFile(IfDevDetail->DevicePath, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
		NULL);

	// Opening of allocated memory
	delete IfDevDetail;

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);

	if(hHandle != INVALID_HANDLE_VALUE)
	{
		cbtOpen.SetWindowTextW(_T("关闭设备"));
		m_cbReadVersion.EnableWindow(TRUE);
		m_cbReset.EnableWindow(TRUE);
		m_cbSetMode.EnableWindow(TRUE);
		m_cbReadMode.EnableWindow(TRUE);
		m_cbSetRes.EnableWindow(TRUE);
		m_cbReadRes.EnableWindow(TRUE);
		m_cbReadQuant1.EnableWindow(TRUE);
		m_cbReadQuant2.EnableWindow(TRUE);
		m_cbSetQuant1.EnableWindow(TRUE);
		m_cbSetQuant2.EnableWindow(TRUE);
	}
	else
	{
		DWORD dwError;
		dwError = GetLastError();
		m_csVersion.Format(_T("%d") , dwError);
		UpdateData(FALSE);
	}

	return;
}


void Ctest_appDlg::OnBnClickedButtonReadVersion()
{
	// TODO: 在此添加控件通知处理程序代码
	ULONG nVersion;
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_VERSION , NULL , 0 , &nVersion , sizeof(nVersion) , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	m_csVersion.Format(_T("%08X") , ntohl(nVersion));
	UpdateData(FALSE);

	return;
}


void Ctest_appDlg::OnBnClickedButtonReset()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_RESET , NULL , 0 , NULL , 0 , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}


void Ctest_appDlg::OnBnClickedButtonSetMode()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD mode;
	DWORD nRead;

	UpdateData(TRUE);
	mode = htonl(m_cbMode.GetCurSel());

	if(DeviceIoControl(hHandle , IOCTL_SET_MODE , &mode , sizeof(mode) , NULL , 0 , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}


void Ctest_appDlg::OnBnClickedButtonReadMode()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD mode;
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_SET_MODE , NULL , 0 , &mode , sizeof(mode) , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	mode = ntohl(mode);
	switch(mode)
	{
	case WORK_MODE_JPEG_90:
		m_csModeCur.Format(_T("JPEG 90%%"));
		break;
	case WORK_MODE_JPEG_80:
		m_csModeCur.Format(_T("JPEG 80%%"));
		break;
	case WORK_MODE_JPEG_70:
		m_csModeCur.Format(_T("JPEG 70%%"));
		break;
	case WORK_MODE_JPEG_60:
		m_csModeCur.Format(_T("JPEG 60%%"));
		break;
	case WORK_MODE_PNG:
		m_csModeCur.Format(_T("PNG"));
		break;
	default:
		m_csModeCur.Format(_T("unknown type %d") , mode);
		break;
	}

	UpdateData(FALSE);

	return;
}


void Ctest_appDlg::OnBnClickedButtonSetRes()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD res , hor , ver;
	DWORD nRead;

	UpdateData(TRUE);

	swscanf(m_csResHor , _T("%d") , &hor);
	swscanf(m_csResVer , _T("%d") , &ver);
	res = htonl((hor << 16) + ver);

	if(DeviceIoControl(hHandle , IOCTL_SET_RESOLUTION , &res , sizeof(res) , NULL , 0 , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}


void Ctest_appDlg::OnBnClickedButtonReadRes()
{
	// TODO: 在此添加控件通知处理程序代码
	DWORD res;
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_SET_RESOLUTION , NULL , 0 , &res , sizeof(res) , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	res = ntohl(res);
	m_csResHorCur.Format(_T("%d") , res >> 16);
	m_csResVerCur.Format(_T("%d") , res & 0xffff);

	UpdateData(FALSE);

	return;
}


void Ctest_appDlg::OnBnClickedButtonSetQuant1()
{
	// TODO: 在此添加控件通知处理程序代码
	BYTE buffer[QUANTIZATION_LEN];
	DWORD i;
	DWORD nRead;

	for(i = 0 ; i < QUANTIZATION_LEN ; i++)
	{
		buffer[i] = i;
	}

	if(DeviceIoControl(hHandle , IOCTL_SET_QUANTIZATION_1 , buffer , sizeof(buffer) , NULL , 0 , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}


void Ctest_appDlg::OnBnClickedButtonReadQuant1()
{
	// TODO: 在此添加控件通知处理程序代码
	BYTE buffer[QUANTIZATION_LEN];
	DWORD nRead , i;

	if(DeviceIoControl(hHandle , IOCTL_READ_QUANTIZATION_1 , NULL , 0 , buffer , sizeof(buffer) , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	m_csQuant.Empty();
	for(i = 0 ; i < QUANTIZATION_LEN ; i++)
	{
		m_csQuant.AppendFormat(_T("%02X") , buffer[i]);
		if(i > 0 && ((i % 16) == 0))
		{
			m_csQuant.AppendFormat(_T("\n"));
		}
	}

	UpdateData(FALSE);

	return;
}


void Ctest_appDlg::OnBnClickedButtonSetQuant2()
{
	// TODO: 在此添加控件通知处理程序代码
	BYTE buffer[QUANTIZATION_LEN];
	DWORD i;
	DWORD nRead;

	for(i = 0 ; i < QUANTIZATION_LEN ; i++)
	{
		buffer[i] = (~i);
	}

	if(DeviceIoControl(hHandle , IOCTL_SET_QUANTIZATION_2 , buffer , sizeof(buffer) , NULL , 0 , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}


void Ctest_appDlg::OnBnClickedButtonReadQuant2()
{
	// TODO: 在此添加控件通知处理程序代码
	BYTE buffer[QUANTIZATION_LEN];
	DWORD nRead , i;

	if(DeviceIoControl(hHandle , IOCTL_READ_QUANTIZATION_2 , NULL , 0 , buffer , sizeof(buffer) , &nRead , NULL) != TRUE)
	{
		MessageBox(_T("DeviceIoControl error"));
		return;
	}

	m_csQuant.Empty();
	for(i = 0 ; i < QUANTIZATION_LEN ; i++)
	{
		m_csQuant.AppendFormat(_T("%02X") , buffer[i]);
		if(i > 0 && ((i % 16) == 0))
		{
			m_csQuant.AppendFormat(_T("\n"));
		}
	}

	UpdateData(FALSE);

	return;
}
