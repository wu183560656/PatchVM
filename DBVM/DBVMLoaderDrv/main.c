#include <ntifs.h>
#include <intrin.h>
#include "DBKKernel/DBKFunc.h"
#include "DBKKernel/vmxoffload.h"
#include "DBKKernel/vmxhelper.h"


static BOOLEAN ExecuteOnEachProcessor(BOOLEAN(*Function)(PVOID), PVOID Context)
{
    BOOLEAN Result = TRUE;
    PROCESSOR_NUMBER processorNumber;
    GROUP_AFFINITY affinity;
    GROUP_AFFINITY oldAffinity;
    ULONG CpuCount = KeQueryActiveProcessorCountEx(0);
    for (ULONG cpu_index = 0; cpu_index < CpuCount && Result; cpu_index++)
    {
        KeGetProcessorNumberFromIndex(cpu_index, &processorNumber);
        affinity.Group = processorNumber.Group;
        affinity.Mask = 1ULL << processorNumber.Number;
        affinity.Reserved[0] = affinity.Reserved[1] = affinity.Reserved[2] = 0;
        KeSetSystemGroupAffinityThread(&affinity, &oldAffinity);
        Result = Function(Context);
        KeRevertToUserGroupAffinityThread(&oldAffinity);
    }
    return Result;
}

static VOID DBVMEntry()
{
    PVOID DeferredContex = NULL;
    PVOID SystemArgument1 = NULL;
    PVOID SystemArgument2 = NULL;
    vmxoffload_override(0, NULL, DeferredContex, &SystemArgument1, &SystemArgument2);
    vmxoffload_dpc(NULL, DeferredContex, SystemArgument1, SystemArgument2);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    char ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * sizeof(wchar_t)]={ 0 };
    wchar_t* vmdiskPath = NULL;
    if (!DriverObject)
    {
        //使用kdmapper映射，第二个参数就是hvm.dll路径
        const wchar_t* UservmdiskPath = (wchar_t*)RegistryPath;
        vmdiskPath = (wchar_t*)ValueBuffer;
        wcscpy_s(vmdiskPath, sizeof(ValueBuffer) / sizeof(wchar_t), UservmdiskPath);
    }
    else
    {
        //读取注册表得到hvm.dll文件路径
        OBJECT_ATTRIBUTES object_attributes;
        HANDLE reg = NULL;
        ULONG ActualSize = 0;
        UNICODE_STRING ImagePathName = RTL_CONSTANT_STRING(L"ImagePath");
        PKEY_VALUE_PARTIAL_INFORMATION ValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        memset(ValueBuffer, 0, sizeof(ValueBuffer));
        InitializeObjectAttributes(&object_attributes, RegistryPath, OBJ_KERNEL_HANDLE, NULL, NULL);
        if (NT_SUCCESS(ZwOpenKey(&reg, KEY_QUERY_VALUE, &object_attributes)))
        {
            if (NT_SUCCESS(ZwQueryValueKey(reg, &ImagePathName, KeyValuePartialInformation, ValueInformation, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256, &ActualSize)))
            {
                PWSTR Path = (PWSTR)(ValueInformation->Data);
                for (LONG i = (ValueInformation->DataLength / 2) - 1; i >= 0; i--)
                {
                    if (Path[i] == L'\\')
                        break;
                    Path[i] = L'\0';
                }
                wcscat(Path, L"vmdisk.img");
                vmdiskPath = Path;
            }
            ZwClose(reg);
        }
    }
    DbgPrintEx(0, 0, "vmdisk.img FileName: %ws\n", vmdiskPath);
    if (!vmdiskPath)
    {
        return -2;
    }
    int r[4];
    __cpuid(r, 0);
    if (r[1] == 0x756e6547) //GenuineIntel
    {
        vmx_init_dovmcall(1);
    }
    else
    {
        DbgPrint("Not an intel cpu");
        vmx_init_dovmcall(0);
    }

    initializeDBVM(vmdiskPath);
    //ExecuteOnEachProcessor(DBVMEntry, NULL);
    forEachCpu(vmxoffload_dpc, NULL, NULL, NULL, vmxoffload_override);
    cleanupDBVM();
    return -1;
}