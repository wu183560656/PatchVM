#include "vmhandle.h"
#include "main.h"

static unsigned long long g_VirutalMemoryBase = 0;
void *malloc2(unsigned int size);
void zeromemory(volatile void *address, unsigned int size);
void* mapPhysicalMemory(unsigned long long PhysicalAddress, unsigned int size);
void unmapPhysicalMemory(void* address, unsigned int size);
UINT64 VirtualToPhysical(void* address);
extern UINT64 getCR3(void);
extern ULONG setCR3(UINT64 newcr3);
// 获取虚拟地址
void* GetVirtualForPhysical(unsigned long long PhysicalAddress, unsigned long long* pByteCount)
{
    if(g_VirutalMemoryBase == 0 || PhysicalAddress >= MAXPHYADDRMASKPB)
    {
        return NULL;
    }
    return (void*)(g_VirutalMemoryBase + PhysicalAddress);
}

// 申请一个物理页面
unsigned long long AllocPhysicalPage()
{
    void *temp=malloc2(4096);
    zeromemory(temp,4096);
    return VirtualToPhysical(temp) & MAXPHYADDRMASKPB;
}

void VmHandleInitialize()
{
    PTE* pPdptTable = (PTE*)malloc2(4096);
    zeromemory(pPdptTable, 4096);
    for(unsigned long long i=0;i<512;i++)
    {
        pPdptTable[i].Present = 1;
        pPdptTable[i].Write = 1;
        pPdptTable[i].LargePage = 1;
        pPdptTable[i].PageFrameNumber = i << 18;   // 每个页目录指向1GB内存
    }

    //查找空项
    CR3 cr3;
    cr3.flags = getCR3();
    PTE* pPml4Table = (PTE*)mapPhysicalMemory(cr3.PageFrameNumber << 12, 4096);
    for(unsigned long long i=10;i<512;i++)
    {
        if(pPml4Table[i].Present == 0)
        {
            pPml4Table[i].flags = 0;
            pPml4Table[i].Present = 1;
            pPml4Table[i].Write = 1;
            pPml4Table[i].PageFrameNumber = (VirtualToPhysical(pPdptTable) >> 12);
            g_VirutalMemoryBase = i << 39;  // 计算虚拟地址基址
            break;
        }
    }
    unmapPhysicalMemory(pPml4Table, 4096);
    _wbinvd();
    setCR3(getCR3()); // 刷新TLB
}