
// test_appDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "test_app.h"
#include "test_appDlg.h"
#include "afxdialogex.h"
#include <Setupapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// Ctest_appDlg �Ի���




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


// Ctest_appDlg ��Ϣ�������

BOOL Ctest_appDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Ctest_appDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR Ctest_appDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Ctest_appDlg::OnBnClickedButtonOpen()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GUID devGuid={0xAC1F07, 0x61bb, 0x4a65, {0x9c, 0x83, 0x68, 0x98, 0xb3, 0x80, 0x2d, 0xd0 } };
	HDEVINFO hDevInfo;
	SP_INTERFACE_DEVICE_DATA IfDevData;
	SP_INTERFACE_DEVICE_DETAIL_DATA *IfDevDetail = NULL;
	DWORD ReqLen;

	if(hHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hHandle);
		hHandle = INVALID_HANDLE_VALUE;
		cbtOpen.SetWindowTextW(_T("���豸"));
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
		cbtOpen.SetWindowTextW(_T("�ر��豸"));
	}

	return;
}
