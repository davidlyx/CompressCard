// cc_dll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "cc_dll.h"
#include <Setupapi.h>



// 这是导出函数的一个示例。
CC_DLL_API HANDLE ccOpen(void)
{
	GUID devGuid={0xAC1F07, 0x61bb, 0x4a65, {0x9c, 0x83, 0x68, 0x98, 0xb3, 0x80, 0x2d, 0xd0 } };
	HDEVINFO hDevInfo;
	SP_INTERFACE_DEVICE_DATA IfDevData;
	SP_INTERFACE_DEVICE_DETAIL_DATA *IfDevDetail = NULL;
	DWORD ReqLen;
	HANDLE hHandle;

	// HDEVINFO as all source device is generated
	hDevInfo = SetupDiGetClassDevs(&devGuid,
		0, // Enumerator
		0, // 
		DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		// Error processing
		//MessageBox(_T("SetupDiGetClassDevs error"));
		return INVALID_HANDLE_VALUE;
	}

	// All device is enumerated
	IfDevData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

	if(!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &devGuid, 0, &IfDevData))
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		//MessageBox(_T("SetupDiEnumDeviceInterfaces error"));
		return INVALID_HANDLE_VALUE;
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
		//MessageBox(_T("SetupDiGetDeviceInterfaceDetail error"));
		return INVALID_HANDLE_VALUE;
	}

	hHandle=CreateFile(IfDevDetail->DevicePath, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
		NULL);

	// Opening of allocated memory
	delete IfDevDetail;

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return hHandle;
}

CC_DLL_API void ccClose(HANDLE hHandle)
{
	CloseHandle(hHandle);
	return;
}

CC_DLL_API ULONG ccReadVersion(HANDLE hHandle)
{
	ULONG nVersion;
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_VERSION , NULL , 0 , &nVersion , sizeof(nVersion) , &nRead , NULL) != TRUE)
	{
		//MessageBox(_T("DeviceIoControl error"));
		return 0xffffffff;
	}

	return nVersion;
}

CC_DLL_API void ccReset(HANDLE hHandle)
{
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_RESET , NULL , 0 , NULL , 0 , &nRead , NULL) != TRUE)
	{
		//MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}

CC_DLL_API void ccSetMode(HANDLE hHandle , DWORD mode)
{
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_SET_MODE , &mode , sizeof(mode) , NULL , 0 , &nRead , NULL) != TRUE)
	{
		//MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}

CC_DLL_API DWORD ccReadMode(HANDLE hHandle)
{
	DWORD mode;
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_SET_MODE , NULL , 0 , &mode , sizeof(mode) , &nRead , NULL) != TRUE)
	{
		//MessageBox(_T("DeviceIoControl error"));
		return 0xffffffff;
	}

	return mode;
}

CC_DLL_API void ccSetResolution(HANDLE hHandle , DWORD hor , DWORD ver)
{
	DWORD res;
	DWORD nRead;

	res = (hor << 16) + ver;

	if(DeviceIoControl(hHandle , IOCTL_SET_RESOLUTION , &res , sizeof(res) , NULL , 0 , &nRead , NULL) != TRUE)
	{
		//MessageBox(_T("DeviceIoControl error"));
		return;
	}

	return;
}

CC_DLL_API void ccSetResolution(HANDLE hHandle , LPDWORD hor , LPDWORD ver)
{
	DWORD res;
	DWORD nRead;

	if(DeviceIoControl(hHandle , IOCTL_SET_RESOLUTION , NULL , 0 , &res , sizeof(res) , &nRead , NULL) != TRUE)
	{
		//MessageBox(_T("DeviceIoControl error"));
		return;
	}

	*hor = res >> 16;
	*ver = res & 0xffff;

	return;
}

CC_DLL_API void ccSetQuant(HANDLE hHandle)
{
	return;
}
