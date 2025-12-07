#ifndef PAGEHOOK_H_
#define PAGEHOOK_H_

/*

vmmhelper.h::tcpuinfo增加成员

//需要两个页面，因为有可能一条指令在两个页面
  struct
  {
    int ID;
    QWORD RIPPfn;
  }eptHookPending[2];
*/

//插入到vmcall.c::_handleVMCallInstruction
void eptHookVMCALL(pcpuinfo currentcpuinfo, VMRegisters *vmregisters, ULONG *vmcall_instruction);
//插入到epthandler.c::handleEPTViolation、nphandler.c::handleNestedPagingFault
BOOL eptHookHandleEvent(pcpuinfo currentcpuinfo, QWORD Address);
//插入到vmeventhandler_amd.c::handleVMEvent_amd、vmeventhandler.c::handleVMEvent
void eptHookHandleEventBefore(pcpuinfo currentcpuinfo);

#endif