#include <WinIoCtl.h>

#ifdef CC_DLL_EXPORTS
#define CC_DLL_API __declspec(dllexport)
#else
#define CC_DLL_API __declspec(dllimport)
#endif




#define IOCTL_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x800 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_RESET CTL_CODE(FILE_DEVICE_UNKNOWN , 0x801 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_MODE CTL_CODE(FILE_DEVICE_UNKNOWN , 0x802 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANT CTL_CODE(FILE_DEVICE_UNKNOWN , 0x803 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_RESOLUTION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x804 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANTIZATION_1 CTL_CODE(FILE_DEVICE_UNKNOWN , 0x805 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANTIZATION_2 CTL_CODE(FILE_DEVICE_UNKNOWN , 0x806 , METHOD_BUFFERED , FILE_WRITE_ACCESS)



#define WORK_MODE_JPEG_90_PERCENT 0x00
#define WORK_MODE_JPEG_80_PERCENT 0x01
#define WORK_MODE_JPEG_70_PERCENT 0x02
#define WORK_MODE_JPEG_60_PERCENT 0x03
#define WORK_MODE_PNG_LOSSLESS 0x04
#define QUANT_LEN 1024



CC_DLL_API HANDLE ccOpen(void);
CC_DLL_API void ccClose(HANDLE hHandle);
CC_DLL_API ULONG ccReadVersion(HANDLE hHandle);
CC_DLL_API void ccReset(HANDLE hHandle);
CC_DLL_API void ccSetMode(HANDLE hHandle , DWORD mode);
CC_DLL_API DWORD ccReadMode(HANDLE hHandle);
CC_DLL_API void ccSetResolution(HANDLE hHandle , DWORD hor , DWORD ver);
CC_DLL_API void ccSetResolution(HANDLE hHandle , LPDWORD hor , LPDWORD ver);
CC_DLL_API void ccSetQuant(HANDLE hHandle);