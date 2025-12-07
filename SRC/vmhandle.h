#ifndef VM_PATCH_H_H_H
#define VM_PATCH_H_H_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif // _MSC_VER

typedef union
{
	unsigned long long flags;
	struct
	{
		unsigned long long Pcid : 12;
		unsigned long long PageFrameNumber : 36;
		unsigned long long Reserved1 : 12;
		unsigned long long Reserved2 : 3;
		unsigned long long PcidInvalidate : 1;
	};
} CR3;
typedef union
{
	unsigned long long flags;
	struct
	{
		struct
		{
			unsigned long long Present : 1;
			unsigned long long Write : 1;
			unsigned long long User : 1;
			unsigned long long WriteThrough : 1;
			unsigned long long CacheDisable : 1;
			unsigned long long Accessed : 1;
			unsigned long long Dirty : 1;
			unsigned long long LargePage : 1;
			unsigned long long Global : 1;
			unsigned long long Reserved1 : 2;
			unsigned long long R : 1;
			unsigned long long PageFrameNumber : 40;
			unsigned long long Avl : 11;
			unsigned long long ExecuteDisable : 1;
		};
	};
} PTE;
typedef struct
{
	unsigned long long Rax;
	unsigned long long Rcx;
	unsigned long long Rbx;
	unsigned long long Rdx;
	unsigned long long Rsi;
	unsigned long long Rdi;
	unsigned long long Rbp;
	unsigned long long Rsp;
	unsigned long long R8;
	unsigned long long R9;
	unsigned long long R10;
	unsigned long long R11;
	unsigned long long R12;
	unsigned long long R13;
	unsigned long long R14;
	unsigned long long R15;
} CONTEXT;
#pragma pack(push,1)
typedef struct
{
	unsigned short InterceptCrRead;
	unsigned short InterceptCrWrite;
	unsigned short InterceptDrRead;
	unsigned short InterceptDrWrite;
	unsigned int InterceptExceptions;
	unsigned int InterceptIntercept1;
	unsigned int InterceptIntercept2;
	unsigned char Reserved1[40];
	unsigned short PauseFilterThreshold; //3c=correct
	unsigned short PauseFilterCount;
	unsigned long long IoBitmapPA;
	unsigned long long MsrBitmapPA;
	unsigned long long TscOffset;
	unsigned int Asid;					//58=correct
	unsigned char TlbControl;
	unsigned char Reserved2[3];

	unsigned char VTpr;					//60=correct
	unsigned char VIrq : 1;
	unsigned char VGif : 1;
	unsigned char Reserved3 : 6;
	unsigned char VIntrPrio : 4;
	unsigned char VIngTpr : 1;
	unsigned char Reserved4 : 3;
	unsigned char VIntrMasking : 1;
	unsigned char VGifEnabled : 1;
	unsigned char Reserved5 : 5;
	unsigned char AvicEnabled : 1;

	unsigned char VIntrVector;			//64=correct
	unsigned char Reserved6[3];

	unsigned long long InterruptShadowEnable : 1;
	unsigned long long InterruptShadowMask : 1;	// Value of the RFLAGS.IF bit for the guest.
	unsigned long long Reserved7 : 62;
	unsigned long long ExitCode;			//70=correct
	unsigned long long ExitInfo1;
	unsigned long long ExitInfo2;
	unsigned int ExitIntInfo;
	unsigned int ExitIntErrorCode;
	unsigned long long NestedPagingEnable : 1;	//90=correct
	unsigned long long Reserved8 : 63;
	unsigned long long AvicApicBar;
	unsigned long long GuestPaOfGhcb;
	unsigned int InjectIntInfo;
	unsigned int InjectIntErrorCode;
	unsigned long long NCr3;
	unsigned long long LbrVirtualizationEnable : 1;
	unsigned long long VirtualVmSaveVmLoad : 1;
	unsigned long long Reserved9 : 62;
	union
	{
		unsigned long long Value;
		struct
		{
			unsigned long long Intercepts : 1;
			unsigned long long Iomsrpm : 1;
			unsigned long long Asid : 1;
			unsigned long long Tpr : 1;
			unsigned long long NestedPaging : 1;
			unsigned long long Cr : 1;
			unsigned long long Dr : 1;
			unsigned long long Dt : 1;
			unsigned long long Seg : 1;
			unsigned long long Cr2 : 1;
			unsigned long long Lbr : 1;
			unsigned long long Avic : 1;
		};
	} VmcbCleanBits;
	unsigned long long NRip;					//c8=correct
	unsigned char NumberOfBytesFetched;
	unsigned char GuestInstructionBytes[15];
	unsigned long long AvicApicBackingPagePointer;
	unsigned long long reserved10;
	unsigned long long AvicLogicalTablePointer;
	unsigned long long AvicPhysicalTablePointer;
	unsigned long long reserved11;
	unsigned long long VmcbSaveStatePointer;
	unsigned char reserved12[752];     //0x110=correct

	unsigned short  EsSelector;
	unsigned short  EsAttribute;
	unsigned int EsLimit;
	unsigned long long EsBaseAddress;

	unsigned short  CsSelector;
	unsigned short  CsAttribute;
	unsigned int CsLimit;
	unsigned long long CsBaseAddress;

	unsigned short  SsSelector;
	unsigned short  SsAttribute;
	unsigned int SsLimit;
	unsigned long long SsBaseAddress;

	unsigned short  DsSelector;
	unsigned short  DsAttribute;
	unsigned int DsLimit;
	unsigned long long DsBaseAddress;

	unsigned short  FsSelector;
	unsigned short  FsAttribute;
	unsigned int FsLimit;
	unsigned long long FsBaseAddress;

	unsigned short  GsSelector;
	unsigned short  GsAttribute;
	unsigned int GsLimit;
	unsigned long long GsBaseAddress;

	unsigned int reserved13;
	unsigned int GdtrLimit;
	unsigned long long GdtrBaseAddress;

	unsigned short  LdtrSelector;
	unsigned short  LdtrAttribute;
	unsigned int LdtrLimit;
	unsigned long long LdtrBaseAddress;

	unsigned int reserved14;
	unsigned int IdtrLimit;
	unsigned long long IdtrBaseAddress;

	unsigned short  TrSelector;
	unsigned short  TrAttribute;
	unsigned int TrLimit;
	unsigned long long TrBaseAddress;

	unsigned char  Reserved14[43];
	unsigned char  Cpl;
	unsigned int Reserved15;
	unsigned long long Efer;

	unsigned char Reserved16[112];
	unsigned long long Cr4;
	unsigned long long Cr3;
	unsigned long long Cr0;
	unsigned long long Dr7;
	unsigned long long Dr6;
	unsigned long long RFlags;
	unsigned long long Rip;				//0x578=correct

	unsigned char Reserved17[88];
	unsigned long long Rsp;				//5d8=correct

	unsigned char Reserved18[24];  //5e0
	unsigned long long Rax;
	unsigned long long Star;
	unsigned long long LStar;
	unsigned long long CStar;
	unsigned long long FMask;
	unsigned long long KernelGsBase;
	unsigned long long SysenterCs;
	unsigned long long SysenterEsp;
	unsigned long long SysenterEip;
	unsigned long long Cr2; //640=correct

	unsigned char Reserved19[32];
	unsigned long long Gpat;
	unsigned long long DebugCtl;
	unsigned long long BrFrom;
	unsigned long long BrTo;
	unsigned long long LastExcpFrom;
	unsigned long long LastExcpTo; //690=correct
}VMCB;
#pragma pack(pop)

/**************** 需要平台实现的函数 *********************/
// 获取虚拟地址
void* GetVirtualForPhysical(unsigned long long PhysicalAddress, unsigned long long* pByteCount);
// 申请一个物理页面
unsigned long long AllocPhysicalPage();
/*******************************************************/


int VMXExitBefore(CONTEXT* Context);
void VMXExitAfter();

int SVMExitBefore(VMCB* pVMCB, CONTEXT* Context);
void SVMExitAfter();


#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif