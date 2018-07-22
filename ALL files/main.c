#pragma region DEFINES_AND_INCLUDES

#include "ntf.h"

#define KGETPID             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KGETBASE_ADDRESS    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KREAD				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KWRITE				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\GameHackD");
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\GameHackL");
PDEVICE_OBJECT pDeviceObject = NULL;

typedef struct _DATA
{
	DWORD32 PID;
	DWORD64 Address;
	DWORD64 Buffer;
	SIZE_T Size;

}DATA, *PDATA;
#pragma endregion

VOID Unload(PDRIVER_OBJECT pDriverObject)
{
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(pDeviceObject);

	KdPrint(("Driver unloaded\r\n"));
}

NTSTATUS KeReadMemory(PEPROCESS TargetProcess, PVOID AddressToRead, PVOID Buffer, SIZE_T Size)
{
	PSIZE_T BytesRead;

	if (NT_SUCCESS(MmCopyVirtualMemory(TargetProcess, AddressToRead, PsGetCurrentProcess(), Buffer, Size, KernelMode, &BytesRead))) return STATUS_SUCCESS;
	else return STATUS_ACCESS_DENIED;
}


NTSTATUS KeWriteMemory(PEPROCESS TargetProcess, PVOID AddressToWrite, PVOID Source, SIZE_T Size)
{
	PSIZE_T BytesRead;

	if (NT_SUCCESS(MmCopyVirtualMemory(PsGetCurrentProcess(), Source, TargetProcess, AddressToWrite, Size, KernelMode, &BytesRead))) return STATUS_SUCCESS;
	else return STATUS_ACCESS_DENIED;
}

NTSTATUS CreateCall(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Create call request\r\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS CloseCall(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Close call request\r\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS IoControl(PDEVICE_OBJECT pDeviceObject, PIRP IRP)
{
	PIO_STACK_LOCATION StackIrp = IoGetCurrentIrpStackLocation(IRP);
	NTSTATUS Status;

	PDATA Buffer = (PDATA)IRP->AssociatedIrp.SystemBuffer;
	PEPROCESS TargetProcess;
	WCHAR* ProcessName;

	switch (StackIrp->Parameters.DeviceIoControl.IoControlCode)
	{
	case KREAD: {
		IRP->IoStatus.Information = sizeof(DATA);

		Status = PsLookupProcessByProcessId(Buffer->PID, &TargetProcess);
		if (!NT_SUCCESS(Status))
		{
			Buffer->Buffer = 0;
			IRP->IoStatus.Status = Status;
			break;
		}

		Status = KeReadMemory(TargetProcess, (PVOID)Buffer->Address, (PVOID)&Buffer->Buffer, Buffer->Size);
		if (!NT_SUCCESS(Status))
		{
			Buffer->Buffer = 0;
			IRP->IoStatus.Status = Status;
			break;
		}

		IRP->IoStatus.Status = Status;

		break;
	}

	case KWRITE: {
		IRP->IoStatus.Information = sizeof(DATA);

		Status = PsLookupProcessByProcessId(Buffer->PID, &TargetProcess);
		if (!NT_SUCCESS(Status))
		{
			Buffer->Buffer = 0;
			IRP->IoStatus.Status = Status;
			break;
		}

		Status = KeWriteMemory(TargetProcess, (PVOID)Buffer->Address, (PVOID)&Buffer->Buffer, Buffer->Size);
		if (NT_SUCCESS(Status))
		{
			Buffer->Buffer = 0;
			IRP->IoStatus.Status = Status;
			break;
		}

		IRP->IoStatus.Status = Status;

		break;
	}

	case KGETPID: {
		ProcessName = Buffer->Buffer;

		break;
	}

	case KGETBASE_ADDRESS: {

		break;
	}

	default:{
		Status = STATUS_INVALID_PARAMETER;
		IRP->IoStatus.Status = Status;
		IRP->IoStatus.Information = 0;
	}
	}

	IoCompleteRequest(IRP, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status = STATUS_SUCCESS;

	Status = IoCreateDevice(pDriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("Unable to create IO device\r\n"));
		return Status;
	}

	Status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("Unable to create symbolic link\r\n"));
		IoDeleteDevice(pDeviceObject);
		return Status;
	}

	pDriverObject->DriverUnload = Unload;

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;


	KdPrint(("Driver loaded\r\n"));

	return Status;
}