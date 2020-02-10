#include "Read.h"
//ע�⣬������IRP_MJ_READ�е���ObQueryNameString������������������������
VOID FindFileName(POBJECT_NAME_INFORMATION info, PIO_STACK_LOCATION sa)
{
	UNREFERENCED_PARAMETER(info);
	UNREFERENCED_PARAMETER(sa);
	/*ULONG len;*/
	if (sa->FileObject)
	{
		/*ObQueryNameString(sa->FileObject, NULL, 0, &len);
		info = (POBJECT_NAME_INFORMATION)ExAllocatePool(NonPagedPool, len);
		if (info != NULL)
		{
			ObQueryNameString(sa->FileObject, info, len, &len);*/
		DEBUG_ENTER_FUNCTION("the read file directory is:%wZ", sa->FileObject->FileName);
		/*ExFreePool(info);
	}*/
	}
}

NTSTATUS FsFilterDispatchReadComplete(PDEVICE_OBJECT dev, PIRP irp, PVOID context)
{
	UNREFERENCED_PARAMETER(dev);
	UNREFERENCED_PARAMETER(context);
	PFILE_CONTEXT Ctx = (PFILE_CONTEXT)context;
    //1.����STATUS_CONTINUE_COMPLETIONʱ����ҪIoMarkIrpPending
    //2.����ײ�������ͬ�����صģ���ײ������������IoMarkIrpPending����ôPendingReturnedΪFALSE�������������ǲ���Ҫ����KeSetEvent��FsFilterDispatchReadֱ����else�߼�
    //3.����ײ��������첽���ص�(STATUS_PENDING)����ײ����������IoMarkIrpPending����ô��I/O���ʱ���������ǵ���ɻص�����ʱPendingReturnedΪTRUE����FsFilterDispatchRead
    //����KeWaitForSingleObject(Ctx.WaitEvent)��������������Ҫ����KeSetEvent����KeWaitForSingleObject����������
	if (irp->PendingReturned == TRUE)
	{
		DEBUG_ENTER_FUNCTION("read pending");
		KeSetEvent(&Ctx->WaitEvent, IO_NO_INCREMENT, FALSE);
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}
NTSTATUS FsFilterDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	NTSTATUS sta;
	UNREFERENCED_PARAMETER(DeviceObject);
	PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION sa = IoGetCurrentIrpStackLocation(irp);
	/*if (sa->MinorFunction == IRP_MN_MDL)
	{
		DbgPrint("MDL!\n");
	}*/
	if (sa->MinorFunction == IRP_MN_NORMAL)
	{
		FILE_CONTEXT Ctx = { 0 };
		KeInitializeEvent(&Ctx.WaitEvent, NotificationEvent, FALSE);
		POBJECT_NAME_INFORMATION info = NULL;
		DEBUG_ENTER_FUNCTION("read");
		FindFileName(info, sa);
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, FsFilterDispatchReadComplete, &Ctx, TRUE, TRUE, TRUE);
		sta = IoCallDriver(pDevExt->AttachedToDeviceObject, irp);
		if (sta == STATUS_PENDING)
		{
			DEBUG_ENTER_FUNCTION("read wait......");
			KeWaitForSingleObject(&Ctx.WaitEvent, Executive, KernelMode, FALSE, NULL);
            sta = irp->IoStatus.Status;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			if (irp->IoStatus.Status == STATUS_SUCCESS)
			{
				if (irp->MdlAddress)
				{
					DEBUG_ENTER_FUNCTION("read OK:read output buffer is in mdl");
				}
				else if(irp->AssociatedIrp.SystemBuffer)
				{
					DEBUG_ENTER_FUNCTION("read OK:read output buffer is in systembuffer");
				}
				else if(irp->UserBuffer)
				{
					DEBUG_ENTER_FUNCTION("read OK:read output buffer is in userbuffer");
				}
			}
			DEBUG_ENTER_FUNCTION("pending read status:0x%x", irp->IoStatus.Status);
			return sta;
		}
		else
		{
			//��ɺ�������STATUS_MORE_PROCESSING_REQUIREDʱ��Ҫ�ٴε���IoCompleteRequest
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			DEBUG_ENTER_FUNCTION("read status:0x%x", sta);
			return sta;
		}
	}
	else
	{
		//IRP_MN_MDL���Ѵ����Ź���˵
		DEBUG_ENTER_FUNCTION("The request is IRP_MN_MDL!");
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pDevExt->AttachedToDeviceObject, irp);
	}
}