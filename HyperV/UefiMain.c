#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../SRC/vmhandle.h"

CONTEXT* Context;
VMCB* VMCBInstance;
static void* CloneSelfToEfiRuntimeServicesCode()
{
    CHAR16* str = NULL;
    Context = (CONTEXT*)gBS->AllocatePool(EfiRuntimeServicesData, sizeof(CONTEXT), (VOID**)&str);
    VMCBInstance = (VMCB*)gBS->AllocatePool(EfiRuntimeServicesData, sizeof(VMCB), (VOID**)&str);
    return NULL;
}


EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE imgHandle, IN EFI_SYSTEM_TABLE* sysTable)
{
    CloneSelfToEfiRuntimeServicesCode();

    VMXExitBefore(Context);
    VMXExitAfter();

    SVMExitBefore(VMCBInstance, Context);
    SVMExitAfter();


    gST = sysTable;
    gBS = sysTable->BootServices;
    gImageHandle = imgHandle;
    // UEFI apps automatically exit after 5 minutes. Stop that here
    gBS->SetWatchdogTimer(0, 0, 0, NULL);
    Print(L"Hello, world!\r\n");
    // Allocate a string
    CHAR16* str = NULL;
    gBS->AllocatePool(EfiLoaderData, 36, (VOID**)&str);
    // Copy over a string
    CHAR16* str2 = L"Allocated string\r\n";
    gBS->CopyMem((VOID*)str, (VOID*)str2, 36);
    Print(str);
    gBS->FreePool(str);
    return EFI_SUCCESS;
}

// 获取虚拟地址
void* GetVirtualForPhysical(unsigned long long PhysicalAddress, unsigned long long* pByteCount)
{
    return (void*)PhysicalAddress;
}
// 申请一个物理页面
unsigned long long AllocPhysicalPage()
{
    CHAR16* str = NULL;
    return (unsigned long long)gBS->AllocatePool(EfiRuntimeServicesData, 36, (VOID**)&str);
}