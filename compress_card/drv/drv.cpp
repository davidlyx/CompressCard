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
	//KSPIN_LOCK SpinIrp;

    PVOID MemBar0;
    ULONG nMemBar0Len;

    PKINTERRUPT InterruptObject;

	PVOID dmaBuffer;
	ULONG nBufferLen;
	PHYSICAL_ADDRESS dmaPhysicalAddr;

	ULONG nIntVersion;
	IO_CONNECT_INTERRUPT_PARAMETERS params;

	PDMA_ADAPTER pDmaAdapter;
	ULONG NumberOfMapRegisters;
} drv_DEVICE_EXTENSION, *Pdrv_DEVICE_EXTENSION;

void drvUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS drvCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject);
NTSTATUS drvIrpCompletion(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp,IN PVOID Context);
NTSTATUS drvForwardIrpSynchronous(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS drvPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvReadPciConfig(IN Pdrv_DEVICE_EXTENSION pDevExt, IN PIO_STACK_LOCATION pIrpStack);
NTSTATUS drvRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID drvStartIO(IN PDEVICE_OBJECT DeviceObject , IN PIRP Irp);
BOOLEAN drvInterruptService(IN struct _KINTERRUPT  *Interrupt, IN PVOID  ServiceContext);
BOOLEAN drvInterruptMessageService(IN struct _KINTERRUPT  *Interrupt, IN PVOID  ServiceContext, IN ULONG  MessageId);
VOID drvDpcForIsr(IN PKDPC Dpc, IN struct _DEVICE_OBJECT *DeviceObject, OPTIONAL struct _IRP *Irp, IN PVOID Context);
VOID drvOnCancelIrp(IN PDEVICE_OBJECT DeviceObject , IN PIRP Irp);
NTSTATUS drvIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

// {00ac1f07-61bb-4a65-9c83-6898b3802dd0}
static const GUID GUID_drvInterface = {0xAC1F07, 0x61bb, 0x4a65, {0x9c, 0x83, 0x68, 0x98, 0xb3, 0x80, 0x2d, 0xd0 } };

static const ULONG VERSION_BAR0 = 0x0000;
static const ULONG RESET_BAR0 = 0x0004;
static const ULONG WORK_MODE_BAR0 = 0x0008;
static const ULONG GLOBAL_INT_MASK_BAR0 = 0x000C;
static const ULONG INT_MASK_BAR0 = 0x0010;
static const ULONG INT_STATUS_BAR0 = 0x0014;
static const ULONG RESOLUTION_BAR0 = 0x0018;
static const ULONG DMA_ADDR_BAR0 = 0x0020;
static const ULONG DMA_LEN_BAR0 = 0x0024;
static const ULONG DMA_START_BAR0 = 0x0028;
static const ULONG QUANTIZATION_1_BAR0 = 0x3000;
static const ULONG QUANTIZATION_2_BAR0 = 0x3400;

static const ULONG RESET_CMD = 0x00;
static const ULONG WORK_MODE_JPEG_90 = 0x00;
static const ULONG WORK_MODE_JPEG_80 = 0x01;
static const ULONG WORK_MODE_JPEG_70 = 0x02;
static const ULONG WORK_MODE_JPEG_60 = 0x03;
static const ULONG WORK_MODE_PNG = 0x04;
static const ULONG DMA_START_CMD = 0x01;
static const ULONG DMA_STOP_INT = 0x01;

static const ULONG QUANTIZATION_LEN = 1024;

static const ULONG READ_BUFFER_SIZE = 2048;
static const ULONG MAX_TRANSFER_SIZE = 32768;


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

	DriverObject->DriverStartIo = drvStartIO;
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
	PHYSICAL_ADDRESS highestAddr;
	DEVICE_DESCRIPTION dmaParam;
	
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

	pExtension->InterruptObject = NULL;
	pExtension->Irp = NULL;
//	KeInitializeSpinLock(&pExtension->SpinIrp);
//	KeInitializeEvent(&pExtension->ReadEvent, SynchronizationEvent, FALSE);
	pExtension->dmaBuffer = NULL;
	pExtension->nBufferLen = READ_BUFFER_SIZE;
	pExtension->dmaPhysicalAddr.QuadPart = 0;
	pExtension->pDmaAdapter = NULL;

	RtlZeroMemory(&dmaParam , sizeof(dmaParam));
	dmaParam.Version = DEVICE_DESCRIPTION_VERSION1;
	dmaParam.Master = TRUE;
	dmaParam.ScatterGather = FALSE;
	dmaParam.Dma32BitAddresses = TRUE;
	dmaParam.IgnoreCount = FALSE;
	dmaParam.InterfaceType = PCIBus;
	dmaParam.MaximumLength = MAX_TRANSFER_SIZE;

	pExtension->pDmaAdapter = IoGetDmaAdapter(pExtension->PhysicalDeviceObject , &dmaParam , &(pExtension->NumberOfMapRegisters));
	if(pExtension->pDmaAdapter == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pExtension->dmaBuffer = pExtension->pDmaAdapter->DmaOperations->AllocateCommonBuffer(pExtension->pDmaAdapter , READ_BUFFER_SIZE , &(pExtension->dmaPhysicalAddr) , TRUE);
	if(pExtension->dmaBuffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(pExtension->dmaBuffer , READ_BUFFER_SIZE);

	/*设置DPC*/
	IoInitializeDpcRequest(pExtension->DeviceObject, drvDpcForIsr);

	IoSetStartIoAttributes(DeviceObject , TRUE , FALSE);

	status = IoRegisterDeviceInterface(PhysicalDeviceObject, &GUID_drvInterface, NULL, &pExtension->DeviceInterface);
	ASSERT(NT_SUCCESS(status));

	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	DeviceObject->Flags |= DO_BUFFERED_IO;

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
	IO_DISCONNECT_INTERRUPT_PARAMETERS params;

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

		if(pExt->pDmaAdapter != NULL)
		{
			if(pExt->dmaBuffer != NULL)
			{
				pExt->pDmaAdapter->DmaOperations->FreeCommonBuffer(pExt->pDmaAdapter , pExt->nBufferLen , pExt->dmaPhysicalAddr , pExt->dmaBuffer , TRUE);
				pExt->dmaBuffer = NULL;
				pExt->nBufferLen = 0;
			}

			pExt->pDmaAdapter->DmaOperations->PutDmaAdapter(pExt->pDmaAdapter);
			pExt->pDmaAdapter = NULL;
		}

		if(pExt->InterruptObject != NULL)
		{
			params.Version = pExt->nIntVersion;
			params.ConnectionContext.InterruptObject = pExt->InterruptObject;
			IoDisconnectInterruptEx(&params);
		}

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

NTSTATUS drvReadPciConfig(IN Pdrv_DEVICE_EXTENSION pDevExt, IN PIO_STACK_LOCATION pIrpStack)
{
	ULONG index;
	PCM_RESOURCE_LIST pResourceList, pResourceListTranslated;
	PCM_PARTIAL_RESOURCE_LIST prlTranslated;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR prdTranslated;
	PCM_FULL_RESOURCE_DESCRIPTOR frdTranslated;
	ULONG NumCount;
	NTSTATUS status = STATUS_SUCCESS;
    IO_CONNECT_INTERRUPT_PARAMETERS paramsMsi , paramsFs;
	DEVICE_DESCRIPTION dmaParam;

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
			// Set members of params.MessageBased here.
			RtlZeroMemory(&paramsMsi, sizeof(IO_CONNECT_INTERRUPT_PARAMETERS));
			paramsMsi.Version = CONNECT_MESSAGE_BASED;
			paramsMsi.MessageBased.PhysicalDeviceObject = pDevExt->PhysicalDeviceObject;
			paramsMsi.MessageBased.MessageServiceRoutine = drvInterruptMessageService;
			paramsMsi.MessageBased.ServiceContext = pDevExt;
			paramsMsi.MessageBased.SpinLock = NULL;
			paramsMsi.MessageBased.SynchronizeIrql = 0;
			paramsMsi.MessageBased.FloatingSave = FALSE;
			paramsMsi.MessageBased.FallBackServiceRoutine = drvInterruptService;

			// Set members of params.FullySpecified here.
			RtlZeroMemory(&paramsFs, sizeof(IO_CONNECT_INTERRUPT_PARAMETERS));
			paramsFs.Version = CONNECT_FULLY_SPECIFIED;
			paramsFs.FullySpecified.PhysicalDeviceObject = pDevExt->PhysicalDeviceObject;
			paramsFs.FullySpecified.InterruptObject = &(pDevExt->InterruptObject);
			paramsFs.FullySpecified.ServiceRoutine = drvInterruptService;
			paramsFs.FullySpecified.ServiceContext = pDevExt;
			paramsFs.FullySpecified.FloatingSave = FALSE;
			paramsFs.FullySpecified.SpinLock = NULL;

			if (prdTranslated->Flags & CM_RESOURCE_INTERRUPT_MESSAGE)
			{
				// The resource is for a message-based interrupt. Use the u.MessageInterrupt.Translated member of IntResource.

				paramsFs.FullySpecified.Vector = prdTranslated->u.MessageInterrupt.Translated.Vector;
				paramsFs.FullySpecified.Irql = (KIRQL)prdTranslated->u.MessageInterrupt.Translated.Level;
				paramsFs.FullySpecified.SynchronizeIrql = (KIRQL)prdTranslated->u.MessageInterrupt.Translated.Level;
				paramsFs.FullySpecified.ProcessorEnableMask = prdTranslated->u.MessageInterrupt.Translated.Affinity;
			}
			else
			{
				// The resource is for a line-based interrupt. Use the u.Interrupt member of IntResource.

				paramsFs.FullySpecified.Vector = prdTranslated->u.Interrupt.Vector;
				paramsFs.FullySpecified.Irql = (KIRQL)prdTranslated->u.Interrupt.Level;
				paramsFs.FullySpecified.SynchronizeIrql = (KIRQL)prdTranslated->u.Interrupt.Level;
				paramsFs.FullySpecified.ProcessorEnableMask = prdTranslated->u.Interrupt.Affinity;
			}

			paramsFs.FullySpecified.InterruptMode = (prdTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive);
			paramsFs.FullySpecified.ShareVector = (BOOLEAN)(prdTranslated->ShareDisposition == CmResourceShareShared);

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

	do
	{
		//status = IoConnectInterruptEx(&paramsMsi);
		//if (NT_SUCCESS(status))
		//{
		//	pDevExt->nIntVersion = paramsMsi.Version;
		//	RtlCopyMemory(&pDevExt->params , &paramsMsi , sizeof(paramsMsi));
		//	break;
		//}

		//if (paramsMsi.Version == CONNECT_FULLY_SPECIFIED)
		//break;
		{
			status = IoConnectInterruptEx(&paramsFs);
			if (NT_SUCCESS(status))
			{
				pDevExt->nIntVersion = paramsFs.Version;
				RtlCopyMemory(&pDevExt->params , &paramsFs , sizeof(paramsFs));
				break;
			}
		}

		return status;
	}while(0);

	RtlZeroMemory(&dmaParam , sizeof(dmaParam));
	dmaParam.Version = DEVICE_DESCRIPTION_VERSION1;
	dmaParam.Master = TRUE;
	dmaParam.ScatterGather = FALSE;
	dmaParam.Dma32BitAddresses = TRUE;
	dmaParam.IgnoreCount = FALSE;
	dmaParam.InterfaceType = PCIBus;
	dmaParam.MaximumLength = MAX_TRANSFER_SIZE;

	pDevExt->pDmaAdapter = IoGetDmaAdapter(pDevExt->PhysicalDeviceObject , &dmaParam , &(pDevExt->NumberOfMapRegisters));
	if(pDevExt->pDmaAdapter == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pDevExt->dmaBuffer = pDevExt->pDmaAdapter->DmaOperations->AllocateCommonBuffer(pDevExt->pDmaAdapter , READ_BUFFER_SIZE , &(pDevExt->dmaPhysicalAddr) , TRUE);
	if(pDevExt->dmaBuffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	return (STATUS_SUCCESS);
}

NTSTATUS drvRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION stack;

	stack = IoGetCurrentIrpStackLocation(Irp);

	if(stack->Parameters.Read.Length < READ_BUFFER_SIZE)
	{
		Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_BUFFER_OVERFLOW;
	}

	IoMarkIrpPending(Irp);

	IoStartPacket(DeviceObject , Irp , 0 , drvOnCancelIrp);

	return STATUS_PENDING;
}

VOID drvStartIO(IN PDEVICE_OBJECT DeviceObject , IN PIRP Irp)
{
	KIRQL oldIrql;
	Pdrv_DEVICE_EXTENSION pDevExt;

	IoAcquireCancelSpinLock(&oldIrql);

	if(Irp != DeviceObject->CurrentIrp)
	{
		IoReleaseCancelSpinLock(oldIrql);
		return;
	}

	if(Irp->Cancel == TRUE)
	{
		IoReleaseCancelSpinLock(oldIrql);

		//Irp->IoStatus.Status = STATUS_CANCELLED;
		//Irp->IoStatus.Information = 0;

		//IoStartNextPacket(DeviceObject , TRUE);
		//IoCompleteRequest(Irp , IO_NO_INCREMENT);

		return;
	}

	IoSetCancelRoutine(Irp , NULL);
	IoReleaseCancelSpinLock(oldIrql);

	pDevExt = (Pdrv_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if(pDevExt->Irp == NULL)
	{
		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + DMA_ADDR_BAR0) , RtlUlongByteSwap((ULONG)pDevExt->dmaPhysicalAddr.LowPart));
		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + DMA_LEN_BAR0) , RtlUlongByteSwap(pDevExt->nBufferLen));
		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + DMA_START_BAR0) , RtlUlongByteSwap(DMA_START_CMD));

		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + RESET_BAR0) , RtlUlongByteSwap(0x01));
	}

	pDevExt->Irp = Irp;

	//Irp->IoStatus.Status = STATUS_SUCCESS;
	//Irp->IoStatus.Information = 0;
	//IoCompleteRequest(Irp, IO_NO_INCREMENT);
	//pDevExt->Irp = NULL;
	//IoStartNextPacket(DeviceObject , TRUE);

	IoAcquireCancelSpinLock(&oldIrql);
	IoSetCancelRoutine(Irp , drvOnCancelIrp);
	IoReleaseCancelSpinLock(oldIrql);

	return;
}

BOOLEAN drvInterruptService(IN struct _KINTERRUPT  *Interrupt, IN PVOID  ServiceContext)
{
	Pdrv_DEVICE_EXTENSION pDevExt;
	ULONG status;

	pDevExt = (Pdrv_DEVICE_EXTENSION)ServiceContext;

	//清中断？
	status = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + INT_STATUS_BAR0));
	if(RtlUlongByteSwap(status) == 0)
	{
		return FALSE;
	}

	WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + INT_STATUS_BAR0) , RtlUlongByteSwap(DMA_STOP_INT));

	IoRequestDpc(pDevExt->DeviceObject , NULL , ServiceContext);

	return TRUE;
}

BOOLEAN drvInterruptMessageService(IN struct _KINTERRUPT  *Interrupt, IN PVOID  ServiceContext, IN ULONG  MessageId)
{
	Pdrv_DEVICE_EXTENSION pDevExt;
	ULONG status;

	pDevExt = (Pdrv_DEVICE_EXTENSION)ServiceContext;

	//清中断？
	status = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + INT_STATUS_BAR0));
	if(RtlUlongByteSwap(status) == 0)
	{
		return FALSE;
	}

	IoRequestDpc(pDevExt->DeviceObject , NULL , ServiceContext);

	return TRUE;
}

VOID drvDpcForIsr(IN PKDPC Dpc, IN struct _DEVICE_OBJECT *DeviceObject, OPTIONAL struct _IRP *Irp, IN PVOID Context)
{
	KIRQL oldIrql;
	Pdrv_DEVICE_EXTENSION pDevExt;
	ULONG nBytesRead , nBytesMissed;
	ULONG i;
	PVOID systemBuffer;

	pDevExt = (Pdrv_DEVICE_EXTENSION)Context;

	if(pDevExt->Irp == NULL)
	{
		return;
	}

	IoAcquireCancelSpinLock(&oldIrql);

	if(pDevExt->Irp != DeviceObject->CurrentIrp)
	{
		IoReleaseCancelSpinLock(oldIrql);
		return;
	}

	if(pDevExt->Irp->Cancel == TRUE)
	{
		IoReleaseCancelSpinLock(oldIrql);

		//Irp->IoStatus.Status = STATUS_CANCELLED;
		//Irp->IoStatus.Information = 0;

		//IoStartNextPacket(DeviceObject , TRUE);
		//IoCompleteRequest(pDevExt->Irp , IO_NO_INCREMENT);

		return;
	}

	IoSetCancelRoutine(pDevExt->Irp , NULL);
	IoReleaseCancelSpinLock(oldIrql);

	RtlCopyMemory(pDevExt->Irp->AssociatedIrp.SystemBuffer , pDevExt->dmaBuffer , pDevExt->nBufferLen);

	pDevExt->Irp->IoStatus.Status = STATUS_SUCCESS;
	pDevExt->Irp->IoStatus.Information = pDevExt->nBufferLen;
	IoCompleteRequest(pDevExt->Irp, IO_NO_INCREMENT);

	pDevExt->Irp = NULL;

	IoStartNextPacket(DeviceObject , TRUE);

    return;
}

VOID drvOnCancelIrp(IN PDEVICE_OBJECT DeviceObject , IN PIRP Irp)
{
	if(Irp == DeviceObject->CurrentIrp)
	{
		KIRQL oldIrql = Irp->CancelIrql;
		IoReleaseCancelSpinLock(Irp->CancelIrql);
		IoStartNextPacket(DeviceObject , TRUE);
		KeLowerIrql(oldIrql);
	}
	else
	{
		if(KeRemoveEntryDeviceQueue(&DeviceObject->DeviceQueue , &Irp->Tail.Overlay.DeviceQueueEntry) == FALSE)
		{
			IoReleaseCancelSpinLock(Irp->CancelIrql);
			return;
		}
		else
		{
			IoReleaseCancelSpinLock(Irp->CancelIrql);
		}
	}

	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp , IO_NO_INCREMENT);

	return;
}

NTSTATUS drvIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION stack;
	ULONG nBytesIn , nBytesOut , ctlCode;
	PVOID buffer;
	Pdrv_DEVICE_EXTENSION pDevExt;
	ULONG ulReadBuffer , i;

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

		*((PULONG)buffer) = RtlUlongByteSwap(READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + VERSION_BAR0)));
		Irp->IoStatus.Information = sizeof(ULONG);
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_RESET:
		WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + RESET_BAR0) , RtlUlongByteSwap(RESET_CMD));
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_SET_MODE:
		ulReadBuffer = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + WORK_MODE_BAR0));
		ulReadBuffer = RtlUlongByteSwap(ulReadBuffer);

		if(nBytesIn == sizeof(ULONG))
		{
			WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + WORK_MODE_BAR0) , RtlUlongByteSwap(*((PULONG)buffer)));
		}
		
		if(nBytesOut == sizeof(ULONG))
		{
			*((PULONG)buffer) = ulReadBuffer;
			Irp->IoStatus.Information = sizeof(ULONG);
		}
		else
		{
			Irp->IoStatus.Information = 0;
		}

		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_SET_QUANT:
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_SET_RESOLUTION:
		ulReadBuffer = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + RESOLUTION_BAR0));
		ulReadBuffer = RtlUlongByteSwap(ulReadBuffer);

		if(nBytesIn == sizeof(ULONG))
		{
			WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + RESOLUTION_BAR0) , RtlUlongByteSwap(*((PULONG)buffer)));
		}
		
		if(nBytesOut == sizeof(ULONG))
		{
			*((PULONG)buffer) = ulReadBuffer;
			Irp->IoStatus.Information = sizeof(ULONG);
		}
		else
		{
			Irp->IoStatus.Information = 0;
		}

		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_SET_QUANTIZATION_1:
		if(nBytesIn == QUANTIZATION_LEN)
		{
			for(i = 0 ; i < QUANTIZATION_LEN / sizeof(ULONG) ; i++)
			{
				WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + QUANTIZATION_1_BAR0 + i * sizeof(ULONG)) , *((PULONG)buffer + i));
			}
		}

		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_SET_QUANTIZATION_2:
		if(nBytesIn == QUANTIZATION_LEN)
		{
			for(i = 0 ; i < QUANTIZATION_LEN / sizeof(ULONG) ; i++)
			{
				WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + QUANTIZATION_2_BAR0 + i * sizeof(ULONG)) , *((PULONG)buffer + i));
			}
		}

		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_READ_QUANTIZATION_1:
		if(nBytesOut == QUANTIZATION_LEN)
		{
			for(i = 0 ; i < QUANTIZATION_LEN / sizeof(ULONG) ; i++)
			{
				*((PULONG)buffer + i) = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + QUANTIZATION_1_BAR0 + i * sizeof(ULONG)));
			}
			Irp->IoStatus.Information = QUANTIZATION_LEN;
		}
		else
		{
			Irp->IoStatus.Information = 0;
		}
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_READ_QUANTIZATION_2:
		if(nBytesOut == QUANTIZATION_LEN)
		{
			for(i = 0 ; i < QUANTIZATION_LEN / sizeof(ULONG) ; i++)
			{
				*((PULONG)buffer + i) = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + QUANTIZATION_2_BAR0 + i * sizeof(ULONG)));
			}
			Irp->IoStatus.Information = QUANTIZATION_LEN;
		}
		else
		{
			Irp->IoStatus.Information = 0;
		}
		Irp->IoStatus.Status = STATUS_SUCCESS;

		break;
	case IOCTL_READ_ADDR:
		if(nBytesIn == sizeof(ULONG) && nBytesOut == sizeof(ULONG))
		{
			ulReadBuffer = READ_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + *((PULONG)buffer)));
			*((PULONG)buffer) = ulReadBuffer;
			Irp->IoStatus.Information = sizeof(ULONG);
		}
		else if(nBytesIn == 0 && nBytesOut == pDevExt->nBufferLen)
		{
			RtlCopyMemory(buffer , pDevExt->dmaBuffer , pDevExt->nBufferLen);
			Irp->IoStatus.Information = nBytesOut;
		}
		else if(nBytesIn == 0 && nBytesOut == sizeof(pDevExt->nBufferLen) + sizeof(pDevExt->dmaPhysicalAddr.LowPart) + sizeof(pDevExt->dmaPhysicalAddr.HighPart) + sizeof(pDevExt->dmaBuffer))
		{
			RtlCopyMemory(buffer , &(pDevExt->dmaPhysicalAddr.LowPart) , sizeof(pDevExt->dmaPhysicalAddr.LowPart));
			RtlCopyMemory((PCHAR)buffer + sizeof(pDevExt->dmaPhysicalAddr.LowPart) , &(pDevExt->dmaPhysicalAddr.HighPart) , sizeof(pDevExt->dmaPhysicalAddr.HighPart));
			RtlCopyMemory((PCHAR)buffer + sizeof(pDevExt->dmaPhysicalAddr.LowPart) + sizeof(pDevExt->dmaPhysicalAddr.HighPart) , &(pDevExt->dmaBuffer) , sizeof(pDevExt->dmaBuffer));
			RtlCopyMemory((PCHAR)buffer + sizeof(pDevExt->dmaPhysicalAddr.LowPart) + sizeof(pDevExt->dmaPhysicalAddr.HighPart) + sizeof(pDevExt->dmaBuffer) , &(pDevExt->nBufferLen) , sizeof(pDevExt->nBufferLen));
			Irp->IoStatus.Information = nBytesOut;
		}
		else
		{
			Irp->IoStatus.Information = 0;
		}
		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;
	case IOCTL_WRITE_ADDR:
		if(nBytesIn == 2 * sizeof(ULONG))
		{
			WRITE_REGISTER_ULONG((PULONG)((PUCHAR)pDevExt->MemBar0 + *((PULONG)buffer)) , *((PULONG)buffer + 1));
			Irp->IoStatus.Information = 0;
		}
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
