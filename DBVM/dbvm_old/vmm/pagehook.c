#include "main.h"
#include "vmmhelper.h"
#include "vmcall.h"
#include "vmcallstructs.h"
#include "mm.h"

typedef struct
{
  int Active;
  QWORD Pfn;
  QWORD ExecutePfn;
}EPTHookEntry,*PEPTHookEntry;

criticalSection eptHookListCS={.name="eptHookListCS", .debuglevel=2};
PEPTHookEntry eptHookList;
int eptHookListSize;
int eptHookListPos;

int ept_hook_split(QWORD Pfn,QWORD ExecutePfn)
{
  int ID = -1;
  int i;
  int exist = 0;
  csEnter(&eptHookListCS);
  for(i=0;i<eptHookListPos;i++)
  {
    if(!eptHookList[i].Active)
    {
      if(ID == -1)
      {
        ID = i;
      }
    }
    else if(eptHookList[i].Pfn == Pfn)
    {
      exist = 1;
      break;
    }
  }
  //判断是否已存在
  if(exist)
  {
    return 1;
  }

  if(ID == -1)
  {
    if(eptHookListPos+1 >= eptHookListSize)
    {
      if(eptHookListSize==0 || eptHookList == NULL)
      {
        eptHookListSize = eptHookListSize+2;
        eptHookList = malloc(sizeof(*eptHookList)*eptHookListSize);
      }
      else
      {
        eptHookListSize = (eptHookListSize+2)*2;
        eptHookList = realloc(eptHookList,sizeof(*eptHookList)*eptHookListSize);
      }
    }
    ID = eptHookListPos++;
  }

  eptHookList[ID].Active = 1;
  eptHookList[ID].Pfn = Pfn;
  eptHookList[ID].ExecutePfn = ExecutePfn;

  pcpuinfo currentcpuinfo=getcpuinfo();
  pcpuinfo c=firstcpuinfo;
  while(c)
  {
    QWORD PA_PTE;
    
    csEnter(&c->EPTPML4CS);
    if(isAMD)
    {
      PA_PTE=NPMapPhysicalMemory(c, Pfn<<12, 1);
      PPTE_PAE ppte = mapPhysicalMemory(PA_PTE,8);
      ppte->EXB = 1;
      ppte->PFN = eptHookList[ID].Pfn;
      unmapPhysicalMemory(ppte,8);
    }
    else
    {
      PA_PTE=EPTMapPhysicalMemory(c, Pfn<<12, 1);
      PEPT_PTE ppte = mapPhysicalMemory(PA_PTE,8);
      ppte->XA = 0;
      ppte->PFN = eptHookList[ID].Pfn;
      unmapPhysicalMemory(ppte,8);
    }
    _wbinvd();
    c->eptUpdated=1;

    csLeave(&c->EPTPML4CS);
    c=c->next;
  }
  csLeave(&eptHookListCS);

  ept_invalidate();
  return 0;
}

int ept_hook_merge(QWORD Pfn)
{
  int i = -1;
  int ID = -1;

  csEnter(&eptHookListCS);
  for(i=0;i<eptHookListPos;i++)
  {
    if(eptHookList[i].Active && eptHookList[i].Pfn == Pfn)
    {
      ID = i;
      break;
    }
  }
  if(ID == -1)
  {
    csLeave(&eptHookListCS);
    return 0;
  }
  
  pcpuinfo currentcpuinfo=getcpuinfo();
  pcpuinfo c=firstcpuinfo;
  while(c)
  {
    QWORD PA_PTE;
    
    csEnter(&c->EPTPML4CS);
    if(isAMD)
    {
      PA_PTE=NPMapPhysicalMemory(c, Pfn<<12, 1);
      PPTE_PAE ppte = mapPhysicalMemory(PA_PTE,8);
      ppte->P = 1;
      ppte->RW = 1;
      ppte->PFN = Pfn;
      ppte->EXB = 0;
      unmapPhysicalMemory(ppte,8);
      //如果有挂起的HOOK
      for(int i=0;i<sizeof(c->eptHookPending)/sizeof(c->eptHookPending[0]);i++)
      {
        if(c->eptHookPending[i].ID == ID + 1)
        {
          c->eptHookPending[i].ID = 0;
        }
      }
    }
    else
    {
      PA_PTE=EPTMapPhysicalMemory(c, Pfn<<12, 1);
      PEPT_PTE ppte = mapPhysicalMemory(PA_PTE,8);
      ppte->RA = 1;
      ppte->WA = 1;
      ppte->XA = 1;
      ppte->PFN = Pfn;
      unmapPhysicalMemory(ppte,8);
    }
    _wbinvd();
    c->eptUpdated=1;
    csLeave(&c->EPTPML4CS);
    c=c->next;
  }
  eptHookList[ID].Active = 0;
  csLeave(&eptHookListCS);

  ept_invalidate();
  return 0;
}

QWORD ept_hook_query(QWORD Pfn)
{
  int i;
  QWORD result = 0;
  csEnter(&eptHookListCS);
  for(i=0;i<eptHookListPos;i++)
  {
    if(eptHookList[i].Active && eptHookList[i].Pfn == Pfn)
    {
      result = eptHookList[i].ExecutePfn;
      break;
    }
  }
  csLeave(&eptHookListCS);
  return result;
}

typedef struct
{
  VMCALL_BASIC vmcall;
  QWORD Pfn;
  QWORD ExecutePfn;
} __attribute__((__packed__)) VMCALL_EPTHOOK_SPLIT_PARAM, *PVMCALL_EPTHOOK_SPLIT_PARAM;

typedef struct
{
  VMCALL_BASIC vmcall;
  QWORD Pfn;
} __attribute__((__packed__)) VMCALL_EPTHOOK_MERGE_PARAM, *PVMCALL_EPTHOOK_MERGE_PARAM;

typedef struct
{
  VMCALL_BASIC vmcall;
  QWORD Pfn;
} __attribute__((__packed__)) VMCALL_EPTHOOK_QUERY_PARAM, *PVMCALL_EPTHOOK_QUERY_PARAM;

void eptHookVMCALL(pcpuinfo currentcpuinfo, VMRegisters *vmregisters, ULONG *vmcall_instruction)
{
  switch (vmcall_instruction[2])
  {
    case 999:
    {
      vmregisters->rax = 0x33445566;
      break;
    }
    //增加HOOK页面
    case 1000:
    {
      if(hasEPTsupport || hasNPsupport)
      {
        vmregisters->rax = ept_hook_split(((PVMCALL_EPTHOOK_SPLIT_PARAM)vmcall_instruction)->Pfn,((PVMCALL_EPTHOOK_SPLIT_PARAM)vmcall_instruction)->ExecutePfn);
      }
      else
      {
        vmregisters->rax = 0x0;
      }
      break;
    }
    case 1001:
    {
      if(hasEPTsupport || hasNPsupport)
      {
        vmregisters->rax = ept_hook_merge(((PVMCALL_EPTHOOK_MERGE_PARAM)vmcall_instruction)->Pfn);
      }
      else
      {
        vmregisters->rax = 0x0;
      }
      break;
    }
    case 1002:
    {
      if(hasEPTsupport || hasNPsupport)
      {
        vmregisters->rax = ept_hook_query(((PVMCALL_EPTHOOK_QUERY_PARAM)vmcall_instruction)->Pfn);
      }
      else
      {
        vmregisters->rax = 0x0;
      }
      break;
    }
    default:
      break;
  }
}

BOOL eptHookHandleEvent(pcpuinfo currentcpuinfo, QWORD Address)
{
  EPT_VIOLATION_INFO evi;
  NP_VIOLATION_INFO nvi;

  int i;
  int ID = -1;
  QWORD PA_PTE;

  QWORD AddressPfn=(Address & MAXPHYADDRMASKPB)>>12;

  csEnter(&eptHookListCS);

  for(i=0;i<eptHookListPos;i++)
  {
    if(eptHookList[i].Active && eptHookList[i].Pfn == AddressPfn)
    {
      ID = i;
      break;
    }
  }
  if(ID == -1)
  {
    csLeave(&eptHookListCS);
    return FALSE;
  }
  sendstring("eptHookHandleEvent\n");
  csEnter(&currentcpuinfo->EPTPML4CS);
  if(isAMD)
  {
    PA_PTE = NPMapPhysicalMemory(currentcpuinfo,Address,1);
    PPTE_PAE ppte = mapPhysicalMemory(PA_PTE,8);
    //记录Pending
    for(int i=0;i<sizeof(currentcpuinfo->eptHookPending)/sizeof(currentcpuinfo->eptHookPending[0]);i++)
    {
      if(!currentcpuinfo->eptHookPending[i].ID)
      {
        currentcpuinfo->eptHookPending[i].ID = ID + 1;
        currentcpuinfo->eptHookPending[i].RIPPfn = currentcpuinfo->vmcb->RIP >> 12;
        break;
      }
    }
    //将pfn切换为执行页面,并设置为可执行属性
    ppte->PFN = eptHookList[ID].ExecutePfn;
    ppte->P = 1;
    ppte->RW = 1;
    ppte->EXB = 0;
    unmapPhysicalMemory(ppte,8);
  }
  else
  {
    PA_PTE = EPTMapPhysicalMemory(currentcpuinfo,Address,1);
    PEPT_PTE ppte = mapPhysicalMemory(PA_PTE,8);
    
    //记录Pending
    for(int i=0;i<sizeof(currentcpuinfo->eptHookPending)/sizeof(currentcpuinfo->eptHookPending[0]);i++)
    {
      if(!currentcpuinfo->eptHookPending[i].ID)
      {
        currentcpuinfo->eptHookPending[i].ID = ID + 1;
        currentcpuinfo->eptHookPending[i].RIPPfn = vmread(vm_guest_rip) >> 12;
        break;
      }
    }
    //将pfn切换为执行页面,并设置为可执行属性
    ppte->PFN = eptHookList[ID].ExecutePfn;
    ppte->RA = 1;
    ppte->WA = 1;
    ppte->XA = 1;
    
   /*
    if(evi.X && !evi.WasExecutable)
    {
      ppte->RA = 0;
      ppte->WA = 0;
      ppte->XA = 1;
      ppte->PFN = eptHookList[i].ExecutePfn;
    }
    else
    {
      ppte->RA = 1;
      ppte->WA = 1;
      ppte->XA = 0;
      ppte->PFN = eptHookList[i].Pfn;
    }
      */
    unmapPhysicalMemory(ppte,8);
  }
  csLeave(&currentcpuinfo->EPTPML4CS);

  _wbinvd();
  csLeave(&eptHookListCS);
  return TRUE;
}

void eptHookHandleEventBefore(pcpuinfo currentcpuinfo)
{
  int inv = 0;
  csEnter(&currentcpuinfo->EPTPML4CS);
  QWORD rip = isAMD ? currentcpuinfo->vmcb->RIP : vmread(vm_guest_rip);
  for(int i=0;i<sizeof(currentcpuinfo->eptHookPending)/sizeof(currentcpuinfo->eptHookPending[0]);i++)
  {
    if(currentcpuinfo->eptHookPending[i].ID
      && (rip>>12) != currentcpuinfo->eptHookPending[i].RIPPfn
      && ((rip+16)>>12) != currentcpuinfo->eptHookPending[i].RIPPfn)
    {
      int ID = currentcpuinfo->eptHookPending[i].ID - 1;
      currentcpuinfo->eptHookPending[i].ID = 0;
      sendstring("eptHookNPEventHandle restore PTE\n");
      QWORD PA_PTE;
      if(isAMD)
      {
        PA_PTE = NPMapPhysicalMemory(currentcpuinfo,eptHookList[ID].Pfn<<12,1);
        PPTE_PAE ppte = mapPhysicalMemory(PA_PTE,8);
        ppte->PFN = eptHookList[ID].Pfn;
        ppte->P = 1;
        ppte->RW = 1;
        ppte->EXB = 1;
        unmapPhysicalMemory(ppte,8);
      }
      else
      {
        PA_PTE = EPTMapPhysicalMemory(currentcpuinfo,eptHookList[ID].Pfn<<12,1);
        PEPT_PTE ppte = mapPhysicalMemory(PA_PTE,8);
        ppte->PFN = eptHookList[ID].Pfn;
        ppte->RA = 1;
        ppte->WA = 1;
        ppte->XA = 0;
        unmapPhysicalMemory(ppte,8);
      }
      inv = 1;
    }
  }
  if(inv)
  {
    _wbinvd();
    ept_invalidate();
  }
  csLeave(&currentcpuinfo->EPTPML4CS);
}