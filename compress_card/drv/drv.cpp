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

void drvUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS drvCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject);
NTSTATUS drvPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS drvRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID drvDpcForIsr(IN PKDPC Dpc, IN struct _DEVICE_OBJECT *DeviceObject, INOUT struct _IRP *Irp, IN PVOID Context);

typedef struct _deviceExtension
{
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT TargetDeviceObject;
	PDEVICE_OBJECT PhysicalDeviceObject;
	UNICODE_STRING DeviceInterface;

	PIRP Irp;
	KSPIN_LOCK SpinIrp;
	KEVENT ReadEvent;

    PVOID MemBar0;
    ULONG nMemBar0Len;
    PVOID MemBar1;
    ULONG nMemBar1Len;

    PKINTERRUPT InterruptObject;
} drv_DEVICE_EXTENSION, *Pdrv_DEVICE_EXTENSION;

// {00ac1f07-61bb-4a65-9c83-6898b3802dd0}
static const GUID GUID_drvInterface = {0xAC1F07, 0x61bb, 0x4a65, {0x9c, 0x83, 0x68, 0x98, 0xb3, 0x80, 0x2d, 0xd0 } };

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
	//DriverObject->MajorFunction[IRP_MJ_READ] = drvPnP;

	DriverObject->DriverUnload = drvUnload;
	DriverObject->DriverStartIo = NULL;
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

	case IRP_MN_QUERY_REMOVE_DEVICE:
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;

	case IRP_MN_REMOVE_DEVICE:
		IoSetDeviceInterfaceState(&pExt->DeviceInterface, FALSE);
		status = drvForwardIrpSynchronous(DeviceObject, Irp);
		IoDetachDevice(pExt->TargetDeviceObject);
		IoDeleteDevice(pExt->DeviceObject);
		RtlFreeUnicodeString(&pExt->DeviceInterface);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;

	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		status = drvForwardIrpSynchronous(DeviceObject, Irp);
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
	return drvDefaultHandler(DeviceObject, Irp);
}

NTSTATUS drvRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS drvReadPciConfig(IN Pdrv_DEVICE_EXTENSION pDevExt, IN PIO_STACK_LOCATION pIrpStack)
{
	int i;
	ULONG index;
	PCM_RESOURCE_LIST pResourceList, pResourceListTranslated;
	PCM_PARTIAL_RESOURCE_LIST prl, prlTranslated;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR prd, prdTranslated;
	PCM_FULL_RESOURCE_DESCRIPTOR frd, frdTranslated;
	ULONG NumCount;
	NTSTATUS status = STATUS_SUCCESS;
	DEVICE_DESCRIPTION DeviceDescription;
	INTERFACE_TYPE BusType;
	ULONG Junk;
    IO_CONNECT_INTERRUPT_PARAMETERS params;

	PUCHAR pBaseMem0, pBaseMem2;
	ULONG value = 0;
	ULONG iCnt = 0;


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
            if(iCnt == 0)
            {
    			pDevExt->nMemBar0Len = prdTranslated->u.Memory.Length;
//			    pDevExt->base[count].MemoryRegisterAddress	= prdTranslated->u.Memory.Start;
			    pDevExt->MemBar0 = MmMapIoSpace(prdTranslated->u.Memory.Start, prdTranslated->u.Memory.Length, MmNonCached);
            }
            else
            {
    			pDevExt->nMemBar1Len = prdTranslated->u.Memory.Length;
//			    pDevExt->base[count].MemoryRegisterAddress	= prdTranslated->u.Memory.Start;
			    pDevExt->MemBar1 = MmMapIoSpace(prdTranslated->u.Memory.Start, prdTranslated->u.Memory.Length, MmNonCached);
            }
			break;
			/*中断资源*/
		case CmResourceTypeInterrupt:
			pDevExt->InterruptLevel	= (KIRQL)prdTranslated->u.Interrupt.Level;
			pDevExt->InterruptVector	= prdTranslated->u.Interrupt.Vector;
			pDevExt->InterruptAffinity	= prdTranslated->u.Interrupt.Affinity;
			pDevExt->InterruptMode	= (prdTranslated->Flags == CM_RESOURCE_INTERRUPT_LATCHED) ? Latched : LevelSensitive;
			pDevExt->InterruptShare	= (prdTranslated->ShareDisposition == CmResourceShareShared);
			pDevExt->InterruptSupport	= 1;

            RtlZeroMemory(&params, sizeof(IO_CONNECT_INTERRUPT_PARAMETERS));
            params.Version = CONNECT_MESSAGE_BASED;
            params.MessageBased.PhysicalDeviceObject = pDevExt->PhysicalDeviceObject;
            params.MessageBased.MessageServiceRoutine = deviceInterruptMessageService;
            params.MessageBased.ServiceContext = ServiceContext;
            params.MessageBased.SpinLock = NULL;
            params.MessageBased.SynchronizeIrql = 0;
            params.MessageBased.FloatingSave = FALSE;
            params.MessageBased.FallBackServiceRoutine = deviceInterruptService;


    		/*注册中断处理程序*/
    		status = IoConnectInterruptEx(   &pDevExt->InterruptObject,		//. Interrupt object pointer
			(PKSERVICE_ROUTINE)InterruptHandler,	//. Interrupt processing routine name
			pDevExt,								//. Context passed to interrupt handler
			NULL, 									//. Spinlock
			pDevExt->InterruptVector, 				//. Interrupt vector
			pDevExt->InterruptLevel, 				//. Interrupt IRQL
			pDevExt->InterruptLevel, 				//. Most significant IRQL
			pDevExt->InterruptMode,					//. Levelsensitive value or latched value
			pDevExt->InterruptShare,				//. Interrupt share
			pDevExt->InterruptAffinity, 			//. Interrupt affinity
			FALSE  );								//. x86 case FALSE
    		if( !NT_SUCCESS(status) )
    		{
    			return(status);
    		}


		    /*设置DPC*/
    		IoInitializeDpcRequest(pDevExt->PhysicalDeviceObject, drvDpcForIsr);
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
	/* It is fails if there is no resource */
	if ( count == 0 ) 
	{
		return (STATUS_UNSUCCESSFUL);
	}

	{
		/*插槽编号*/
		PCI_SLOT_NUMBER slot_number;
		IoGetDeviceProperty( pDevExt->PhysicalDeviceObject,	DevicePropertyAddress,	sizeof(ULONG), &slot_number, &Junk);
		pDevExt->SlotNumber = (slot_number.u.AsULONG >> 16);
	}

	{
		/*总线编号*/
		PCI_SLOT_NUMBER slot_number;
		IoGetDeviceProperty( pDevExt->PhysicalDeviceObject,	DevicePropertyBusNumber, sizeof(ULONG), &slot_number, &Junk);
		pDevExt->BusNumber = (slot_number.u.AsULONG);
	}

	/*本例中我们只使用了两个BAR*/
	//pBaseMem0 = (PUCHAR)(pDevExt->base[0].MemoryMappedAddress);
	//pBaseMem2 = (PUCHAR)(pDevExt->base[2].MemoryMappedAddress);

	/*本设备我们使用了4124芯片作为PCI芯片，访问前需设置以下两个寄存器*/
	//WRITE_REGISTER_ULONG( (PULONG)(pBaseMem2 + 0x808), 0x1F03C);
	//WRITE_REGISTER_ULONG( (PULONG)(pBaseMem2 + 0x800), 0x25000);	

	//do
	//{
	//	value = READ_REGISTER_ULONG( (PULONG)(pBaseMem2 + 0x808));
	//	if ((value & 0xE0000000) == 0xE0000000)
	//	{
	//		break;
	//	}
	//	iCnt++;
	//}while (iCnt < 10000000);
	//DebugPrint("[0x808] = 0x%08X! \n", value);
	//if (iCnt == 10000000)
	//{
	//	DebugPrint("*************** Clock unlock! \n");
	//}

	if(pDevExt->InterruptSupport != 0)
	{




	}



	//DebugPrint("DemoPciStartDevice() End\n");

	return (STATUS_SUCCESS);
}

VOID drvDpcForIsr(IN PKDPC Dpc, IN struct _DEVICE_OBJECT *DeviceObject, INOUT struct _IRP *Irp, IN PVOID Context)
{
    return;
}

