#include "stdafx.h"

/*
	drv - Main file
	This file contains a very simple implementation of a WDM driver. Note that it does not support all
	WDM functionality, or any functionality sufficient for practical use. The only thing this driver does
	perfectly, is loading and unloading.

	To install the driver, go to Control Panel -> Add Hardware Wizard, then select "Add a new hardware device".
	Select "manually select from list", choose device category, press "Have Disk" and enter the path to your
	INF file.
	Note that not all device types (specified as Class in INF file) can be installed that way.

	To start/stop this driver, use Windows Device Manager (enable/disable device command).

	If you want to speed up your driver development, it is recommended to see the BazisLib library, that
	contains convenient classes for standard device types, as well as a more powerful version of the driver
	wizard. To get information about BazisLib, see its website:
		http://bazislib.sysprogs.org/
*/

typedef struct _deviceExtension
{
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT TargetDeviceObject;
	PDEVICE_OBJECT PhysicalDeviceObject;
	UNICODE_STRING DeviceInterface;

	PIRP Irp;
	KSPIN_LOCK SpinIrp;

    PVOID MemBar0;
    ULONG nMemBar0Len;

    PKINTERRUPT InterruptObject;

	PVOID dmaBuffer;
	ULONG nBufferLen;

	ULONG nBytesMissed;
} drv_DEVICE_EXTENSION, *Pdrv_DEVICE_EXTENSION;

void drvUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS drvCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject);
NTSTATUS drvPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvReadPciConfig(IN Pdrv_DEVICE_EXTENSION pDevExt, IN PIO_STACK_LOCATION pIrpStack);
VOID drvDpcForIsr(IN PKDPC Dpc, IN struct _DEVICE_OBJECT *DeviceObject, OPTIONAL struct _IRP *Irp, IN PVOID Context);
BOOLEAN drvInterruptService(IN struct _KINTERRUPT  *Interrupt, IN PVOID  ServiceContext);
NTSTATUS drvIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

// {00ac1f07-61bb-4a65-9c83-6898b3802dd0}
static const GUID GUID_drvInterface = {0xAC1F07, 0x61bb, 0x4a65, {0x9c, 0x83, 0x68, 0x98, 0xb3, 0x80, 0x2d, 0xd0 } };

static const ULONG VERSION_BAR0 = 0x0000;
static const ULONG RESET_BAR0 = 0x0004;
static const ULONG WORK_MODE_BAR0 = 0x0008;
static const ULONG GLOBAL_INT_MASK_BAR0 = 0x000C;
static const ULONG INT_STATUS_BAR0 = 0x0010;
static const ULONG INT_MASK_BAR0 = 0x0014;
static const ULONG RESOLUTION_BAR0 = 0x0018;
static const ULONG DMA_ADDR_BAR0 = 0x0020;
static const ULONG DMA_LEN_BAR0 = 0x0024;
static const ULONG DMA_START_BAR0 = 0x0028;
static const ULONG QUANTIZATION_BAR1 = 0x0100;

static const ULONG RESET_CMD = 0x00;
static const ULONG WORK_MODE_JPEG_90 = 0x00;
static const ULONG WORK_MODE_JPEG_80 = 0x01;
static const ULONG WORK_MODE_JPEG_70 = 0x02;
static const ULONG WORK_MODE_JPEG_60 = 0x03;
static const ULONG WORK_MODE_PNG = 0x04;
static const ULONG DMA_START_CMD = 0x01;


#define IOCTL_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x800 , METHOD_BUFFERED , FILE_READ_ACCESS)
#define IOCTL_RESET CTL_CODE(FILE_DEVICE_UNKNOWN , 0x801 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_MODE CTL_CODE(FILE_DEVICE_UNKNOWN , 0x802 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_QUANT CTL_CODE(FILE_DEVICE_UNKNOWN , 0x803 , METHOD_BUFFERED , FILE_WRITE_ACCESS)
#define IOCTL_SET_RESOLUTION CTL_CODE(FILE_DEVICE_UNKNOWN , 0x804 , METHOD_BUFFERED , FILE_WRITE_ACCESS)

#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	unsigned i;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = drvDefaultHandler;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = drvCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = drvCreateClose;
	DriverObject->MajorFunction[IRP_MJ_PNP] = drvPnP;
	DriverObject->MajorFunction[IRP_MJ_READ] = drvRead;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = drvIOControl;

	DriverObject->DriverUnload = drvUnload;
	DriverObject->DriverExtension->AddDevice = drvAddDevice;

	return STATUS_SUCCESS;
}

void drvUnload(IN PDRIVER_OBJECT DriverObject)
{
}

NTSTATUS drvCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS drvDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Pdrv_DEVICE_EXTENSION deviceExtension = NULL;
	
	IoSkipCurrentIrpStackLocation(Irp);
	deviceExtension = (Pdrv_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
	return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
}

NTSTATUS drvAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	Pdrv_DEVICE_EXTENSION pExtension = NULL;
	NTSTATUS status;
	
	status = IoCreateDevice(DriverObject,
						    sizeof(drv_DEVICE_EXTENSION),
							NULL,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							TRUE,
							&DeviceObject);

	if (!NT_SUCCESS(status))
		return status;

	pExtension = (Pdrv_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	pExtension->DeviceObject = DeviceObject;
	pExtension->PhysicalDeviceObject = PhysicalDeviceObject;
	pExtension->TargetDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

	pExtension->Irp = NULL;
//	KeInitializeSpinLock(&pExtension->SpinIrp);
//	KeInitializeEvent(&pExtension->ReadEvent, SynchronizationEvent, FALSE);
	pExtension->dmaBuffer = NULL;
	pExtension->nBufferLen = 0;
	pExtension->nBytesMissed = 0;

	/*设置DPC*/
	IoInitializeDpcRequest(pExtension->DeviceObject, drvDpcForIsr);

	status = IoRegisterDeviceInterface(PhysicalDeviceObject, &GUID_drvInterface, NULL, &pExtension->DeviceInterface);
	ASSERT(NT_SUCCESS(status));

	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	DeviceObject->Flags |= DO_DIRECT_IO;

	return STATUS_SUCCESS;
}


NTSTATUS drvIrpCompletion(
					  IN PDEVICE_OBJECT DeviceObject,
					  IN PIRP Irp,
					  IN PVOID Context
					  )
{
	PKEVENT Event = (PKEVENT) Context;

	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

	return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS drvForwardIrpSynchronous(
							  IN PDEVICE_OBJECT DeviceObject,
							  IN PIRP Irp
							  )
{
	Pdrv_DEVICE_EXTENSION   deviceExtension;
	KEVENT event;
	NTSTATUS status;

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	deviceExtension = (Pdrv_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, drvIrpCompletion, &event, TRUE, TRUE, TRUE);

	status = IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = Irp->IoStatus.Status;
	}
	return status;
}

NTSTATUS drvPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	Pdrv_DEVICE_EXTENSION pExt = ((Pdrv_DEVICE_EXTENSION)DeviceObject->DeviceExtension);
	NTSTATUS status;

	ASSERT(pExt);

	switch (irpSp->MinorFunction)
	{
	case IRP_MN_START_DEVICE:
		status = drvForwardIrpSynchronous(DeviceObject, Irp);
		if(NT_SUCCESS(status))
		{
			status = drvReadPciConfig(pExt, irpSp);
			if(NT_SUCCESS(status))
			{
				IoSetDeviceInterfaceState(&pExt->DeviceInterface, TRUE);
			}
		}

		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
		break;

	case IRP_MN_QUERY_REMOVE_DEVICE:
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
		break;

	case IRP_MN_REMOVE_DEVICE:
		IoSetDeviceInterfaceState(&pExt->DeviceInterface, FALSE);
		status = drvForwardIrpSynchronous(DeviceObject, Irp);
		IoDetachDevice(pExt->TargetDeviceObject);
		IoDeleteDevice(pExt->DeviceObject);
		RtlFreeUnicodeString(&pExt->DeviceInterface);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
		break;

	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		status = drvForwardIrpSynchronous(DeviceObject, Irp);
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
		break;
	}
	return drvDefaultHandler(DeviceObject, Irp);
}

NTSTATUS drvRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Pdrv_DEVICE_EXTENSION pDevExt;
	PIO_STACK_LOCATION stack;
	PHYSICAL_ADDRESS highestAddress;

	pDevExt = (Pdrv_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if(pDevExt->Irp != NULL)
	{
		Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	stack = IoGetCurrentIrpStackLocation(Irp);

	if(stack->Parameters.Read.Length >> 2 == 0)
	{
		Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	highestAddress.HighPart = 0;
	highestAddress.LowPart = 0xffffffff;

	pDevExt->Irp = Irp;
	pDevExt->nBufferLen = (stack->Parameters.Read.Length >> 2) << 2;
	pDevExt->dmaBuffer = MmAllocateContiguousMemory(pDevExt->nBufferLen , highestAddress);

	WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + DMA_ADDR_BAR0) , (ULONG)pDevExt->dmaBuffer);
	WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + DMA_LEN_BAR0) , pDevExt->nBufferLen);
	WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + DMA_START_BAR0) , DMA_START_CMD);

	IoMarkIrpPending(Irp);

	return STATUS_PENDING;
}

NTSTATUS drvReadPciConfig(IN Pdrv_DEVICE_EXTENSION pDevExt, IN PIO_STACK_LOCATION pIrpStack)
{
	ULONG index;
	PCM_RESOURCE_LIST pResourceList, pResourceListTranslated;
	PCM_PARTIAL_RESOURCE_LIST prlTranslated;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR prdTranslated;
	PCM_FULL_RESOURCE_DESCRIPTOR frdTranslated;
	ULONG NumCount;
	NTSTATUS status = STATUS_SUCCESS;
    IO_CONNECT_INTERRUPT_PARAMETERS params;

	/*获取设备已经分配的资源*/	
	pResourceListTranslated = pIrpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
	if( (pResourceListTranslated == NULL) || (pResourceListTranslated->Count == 0) )
	{
		return (STATUS_INSUFFICIENT_RESOURCES);
	}
	frdTranslated  = &pResourceListTranslated->List[0];
	prlTranslated  = &frdTranslated->PartialResourceList;
	prdTranslated = prlTranslated->PartialDescriptors;
	NumCount       = prlTranslated->Count;
	for (index=0; index<NumCount ; prdTranslated++,index++ )
	{
		switch (prdTranslated->Type)
		{
			/*I/O端口资源*/
		case CmResourceTypePort:
//			pDevExt->base[count].IoPortSize				= prdTranslated->u.Port.Length;
//			pDevExt->base[count].IoPortMappedAddress		= (PVOID)prdTranslated->u.Port.Start.LowPart;
//			pDevExt->base[count].WhichMapped			= TYPE_IO;
//			count++;
			break;
			/*Memory资源*/
		case CmResourceTypeMemory:
			pDevExt->nMemBar0Len = prdTranslated->u.Memory.Length;
			//pDevExt->base[count].MemoryRegisterAddress	= prdTranslated->u.Memory.Start;
			pDevExt->MemBar0 = MmMapIoSpace(prdTranslated->u.Memory.Start, prdTranslated->u.Memory.Length, MmNonCached);
			break;
			/*中断资源*/
		case CmResourceTypeInterrupt:
			RtlZeroMemory( &params, sizeof(IO_CONNECT_INTERRUPT_PARAMETERS) );
			params.Version = CONNECT_FULLY_SPECIFIED;
			params.FullySpecified.PhysicalDeviceObject = pDevExt->DeviceObject;
			params.FullySpecified.InterruptObject = &(pDevExt->InterruptObject);
			params.FullySpecified.ServiceRoutine = drvInterruptService;
			params.FullySpecified.ServiceContext = pDevExt;
			params.FullySpecified.FloatingSave = FALSE;
			params.FullySpecified.SpinLock = NULL;

			if (prdTranslated->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) {
				// The resource is for a message-based interrupt. Use the u.MessageInterrupt.Translated member of IntResource.

				params.FullySpecified.Vector = prdTranslated->u.MessageInterrupt.Translated.Vector;
				params.FullySpecified.Irql = (KIRQL)prdTranslated->u.MessageInterrupt.Translated.Level;
				params.FullySpecified.SynchronizeIrql = (KIRQL)prdTranslated->u.MessageInterrupt.Translated.Level;
				params.FullySpecified.ProcessorEnableMask = prdTranslated->u.MessageInterrupt.Translated.Affinity;
			} else {
				// The resource is for a line-based interrupt. Use the u.Interrupt member of IntResource.

				params.FullySpecified.Vector = prdTranslated->u.Interrupt.Vector;
				params.FullySpecified.Irql = (KIRQL)prdTranslated->u.Interrupt.Level;
				params.FullySpecified.SynchronizeIrql = (KIRQL)prdTranslated->u.Interrupt.Level;
				params.FullySpecified.ProcessorEnableMask = prdTranslated->u.Interrupt.Affinity;
			}

			params.FullySpecified.InterruptMode = (prdTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive);
			params.FullySpecified.ShareVector = (BOOLEAN)(prdTranslated->ShareDisposition == CmResourceShareShared);

			//status = IoConnectInterruptEx(&params);
			//if(!NT_SUCCESS(status))
			//{
			//	return status;
			//}

			break;
			/*DMA资源*/
		case CmResourceTypeDma:
			break;
			/*总线资源*/
		case CmResourceTypeBusNumber:
			break;
		default:
			break;
		}
	}

	return (STATUS_SUCCESS);
}

VOID drvDpcForIsr(IN PKDPC Dpc, IN struct _DEVICE_OBJECT *DeviceObject, OPTIONAL struct _IRP *Irp, IN PVOID Context)
{
	Pdrv_DEVICE_EXTENSION pDevExt;
	ULONG nBytesRead , nBytesMissed;
	ULONG i;
	PVOID systemBuffer;

	pDevExt = (Pdrv_DEVICE_EXTENSION)Context;

	if(pDevExt->Irp == NULL)
	{
		return;
	}

	systemBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress , NormalPagePriority);
	memcpy(systemBuffer , pDevExt->dmaBuffer , pDevExt->nBufferLen);
	MmFreeContiguousMemory(pDevExt->dmaBuffer);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = pDevExt->nBufferLen;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	pDevExt->dmaBuffer = NULL;
	pDevExt->nBufferLen = 0;
	pDevExt->Irp = NULL;

    return;
}

BOOLEAN drvInterruptService(IN struct _KINTERRUPT  *Interrupt, IN PVOID  ServiceContext)
{
	Pdrv_DEVICE_EXTENSION pDevExt;

	//清中断？

	pDevExt = (Pdrv_DEVICE_EXTENSION)ServiceContext;

	if(pDevExt->Irp != NULL)
	{
		IoRequestDpc(pDevExt->DeviceObject , pDevExt->Irp , ServiceContext);
	}

	return TRUE;
}

NTSTATUS drvIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION stack;
	ULONG nBytesIn , nBytesOut , ctlCode;
	PVOID buffer;
	Pdrv_DEVICE_EXTENSION pDevExt;
	ULONG mode , resolution;

	pDevExt = (Pdrv_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(Irp);
	nBytesIn = stack->Parameters.DeviceIoControl.InputBufferLength;
	nBytesOut = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ctlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	buffer = Irp->AssociatedIrp.SystemBuffer;

	switch(ctlCode)
	{
	case IOCTL_VERSION:
		if(nBytesOut < sizeof(ULONG))
		{
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		*((PULONG)buffer) = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + VERSION_BAR0));
		Irp->IoStatus.Information = sizeof(ULONG);
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_RESET:
		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + RESET_BAR0) , RESET_CMD);
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_SET_MODE:
		if(nBytesIn < sizeof(ULONG))
		{
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		mode = *((PULONG)buffer);
		switch(mode)
		{
		case WORK_MODE_JPEG_90:
		case WORK_MODE_JPEG_80:
		case WORK_MODE_JPEG_70:
		case WORK_MODE_JPEG_60:
		case WORK_MODE_PNG:
			WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + WORK_MODE_BAR0) , mode);
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		default:
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_INVALID_VARIANT;
			break;
		}

		break;
	case IOCTL_SET_QUANT:
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_SET_RESOLUTION:
		if(nBytesIn < sizeof(ULONG))
		{
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		resolution = *((PULONG)buffer);
		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + WORK_MODE_BAR0) , resolution);

		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	default:
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_INVALID_VARIANT;
		break;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Irp->IoStatus.Status;
}
