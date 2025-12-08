#include <ntifs.h>

#include "../../SRC/vmcall.hpp"

#ifndef _DEBUG
#pragma optimize( "", off )
#endif // _DEBUG

#pragma code_seg("ADD")
_declspec(noinline) static int volatile TestAdd(int a, int b)
{
	return a + b;
}
#pragma code_seg()

#pragma code_seg("SUB")
_declspec(noinline) static int volatile TestSub(int a, int b)
{
	return a - b;
}
#pragma code_seg()

#ifndef _DEBUG
#pragma optimize( "", on )
#endif // _DEBUG

static VOID DriverUnload(DRIVER_OBJECT* DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

extern"C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	DriverObject->DriverUnload = DriverUnload;
	UNREFERENCED_PARAMETER(RegistryPath);

    ULONG64 addPA = MmGetPhysicalAddress(TestAdd).QuadPart;
    ULONG64 subPA = MmGetPhysicalAddress(TestSub).QuadPart;
    DbgPrintEx(0, 0, "TestAdd:%p\n", TestAdd);
    DbgPrintEx(0, 0, "TestSub:%p\n", TestSub);
    int retCode;
    retCode = VM::EptHookSplit(addPA >> 12, subPA >> 12);
    DbgPrintEx(0, 0, " DBVM::EptHookSplit:%d\n", retCode);

    DbgPrintEx(0, 0, "TestAdd(5,2)=%d\n", TestAdd(5, 2));

    __int64 ExecutePfn = VM::EptHookQuery(addPA >> 12);
    DbgPrintEx(0, 0, " DBVM::EptHookQuery:%p\n", (PVOID)ExecutePfn);

    DbgBreakPoint();
    retCode = VM::EptHookMerge(addPA >> 12);
    DbgPrintEx(0, 0, " DBVM::EptHookMerge:%d\n", retCode);


    DbgPrintEx(0, 0, "TestAdd(6,2)=%d\n", TestAdd(6, 2));
    DbgBreakPoint();

	return STATUS_SUCCESS;
}