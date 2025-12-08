#ifdef _MSC_VER

#pragma warning(disable:4201)
#include <intrin.h>

char __invept(unsigned long type, void* descriptor)
{
	#pragma code_seg(".text")
	__declspec(allocate(".text")) const static unsigned char __invept_code[] = {
		0x66, 0x0F, 0x38, 0x80, 0x0A	//invept  rcx, oword ptr [rdx]
		,0x74, 0x05						//jz      @jz
		,0x72, 0x06						//jb      short @jc
		,0x32, 0xC0						//xor     al, al
		,0xC3							//retn
		,0xB0, 0x01						//@jz: mov     al, 1
		,0xC3							//retn
		,0xB0, 0x02						//@jc: mov     al, 2
		,0xC3							//retn
	};
	#pragma code_seg()
	return ((char(*)(unsigned long,void*))__invept_code)(type,descriptor);
}

#elif __GNUC__

#define _InterlockedCompareExchange __sync_val_compare_and_swap
#define _mm_pause() __asm__ __volatile__("pause")
#define __vmx_vmread(msr_id, value) ({                                       \
	unsigned long long __msr_local = (unsigned long long)(msr_id);           \
	unsigned long long __tmp = 0;                                             \
	unsigned char __cf = 0, __zf = 0;                                         \
	__asm__ __volatile__(                                                      \
		"vmread %3, %0\n\t"                                                 \
		"setc %b1\n\t"                                                      \
		"setz %b2\n\t"                                                      \
		: "=r"(__tmp), "=q"(__cf), "=q"(__zf)                             \
		: "r"(__msr_local)                                                   \
		: "cc"                                                               \
	);                                                                        \
	(__zf ? 1 : (__cf ? 2 : (*(value) = __tmp, 0)));                           \
})
#define __vmx_vmwrite(msr_id, value) ({                                      \
	unsigned long long __msr_local = (unsigned long long)(msr_id);           \
	unsigned long long __val_local = (unsigned long long)(value);             \
	unsigned char __cf = 0, __zf = 0;                                         \
	__asm__ __volatile__(                                                     \
		"vmwrite %2, %3\n\t"                                               \
		"setc %b0\n\t"                                                     \
		"setz %b1\n\t"                                                     \
		: "=q"(__cf), "=q"(__zf)                                           \
		: "r"(__val_local), "r"(__msr_local)                              \
		: "cc"                                                                \
	);                                                                        \
	(__zf ? 1 : (__cf ? 2 : 0));                                              \
})
#define __cpuidex(values, eax_in, ecx_in) ({                              \
	unsigned int __eax_local = (unsigned int)(eax_in);                     \
	unsigned int __ecx_local = (unsigned int)(ecx_in);                     \
	unsigned int __a, __b, __c, __d;                                       \
	__asm__ __volatile__(                                                   \
		"cpuid"                                                           \
		: "=a"(__a), "=b"(__b), "=c"(__c), "=d"(__d)                 \
		: "a"(__eax_local), "c"(__ecx_local)                            \
	);                                                                      \
	(values)[0] = (int)__a;                                                 \
	(values)[1] = (int)__b;                                                 \
	(values)[2] = (int)__c;                                                 \
	(values)[3] = (int)__d;                                                 \
})
#define __wbinvd() __asm__ __volatile__("wbinvd" ::: "memory")
#define __invept(type_in, descriptor_in) ({                                 \
	unsigned long __type_local = (unsigned long)(type_in);                  \
	void* __desc_local = (void*)(descriptor_in);                            \
	unsigned char __cf = 0, __zf = 0;                                       \
	__asm__ __volatile__(                                                    \
		"mov %%rdi, %%rcx\n\t"                                            \
		"mov %%rsi, %%rdx\n\t"                                            \
		".byte 0x66,0x0F,0x38,0x80,0x0A\n\t"                                \
		"setc %b0\n\t"                                                    \
		"setz %b1\n\t"                                                    \
		: "=q"(__cf), "=q"(__zf)                                          \
		: "D"(__type_local), "S"(__desc_local)                            \
		: "rcx", "rdx", "memory", "cc"                                \
	);                                                                        \
	(__zf ? 1 : (__cf ? 2 : 0));                                              \
})

#else

#error Unsupported compiler type
long _InterlockedCompareExchange(long volatile* _Destination, long _Exchange, long _Comparand);
void _mm_pause(void);
unsigned char __vmx_vmread(unsigned long long msr_id, unsigned long long* value);
unsigned char __vmx_vmwrite(unsigned long long msr_id, unsigned long long value);
void __cpuidex(int[4], int eax, int ecx);
void __wbinvd(void);
char __invept(unsigned long type, void* descriptor);
#endif // _MSC_VER

#include "vmhandle.h"
#include "vmcall_def.h"

#ifndef NULL
#define NULL 0
#endif // !NULL
#define PA_NULL 0xFFFFFFFFFFFFFFFFULL

typedef CR3 NPT_CR3;
typedef PTE NPT_PTE;

typedef union
{
	unsigned long long flags;
	struct
	{
		unsigned long long MemoryType : 3;
		unsigned long long PageWalkLength : 3;
		unsigned long long EnableAccessAndDirtyFlags : 1;
		unsigned long long Reserved1 : 5;
		unsigned long long PageFrameNumber : 36;
	};
} EPT_POINTER;
typedef union
{
	unsigned long long flags;
	struct
	{
		unsigned long long ReadAccess : 1;
		unsigned long long WriteAccess : 1;
		unsigned long long ExecuteAccess : 1;
		unsigned long long MemoryType : 3;
		unsigned long long IgnorePat : 1;
		unsigned long long LargePage : 1;
		unsigned long long Accessed : 1;
		unsigned long long Dirty : 1;
		unsigned long long UserModeExecute : 1;
		unsigned long long Reserved1 : 1;
		unsigned long long PageFrameNumber : 36;
		unsigned long long Reserved2 : 15;
		unsigned long long SuppressVe : 1;
	};
} __EPT_PTE;

typedef struct
{
	unsigned char Activate : 1;
	unsigned char ept_invalidate : 1;
	unsigned char reserved1 : 6;
	unsigned char reserved2;
	short PageHookUsedIndex;
	volatile long EptLock;
	union
	{
		struct
		{
			EPT_POINTER EptPointer;
		} INTEL;
		struct
		{
			NPT_CR3 NCr3;
			VMCB* pVMCB;
		}AMD;
	};
	unsigned long long PageHookUsedRipPfn;
} CPU_DATA;

typedef union
{
	unsigned long long flags;
	unsigned char access : 3;
	struct
	{
		unsigned long long data_read : 1;
		unsigned long long data_write : 1;
		unsigned long long data_execute : 1;
		unsigned long long entry_read : 1;
		unsigned long long entry_write : 1;
		unsigned long long entry_execute : 1;
		unsigned long long entry_execute_for_user_mode : 1;
		unsigned long long valid_guest_linear_address : 1;
		unsigned long long ept_translated_access : 1;
		unsigned long long user_mode_linear_address : 1;
		unsigned long long readable_writable_page : 1;
		unsigned long long execute_disable_page : 1;
		unsigned long long nmi_unblocking : 1;
	};
}EXIT_QUALIFICATION_EPT_VIOLATION;
typedef union
{
	unsigned long long flags;
	struct
	{
		unsigned long long valid : 1;                   //0 if not present
		unsigned long long write : 1;                   //1 if it was a write access
		unsigned long long user : 1;                    //1 if it was a usermode execution
		unsigned long long reserved : 1;                //1 if a reserved bit was set
		unsigned long long execute : 1;                 //1 if it was a code fetch
		unsigned long long reserved2 : 27;              //[5:31]
		unsigned long long guest_physical_address : 1;    //32:guest final addr ess
		unsigned long long guest_page_tables : 1;         //33:guest page table
	};
} EXITINFO1_IOIO_NPT_VIOLATION;

typedef struct
{
	unsigned long long Pfn;
	unsigned long long OriginalPfn;
	unsigned long long ExecutePfn;
} PAGE_HOOK;

static CPU_DATA _CpuDataList[0x100] = { 0 };
static unsigned int PassWordEcx = DEFAULT_VMCALL_PASSWORD_ECX;
static unsigned int PassWordEdx = DEFAULT_VMCALL_PASSWORD_EDX;
static PAGE_HOOK _PageHookList[0x100] = { 0 };
static volatile long _PageHookListLock = 0;


static void SpinLock(volatile long* pLock)
{
	while (_InterlockedCompareExchange(pLock, 1, 0))
	{
		_mm_pause();
	}
}
static void SpinUnLock(volatile long* pLock)
{
	*pLock = 0;
}

static void* MemoryCopy(void* dst, const void* src, unsigned long long n)
{
	if (dst == NULL || src == NULL || n <= 0)
		return NULL;

	char *pdst = (char *)dst;
	char *psrc = (char *)src;

	if (pdst > psrc && pdst < psrc + n) {
		pdst = pdst + n - 1;
		psrc = psrc + n - 1;
		while (n--)
			*pdst-- = *psrc--;
	} else {
		while (n--)
			*pdst++ = *psrc++;
	}
	return dst;
}

// Check if the CPU is AMD
static int IsAmd()
{
	static int type = 0;
	if (type == 0)
	{
		unsigned int values[4] = { 0 };
		__cpuidex(values, 0x00000001, 0);
		if ((values[1] == 'htuA') && (values[3] == 'itne') && (values[2] == 'DMAc'))
		{
			type = 1; // AMD
		}
		else
		{
			type = -1; // Intel
		}
	}
	return type > 0 ? 1 : 0;
}

static unsigned int GetCpuIndex()
{
	unsigned int values[4];
	__cpuidex(values, 0x00000001, 0);
	return values[1] >> 24;
}

static CPU_DATA* GetCurrentCpuData()
{
	return &_CpuDataList[GetCpuIndex()];
}

// 读物理内存
static int ReadPhysicalMemory(unsigned long long PhysicalAddress, void* Buffer, unsigned long long Size)
{
	if (Size == 0)
	{
		return 1;
	}
	unsigned long long byteCount;
	unsigned char* pSrc = (unsigned char*)GetVirtualForPhysical(PhysicalAddress, &byteCount);
	if (pSrc == NULL)
	{
		return 0;
	}
	//计算本次可操作的字节数
	if (Size < byteCount)byteCount = Size;
	//先尝试写后续的部分
	if (!ReadPhysicalMemory(PhysicalAddress + byteCount, (unsigned char*)Buffer, Size - byteCount))
	{
		return 0;
	}
	MemoryCopy(Buffer, pSrc, byteCount);
	return 1;
}
// 写物理内存
static int WritePhysicalMemory(unsigned long long PhysicalAddress, void* Buffer, unsigned long long Size)
{
	if (Size == 0)
	{
		return 1;
	}
	unsigned long long byteCount;
	unsigned char* pDest = (unsigned char*)GetVirtualForPhysical(PhysicalAddress, &byteCount);
	if (pDest == NULL)
	{
		return 0;
	}
	//计算本次可操作的字节数
	if (Size < byteCount)byteCount = Size;
	//先尝试写后续的部分
	if (!WritePhysicalMemory(PhysicalAddress + byteCount, (unsigned char*)Buffer, Size - byteCount))
	{
		return 0;
	}
	MemoryCopy(pDest, Buffer, byteCount);
	return 1;
}

static int GetEptEntry(CPU_DATA* pCpuData, unsigned long long GuestPhysicalAddress, void** ppPml4e, void** ppPdpte, void** ppPde, void** ppPte)
{
	int result = 0;
	if (ppPml4e)*ppPml4e = NULL;
	if (ppPdpte)*ppPdpte = NULL;
	if (ppPde)*ppPde = NULL;
	if (ppPte)*ppPte = NULL;

	if (IsAmd())
	{
		NPT_PTE* pTable, * pPte;
		do
		{
			pTable = (NPT_PTE*)GetVirtualForPhysical(pCpuData->AMD.NCr3.PageFrameNumber << 12, NULL);   //pml4_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 39) & 0x1FF);
			if (ppPml4e)*ppPml4e = pPte;
			if (!pPte->Present)break;

			pTable = (NPT_PTE*)GetVirtualForPhysical(pPte->PageFrameNumber << 12, NULL);//pdpt_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 30) & 0x1FF);
			if (ppPdpte)*ppPdpte = pPte;
			if (!pPte->Present)break;
			if (pPte->LargePage)
			{
				result = 1;
				break;
			}

			pTable = (NPT_PTE*)GetVirtualForPhysical(pPte->PageFrameNumber << 12, NULL);//pd_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 21) & 0x1FF);
			if (ppPde)*ppPde = pPte;
			if (!pPte->Present)break;
			if (pPte->LargePage)
			{
				result = 1;
				break;
			}

			pTable = (NPT_PTE*)GetVirtualForPhysical(pPte->PageFrameNumber << 12, NULL);//pt_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 12) & 0x1FF);
			if (ppPte)*ppPte = pPte;
			if (!pPte->Present)break;

			result = 1;
		} while (0);
	}
	else
	{
		__EPT_PTE* pTable, * pPte;
		do
		{
			pTable = (__EPT_PTE*)GetVirtualForPhysical(pCpuData->INTEL.EptPointer.PageFrameNumber << 12, NULL);   //pml4_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 39) & 0x1FF);
			if (ppPml4e)*ppPml4e = pPte;
			if ((pPte->flags & 0x7) == 0)break;

			pTable = (__EPT_PTE*)GetVirtualForPhysical(pPte->PageFrameNumber << 12, NULL);//pdpt_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 30) & 0x1FF);
			if (ppPdpte)*ppPdpte = pPte;
			if ((pPte->flags & 0x7) == 0)break;
			if (pPte->LargePage)
			{
				result = 1;
				break;
			}

			pTable = (__EPT_PTE*)GetVirtualForPhysical(pPte->PageFrameNumber << 12, NULL);//pd_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 21) & 0x1FF);
			if (ppPde)*ppPde = pPte;
			if ((pPte->flags & 0x7) == 0)break;
			if (pPte->LargePage)
			{
				result = 1;
				break;
			}

			pTable = (__EPT_PTE*)GetVirtualForPhysical(pPte->PageFrameNumber << 12, NULL);//pt_table
			if (pTable == NULL)break;
			pPte = pTable + ((GuestPhysicalAddress >> 12) & 0x1FF);
			if (ppPte)*ppPte = pPte;
			if ((pPte->flags & 0x7) == 0)break;

			result = 1;
		} while (0);
	}
	return result;
}

//GPA->HPA
static unsigned long long GuestPhysicalToPhysical(CPU_DATA* pCpuData, unsigned long long GuestPhysicalAddress, unsigned long long* pByteCount)
{
	//直接返回
	if (IsAmd())
	{
		NPT_PTE* pPml4e, * pPdpte, * pPde, * pPte;
		if (GetEptEntry(pCpuData, GuestPhysicalAddress, &pPml4e, &pPdpte, &pPde, &pPte))
		{
			if (pPte)
			{
				if(pByteCount)*pByteCount = 0x1000 - (GuestPhysicalAddress & 0xFFF);
				return (pPte->PageFrameNumber >> 12) | (GuestPhysicalAddress & 0xFFF);
			}
			if (pPde)
			{
				if (pByteCount)*pByteCount = 0x200000 - (GuestPhysicalAddress & 0x1FFFFF);
				return (pPde->PageFrameNumber >> 12) | (GuestPhysicalAddress & 0x1FFFFF);
			}
			if (pPdpte)
			{
				if (pByteCount)*pByteCount = 0x40000000 - (GuestPhysicalAddress & 0x3FFFFFFF);
				return (pPdpte->PageFrameNumber >> 12) | (GuestPhysicalAddress & 0x3FFFFFFF);
			}
		}
	}
	else
	{
		__EPT_PTE* pPml4e, * pPdpte, * pPde, * pPte;
		if (GetEptEntry(pCpuData, GuestPhysicalAddress, &pPml4e, &pPdpte, &pPde, &pPte))
		{
			if (pPte)
			{
				if (pByteCount)*pByteCount = 0x1000 - (GuestPhysicalAddress & 0xFFF);
				return (pPte->PageFrameNumber >> 12) | (GuestPhysicalAddress & 0xFFF);
			}
			if (pPde)
			{
				if (pByteCount)*pByteCount = 0x200000 - (GuestPhysicalAddress & 0x1FFFFF);
				return (pPde->PageFrameNumber >> 12) | (GuestPhysicalAddress & 0x1FFFFF);
			}
			if (pPdpte)
			{
				if (pByteCount)*pByteCount = 0x40000000 - (GuestPhysicalAddress & 0x3FFFFFFF);
				return (pPdpte->PageFrameNumber >> 12) | (GuestPhysicalAddress & 0x3FFFFFFF);
			}
		}
	}
	return PA_NULL;
}
static unsigned long long GuestVirtualToGuestPhysical(CPU_DATA* pCpuData, unsigned long long GuestCr3, unsigned long long GuestVirtualAddress, unsigned long long* pByteCount)
{
	CR3 cr3;
	cr3.flags = GuestCr3;
	PTE* pTable = (PTE*)GetVirtualForPhysical(GuestPhysicalToPhysical(pCpuData, cr3.PageFrameNumber << 12, NULL), NULL);   //pml4_table
	if (!pTable || !pTable[(GuestVirtualAddress >> 39) & 0x1FF].Present) //check if pml4_table is valid
	{
		return PA_NULL; //not a valid PTE entry
	}

	pTable = (PTE*)GetVirtualForPhysical(GuestPhysicalToPhysical(pCpuData, pTable[(GuestVirtualAddress >> 39) & 0x1FF].PageFrameNumber << 12, NULL), NULL);//pdpt_table
	if (!pTable || !pTable[(GuestVirtualAddress >> 30) & 0x1FF].Present) //check if pdpt_table is valid
	{
		return PA_NULL; //not a valid PTE entry
	}
	if (pTable[(GuestVirtualAddress >> 30) & 0x1FF].LargePage)
	{
		// 1GB页面
		if (pByteCount != NULL)
		{
			*pByteCount = 0x40000000 - (GuestVirtualAddress & 0x3FFFFFFF);
		}
		return (pTable[(GuestVirtualAddress >> 30) & 0x1FF].PageFrameNumber << 12) | (GuestVirtualAddress & 0x3FFFFFFF);
	}

	pTable = (PTE*)GetVirtualForPhysical(GuestPhysicalToPhysical(pCpuData, pTable[(GuestVirtualAddress >> 30) & 0x1FF].PageFrameNumber << 12, NULL), NULL);//pd_table
	if (!pTable || !pTable[(GuestVirtualAddress >> 21) & 0x1FF].Present) //check if pd_table is valid
	{
		return PA_NULL; //not a valid PTE entry
	}
	if (pTable[(GuestVirtualAddress >> 21) & 0x1FF].LargePage)
	{
		// 2MB页面
		if (pByteCount != NULL)
		{
			*pByteCount = 0x200000 - (GuestVirtualAddress & 0x1FFFFF);
		}
		return (pTable[(GuestVirtualAddress >> 21) & 0x1FF].PageFrameNumber << 12) | (GuestVirtualAddress & 0x1FFFFF);
	}

	pTable = (PTE*)GetVirtualForPhysical(GuestPhysicalToPhysical(pCpuData, pTable[(GuestVirtualAddress >> 21) & 0x1FF].PageFrameNumber << 12, NULL), NULL);//pt_table
	if (!pTable || !pTable[(GuestVirtualAddress >> 12) & 0x1FF].Present) //check if pt_table is valid
	{
		return PA_NULL; //not a valid PTE entry
	}
	return (pTable[(GuestVirtualAddress >> 12) & 0x1FF].PageFrameNumber << 12) | (GuestVirtualAddress & 0xFFF);
}
static int ReadGuestPhysicalMemory(CPU_DATA* pCpuData, unsigned long long GuestPhysicalAddress, void* Buffer, unsigned long long Size)
{
	if (Size == 0)
	{
		return 1;
	}
	unsigned long long byteCount, byteCount2;
	unsigned long long PhysicalAddress = GuestPhysicalToPhysical(pCpuData, GuestPhysicalAddress, &byteCount);
	if (PhysicalAddress == PA_NULL)
	{
		return 0;
	}
	unsigned char* pSrc = (unsigned char*)GetVirtualForPhysical(PhysicalAddress, &byteCount2);
	if (pSrc == NULL)
	{
		return 0;
	}
	//计算本次可操作的字节数
	if (byteCount2 < byteCount)byteCount = byteCount2;
	if (Size < byteCount)byteCount = Size;
	//先尝试写后续的部分
	if (!ReadGuestPhysicalMemory(pCpuData, GuestPhysicalAddress + byteCount, (unsigned char*)Buffer + byteCount, Size - byteCount))
	{
		return 0;
	}
	MemoryCopy(Buffer, pSrc, byteCount);
	return 1;
}
static int WriteGuestPhysicalMemory(CPU_DATA* pCpuData, unsigned long long GuestPhysicalAddress, void* Buffer, unsigned long long Size)
{
	if (Size == 0)
	{
		return 1;
	}
	unsigned long long byteCount, byteCount2;
	unsigned long long PhysicalAddress = GuestPhysicalToPhysical(pCpuData, GuestPhysicalAddress, &byteCount);
	if (PhysicalAddress == PA_NULL)
	{
		return 0;
	}
	unsigned char* pDest = (unsigned char*)GetVirtualForPhysical(PhysicalAddress, &byteCount2);
	if (pDest == NULL)
	{
		return 0;
	}
	//计算本次可操作的字节数
	if (byteCount2 < byteCount)byteCount = byteCount2;
	if (Size < byteCount)byteCount = Size;
	//先尝试写后续的部分
	if (!WriteGuestPhysicalMemory(pCpuData, GuestPhysicalAddress + byteCount, (unsigned char*)Buffer + byteCount, Size - byteCount))
	{
		return 0;
	}
	MemoryCopy(pDest, Buffer, byteCount);
	return 1;
}

static int ReadGuestVirtualMemory(CPU_DATA* pCpuData, unsigned long long GuestCr3, unsigned long long GuestVirtualAddress, void* Buffer, unsigned long long Size)
{
	if (Size == 0)
	{
		return 1;
	}
	unsigned long long byteCount, byteCount2, byteCount3;
	unsigned long long GuestPhysicalAddress = GuestVirtualToGuestPhysical(pCpuData, GuestCr3, GuestVirtualAddress, &byteCount);
	if (GuestPhysicalAddress == PA_NULL)
	{
		return 0;
	}
	unsigned long long PhysicalAddress = GuestPhysicalToPhysical(pCpuData, GuestPhysicalAddress, &byteCount2);
	if (PhysicalAddress == PA_NULL)
	{
		return 0;
	}
	void* VirtualAddress = GetVirtualForPhysical(PhysicalAddress, &byteCount3);
	if (VirtualAddress == NULL)
	{
		return 0;
	}
	//计算本次可操作的字节数
	if (byteCount2 < byteCount)byteCount = byteCount2;
	if (byteCount3 < byteCount)byteCount = byteCount3;
	if (Size < byteCount)byteCount = Size;
	//先尝试写后续的部分
	if (!ReadGuestVirtualMemory(pCpuData, GuestCr3, GuestVirtualAddress + byteCount, (unsigned char*)Buffer + byteCount, Size - byteCount))
	{
		return 0;
	}
	MemoryCopy(Buffer, VirtualAddress, byteCount);
	return 1;
}
static int WriteGuestVirtualMemory(CPU_DATA* pCpuData, unsigned long long GuestCr3, unsigned long long GuestVirtualAddress, void* Buffer, unsigned long long Size)
{
	if (Size == 0)
	{
		return 1;
	}
	unsigned long long byteCount, byteCount2, byteCount3;
	unsigned long long GuestPhysicalAddress = GuestVirtualToGuestPhysical(pCpuData, GuestCr3, GuestVirtualAddress, &byteCount);
	if (GuestPhysicalAddress == PA_NULL)
	{
		return 0;
	}
	unsigned long long PhysicalAddress = GuestPhysicalToPhysical(pCpuData, GuestPhysicalAddress, &byteCount2);
	if (PhysicalAddress == PA_NULL)
	{
		return 0;
	}
	void* VirtualAddress = GetVirtualForPhysical(PhysicalAddress, &byteCount3);
	if (VirtualAddress == NULL)
	{
		return 0;
	}
	//计算本次可操作的字节数
	if (byteCount2 < byteCount)byteCount = byteCount2;
	if (byteCount3 < byteCount)byteCount = byteCount3;
	if (Size < byteCount)byteCount = Size;
	//先尝试写后续的部分
	if (!ReadGuestVirtualMemory(pCpuData, GuestCr3, GuestVirtualAddress + byteCount, (unsigned char*)Buffer + byteCount, Size - byteCount))
	{
		return 0;
	}
	MemoryCopy(VirtualAddress, Buffer, byteCount);
	return 1;
}

static int OnExecuteVmcall(CPU_DATA* pCpuData, unsigned int Command, void* Data, unsigned int DataSize, int* pWriteBack)
{
	unsigned int result = 0;
	*pWriteBack = 0;
	switch (Command)
	{
	case VMCALL_EXIST:
	{

		return 1; //存在
	}
	case VMCALL_CHANGE_PASSWORD:
	{
		if (DataSize >= sizeof(CHANGE_PASSWORD_DATA))
		{
			CHANGE_PASSWORD_DATA* pChangePasswordData = (CHANGE_PASSWORD_DATA*)Data;
			PassWordEcx = pChangePasswordData->NewPassWordEcx;
			PassWordEdx = pChangePasswordData->NewPassWordEdx;
			result = 1;
		}
	}
	break;
	case VMCALL_SET_PAGE_EXECUTE_PFN:
	{
		if (DataSize >= sizeof(SET_PAGE_EXECUTE_PFN_DATA))
		{
			SET_PAGE_EXECUTE_PFN_DATA* pSetPageExecutePfnData = (SET_PAGE_EXECUTE_PFN_DATA*)Data;
			SpinLock(&_PageHookListLock);
			{
				unsigned long long Pfn = pSetPageExecutePfnData->Pfn;
				unsigned long long ExecutePfn = pSetPageExecutePfnData->ExecutePfn;
				if (Pfn)
				{
					int hook_index = -1;
					if (ExecutePfn == 0)
					{
						//remove hook
						for (int i = 0; i < sizeof(_PageHookList) / sizeof(_PageHookList[0]); i++)
						{
							if (_PageHookList[i].Pfn == Pfn)
							{
								hook_index = i;

								_PageHookList[i].Pfn = 0;
								_PageHookList[i].ExecutePfn = 0;
								break;
							}
						}
						result = 1;
					}
					else
					{
						//add hook
						for (int i = 0; i < sizeof(_PageHookList) / sizeof(_PageHookList[0]); i++)
						{
							if (_PageHookList[i].Pfn == 0)
							{
								hook_index = i;

								_PageHookList[i].Pfn = Pfn;
								_PageHookList[i].ExecutePfn = ExecutePfn;
								_PageHookList[i].OriginalPfn = 0;
								result = 1;
								break;
							}
						}
					}
					if (hook_index >= 0)
					{
						//循环所有核心
						for (int cpu_index = 0; cpu_index < sizeof(_CpuDataList) / sizeof(_CpuDataList[0]); cpu_index++)
						{
							CPU_DATA* pTmpCpuData = _CpuDataList + cpu_index;
							if (!pTmpCpuData->Activate)
								continue;
							if (IsAmd())
							{
								NPT_PTE* pPml4e, * pPdpte, * pPde, * pPte;
								if (GetEptEntry(pTmpCpuData, Pfn << 12, &pPml4e, &pPdpte, &pPde, &pPte))
								{
									if (!pPte)
									{
										//分割2MB页面到4KB
										if (!pPde)
										{
											//分割1GB页面到2MB
											unsigned long long PdeTablePA = AllocPhysicalPage();
											NPT_PTE* PdeTable = (NPT_PTE*)GetVirtualForPhysical(PdeTablePA, NULL);
											for(unsigned long long pd_index =0;pd_index<512;pd_index++)
											{
												PdeTable[pd_index] = *pPdpte;
												PdeTable[pd_index].LargePage = 1;
												PdeTable[pd_index].PageFrameNumber = pPdpte->PageFrameNumber + (pd_index<<9);
											}
											pPdpte->PageFrameNumber = PdeTablePA>>12;
											pPdpte->LargePage = 0;

											pPde = PdeTable + ((Pfn>>9)&0x1FF);
										}

										//分割2MB页面到4KB
										unsigned long long PteTablePA = AllocPhysicalPage();
										NPT_PTE* PteTable = (NPT_PTE*)GetVirtualForPhysical(PteTablePA, NULL);
										for(unsigned long long pt_index =0;pt_index<512;pt_index++)
										{
											PteTable[pt_index] = *pPde;
											PteTable[pt_index].LargePage = 0;
											PteTable[pt_index].PageFrameNumber = pPde->PageFrameNumber + pt_index;
										}
										pPdpte->PageFrameNumber = PteTablePA>>12;
										pPdpte->LargePage = 0;

										pPte = PteTable + ((Pfn>>0)&0x1FF);
									}
									if (_PageHookList[hook_index].ExecutePfn == 0)
									{
										//还原HOOK
										pPte->PageFrameNumber = _PageHookList[hook_index].OriginalPfn;
										pPte->ExecuteDisable = 0;
									}
									else
									{
										//开启HOOK
										if (cpu_index == 0)
										{
											_PageHookList[hook_index].OriginalPfn = pPte->PageFrameNumber;
										}
										pPte->PageFrameNumber = _PageHookList[hook_index].ExecutePfn;
										pPte->ExecuteDisable = 1;
									}
									__wbinvd();
									pTmpCpuData->ept_invalidate = 1;
								}
							}
							else
							{
								__EPT_PTE* pPml4e, * pPdpte, * pPde, * pPte;
								if (GetEptEntry(pTmpCpuData, Pfn << 12, &pPml4e, &pPdpte, &pPde, &pPte))
								{
									if (!pPte)
									{
										//分割2MB页面到4KB
										if (!pPde)
										{
											//分割1GB页面到2MB
											unsigned long long PdeTablePA = AllocPhysicalPage();
											__EPT_PTE* PdeTable = (__EPT_PTE*)GetVirtualForPhysical(PdeTablePA, NULL);
											for (unsigned long long pd_index = 0; pd_index < 512; pd_index++)
											{
												PdeTable[pd_index] = *pPdpte;
												PdeTable[pd_index].LargePage = 1;
												PdeTable[pd_index].PageFrameNumber = pPdpte->PageFrameNumber + (pd_index << 9);
											}
											pPdpte->PageFrameNumber = PdeTablePA >> 12;
											pPdpte->LargePage = 0;

											pPde = PdeTable + ((Pfn >> 9) & 0x1FF);
										}

										//分割2MB页面到4KB
										unsigned long long PteTablePA = AllocPhysicalPage();
										__EPT_PTE* PteTable = (__EPT_PTE*)GetVirtualForPhysical(PteTablePA, NULL);
										for (unsigned long long pt_index = 0; pt_index < 512; pt_index++)
										{
											PteTable[pt_index] = *pPde;
											PteTable[pt_index].LargePage = 0;
											PteTable[pt_index].PageFrameNumber = pPde->PageFrameNumber + pt_index;
										}
										pPdpte->PageFrameNumber = PteTablePA >> 12;
										pPdpte->LargePage = 0;

										pPte = PteTable + ((Pfn >> 0) & 0x1FF);
									}
									if (_PageHookList[hook_index].ExecutePfn == 0)
									{
										//还原HOOK
										pPte->PageFrameNumber = _PageHookList[hook_index].OriginalPfn;
										pPte->ExecuteAccess = 1;
									}
									else
									{
										//开启HOOK
										if (cpu_index == 0)
										{
											_PageHookList[hook_index].OriginalPfn = pPte->PageFrameNumber;
										}
										pPte->PageFrameNumber = _PageHookList[hook_index].ExecutePfn;
										pPte->ExecuteAccess = 0;
									}
									__wbinvd();
									pTmpCpuData->ept_invalidate = 1;
								}
							}
						}
					}
				}
			}
			SpinUnLock(&_PageHookListLock);
		}
	}
	break;
	case VMCALL_GET_PAGE_EXECUTE_PFN:
	{
		if (DataSize >= sizeof(GET_PAGE_EXECUTE_PFN_DATA))
		{
			GET_PAGE_EXECUTE_PFN_DATA* pGetPageExecutePfnData = (GET_PAGE_EXECUTE_PFN_DATA*)Data;
			SpinLock(&_PageHookListLock);
			{
				unsigned long long Pfn = pGetPageExecutePfnData->Pfn;
				if (Pfn)
				{
					for (int i = 0; i < sizeof(_PageHookList) / sizeof(_PageHookList[0]); i++)
					{
						if (_PageHookList[i].Pfn == Pfn)
						{
							pGetPageExecutePfnData->ExecutePfn = _PageHookList[i].ExecutePfn;
							*pWriteBack = 1;
							result = 1;
							break;
						}
					}
				}
			}
			SpinUnLock(&_PageHookListLock);
		}
	}
	break;

	default:
		break;
	}

	return result;
}

int VMXExitBefore(CONTEXT* Context)
{
#ifdef DEBUG
	//测试内存读写是否正常
	unsigned long long* va = GetVirtualForPhysical(0x1000000, NULL);
	*va = *va;
#endif

	int result = 0;
	CPU_DATA* pCpuData = GetCurrentCpuData();
	//初始化
	if (!pCpuData->Activate)
	{
		pCpuData->ept_invalidate = 0;
		pCpuData->PageHookUsedIndex = -1;
		__vmx_vmread(0x201A, &pCpuData->INTEL.EptPointer.flags);
		pCpuData->PageHookUsedRipPfn = 0;

		pCpuData->Activate = 1;
	}

	unsigned long long GuestRip;		//GuestRip
	__vmx_vmread(0x681E, &GuestRip);
	unsigned long long ExitReason;
	__vmx_vmread(0x4402, &ExitReason);	//VM_EXIT_REASON
	switch (ExitReason)
	{
	case 48:  //EPTViolation
	{
		EXIT_QUALIFICATION_EPT_VIOLATION ExitQualification;
		__vmx_vmread(0x6400, &ExitQualification.flags);	//VM_EXIT_QUALIFICATION
		unsigned long long PhysicalAddress;
		__vmx_vmread(0x2400, &PhysicalAddress);	//VM_EXIT_GUEST_PHYSICAL_ADDRESS
		if (ExitQualification.data_execute)
		{
			SpinLock(&_PageHookListLock);
			{
				for (unsigned short index = 0; index < sizeof(_PageHookList) / sizeof(_PageHookList[0]); index++)
				{
					if (_PageHookList[index].Pfn == (PhysicalAddress >> 12))
					{
						__EPT_PTE* pPte;
						GetEptEntry(pCpuData, PhysicalAddress, NULL, NULL, NULL, &pPte);
						if (pPte)
						{
							pPte->PageFrameNumber = _PageHookList[index].OriginalPfn;
							pPte->ExecuteAccess = 1;

							pCpuData->ept_invalidate = 1;

							pCpuData->PageHookUsedIndex = index;
							pCpuData->PageHookUsedRipPfn = GuestRip >> 12;
							__wbinvd();

							result = 1;
							break;
						}
					}
				}
			}
			SpinUnLock(&_PageHookListLock);
		}
	}
	break;
	case 10:  //ExecuteCpuid
	case 18:  //ExecuteVmcall
	{
		char Buffer[0x100];
		if (Context->Rcx == PassWordEcx && Context->Rdx == PassWordEdx
			&& Context->R8 && Context->R9 >= sizeof(VMCALL_HEADER) && Context->R9 <= sizeof(Buffer))
		{
			unsigned long long GuestCr3;
			__vmx_vmread(0x6802, &GuestCr3);	//GuestCr3
			if (ReadGuestVirtualMemory(pCpuData, GuestCr3, Context->R8, (unsigned char*)Buffer, Context->R9))
			{
				VMCALL_HEADER* pHeader = (VMCALL_HEADER*)Buffer;
				int WriteBack = 0;
				result = OnExecuteVmcall(pCpuData, pHeader->Command, Buffer + sizeof(VMCALL_HEADER), (unsigned long)Context->R9 - sizeof(VMCALL_HEADER), &WriteBack);
				if (result)
				{
					Context->Rax = 0x00000000;
					__vmx_vmwrite(0x681E, GuestRip + 3);	//skip vmcall
					//回写数据
					if (WriteBack)
					{
						WriteGuestVirtualMemory(pCpuData, GuestCr3, Context->R8, (unsigned char*)Buffer, Context->R9);
					}
				}
			}
		}
	}
	break;
	default:
		break;
	}
	return result;
}
void VMXExitAfter()
{
	CPU_DATA* pCpuData = GetCurrentCpuData();
	if (!pCpuData->Activate)
		return;

	unsigned long long GuestRip;
	__vmx_vmread(0x681E, &GuestRip);
	//恢复HOOK
	if (pCpuData->PageHookUsedIndex >= 0)
	{
		//如果已离开HOOK页面，则恢复原始EPT
		if (GuestRip >> 12 != pCpuData->PageHookUsedRipPfn)
		{
			SpinLock(&_PageHookListLock);
			{
				unsigned long long Pfn = _PageHookList[pCpuData->PageHookUsedIndex].Pfn;
				unsigned long long ExecutePfn = _PageHookList[pCpuData->PageHookUsedIndex].ExecutePfn;
				if (Pfn)
				{
					//restore EPT entry
					__EPT_PTE* pPte;
					if (GetEptEntry(pCpuData, Pfn << 12, NULL, NULL, NULL, &pPte) && pPte)
					{
						//还原为OriginalPfn，并将它设置为不可执行。这样当下一次该页面需要执行时又会触发VMExit事件
						pPte->ExecuteAccess = 0;
						pPte->PageFrameNumber = ExecutePfn;

						//flush TLB
						pCpuData->ept_invalidate = 1;

						pCpuData->PageHookUsedIndex = -1;
						pCpuData->PageHookUsedRipPfn = 0;
						__wbinvd();
					}
				}
			}
			SpinUnLock(&_PageHookListLock);
		}
	}
	if (pCpuData->ept_invalidate)
	{
		pCpuData->ept_invalidate = 0;

		struct
		{
			unsigned long long EPTPointer;
			unsigned long long Zero;
		} eptd;
		eptd.Zero = 0;
		eptd.EPTPointer = pCpuData->INTEL.EptPointer.flags;
		__invept(1, &eptd); //single-context invalidation
		//Invept(2,&eptd); //all-context invalidation
	}
}

int SVMExitBefore(VMCB* pVMCB, CONTEXT* Context)
{
#ifdef DEBUG
	//测试内存读写是否正常
	unsigned long long* va = GetVirtualForPhysical(0x1000000, NULL);
	*va = *va;
#endif
	int result = 0;
	CPU_DATA* pCpuData = GetCurrentCpuData();
	//初始化
	if (!pCpuData->Activate)
	{
		pCpuData->ept_invalidate = 0;
		pCpuData->PageHookUsedIndex = -1;
		pCpuData->AMD.NCr3.flags = pVMCB->NCr3;
		pCpuData->AMD.pVMCB = pVMCB;
		pCpuData->PageHookUsedRipPfn = 0;

		pCpuData->Activate = 1;
	}

	unsigned long long Rip = pVMCB->Rip;
	switch (pVMCB->ExitCode)
	{
	case 0x400:     //NptViolation
	{
		EXITINFO1_IOIO_NPT_VIOLATION ExitInfo1;
		ExitInfo1.flags = pVMCB->ExitInfo1;
		unsigned long long PhysicalAddress = pVMCB->ExitInfo2; //VM_EXIT_GUEST_PHYSICAL_ADDRESS
		if (ExitInfo1.execute)
		{
			SpinLock(&_PageHookListLock);
			{
				for (unsigned short index = 0; index < sizeof(_PageHookList) / sizeof(_PageHookList[0]); index++)
				{
					if (_PageHookList[index].Pfn == (PhysicalAddress >> 12))
					{
						NPT_PTE* pPte;
						GetEptEntry(pCpuData, PhysicalAddress, NULL, NULL, NULL, &pPte);
						if (pPte)
						{
							pPte->PageFrameNumber = _PageHookList[index].OriginalPfn;
							pPte->ExecuteDisable = 0;

							pCpuData->ept_invalidate = 1;

							pCpuData->PageHookUsedIndex = index;
							pCpuData->PageHookUsedRipPfn = Rip >> 12;
							__wbinvd();

							result = 1;
							break;
						}
					}
				}
			}
			SpinUnLock(&_PageHookListLock);
		}
	}
	break;
	case 0x72:  	//ExecuteCpuid
	case 0x81:      //ExecuteVmcall
	{
		char Buffer[0x100];
		if (Context->Rcx == PassWordEcx && Context->Rdx == PassWordEdx
			&& Context->R8 && Context->R9 >= sizeof(VMCALL_HEADER) && Context->R9 <= sizeof(Buffer))
		{
			if (ReadGuestVirtualMemory(pCpuData, pVMCB->Cr3, Context->R8, (unsigned char*)Buffer, Context->R9))
			{
				VMCALL_HEADER* pHeader = (VMCALL_HEADER*)Buffer;
				int WriteBack = 0;
				result = OnExecuteVmcall(pCpuData, pHeader->Command, Buffer + sizeof(VMCALL_HEADER), (unsigned long)Context->R9 - sizeof(VMCALL_HEADER), &WriteBack);
				if (result)
				{
					Context->Rax = 0x00000000;
					pVMCB->Rip += 3; //skip vmcall
					if (WriteBack)
					{
						//回写数据
						WriteGuestVirtualMemory(pCpuData, pVMCB->Cr3, Context->R8, (unsigned char*)Buffer, Context->R9);
					}
				}
			}
		}
	}
	break;
	default:
		break;
	}
	return result;
}
void SVMExitAfter()
{
	CPU_DATA* pCpuData = GetCurrentCpuData();
	if (!pCpuData->Activate)
		return;

	unsigned long long Rip = pCpuData->AMD.pVMCB->Rip;
	//恢复HOOK
	if (pCpuData->PageHookUsedIndex >= 0)
	{
		//如果已离开HOOK页面，则恢复原始EPT
		if (Rip >> 12 != pCpuData->PageHookUsedRipPfn)
		{
			SpinLock(&_PageHookListLock);
			{
				unsigned long long Pfn = _PageHookList[pCpuData->PageHookUsedIndex].Pfn;
				unsigned long long ExecutePfn = _PageHookList[pCpuData->PageHookUsedIndex].ExecutePfn;
				if (Pfn)
				{
					//restore EPT entry
					NPT_PTE* pPte;
					if (GetEptEntry(pCpuData, Pfn << 12, NULL, NULL, NULL, &pPte) && pPte)
					{
						pPte->ExecuteDisable = 1;
						pPte->PageFrameNumber = ExecutePfn;
						//flush TLB
						pCpuData->ept_invalidate = 1;

						pCpuData->PageHookUsedIndex = -1;
						pCpuData->PageHookUsedRipPfn = 0;
						__wbinvd();
					}
				}
			}
			SpinUnLock(&_PageHookListLock);
		}
	}
	if (pCpuData->ept_invalidate)
	{
		pCpuData->ept_invalidate = 0;

		pCpuData->AMD.pVMCB->VmcbCleanBits.NestedPaging = 0;
		pCpuData->AMD.pVMCB->TlbControl = 1;
	}
}