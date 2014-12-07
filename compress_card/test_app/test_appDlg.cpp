
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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	hHandle = INVALID_HANDLE_VALUE;
}

void Ctest_appDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_OPEN, cbtOpen);
}

BEGIN_MESSAGE_MAP(Ctest_appDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &Ctest_appDlg::OnBnClickedButtonOpen)
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
	}

	return;
}
