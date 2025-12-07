#include <ntifs.h>

static VOID DriverUnload(DRIVER_OBJECT* DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}
extern"C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	DriverObject->DriverUnload = DriverUnload;
	UNREFERENCED_PARAMETER(RegistryPath);

	return STATUS_SUCCESS;
}