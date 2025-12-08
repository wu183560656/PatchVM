#pragma once

#include "vmcall_def.h"
#include <intrin.h>

#pragma warning(push)
#pragma warning(disable:4996)
class VM
{
private:
	struct static_data_t
	{
		unsigned int password_ecx = DEFAULT_VMCALL_PASSWORD_ECX;
		unsigned int password_edx = DEFAULT_VMCALL_PASSWORD_EDX;
	};
	static inline static_data_t& static_data() noexcept
	{
		static static_data_t data_{};
		return data_;
	}
	inline static __int64 vmcall(void* data, unsigned int data_size) noexcept
	{
#pragma code_seg(".text")
		__declspec(allocate(".text")) const static unsigned char _vmm_call[] = { 0x4C, 0x89, 0xC0, 0x0F, 0x01, 0xD9, 0xC3, 0xCC };
		__declspec(allocate(".text")) const static unsigned char _vmx_call[] = { 0x4C, 0x89, 0xC0, 0x0F, 0x01, 0xC1, 0xC3, 0xCC };
		__declspec(allocate(".text")) const static unsigned char _cpuid[] = { 0x0F, 0xA2, 0xC3, 0xCC };
#pragma code_seg()
		return ((__int64(*)(unsigned int, unsigned int, void*, unsigned int))&_cpuid)(static_data().password_ecx, static_data().password_edx, data, data_size);
		/*
		int type = 0;
		if (!type)
		{
			int cpuid_value[4] = { 0 };
			__cpuid(cpuid_value, 0);
			type = ((cpuid_value[1] == 'htuA') && (cpuid_value[3] == 'itne') && (cpuid_value[2] == 'DMAc')) ? 2 : 1;
		}
		__int64(*fun)(unsigned int, unsigned int, void*, unsigned int) = (decltype(fun))(type == 2 ? &_vmm_call : &_vmx_call);
		__try
		{
			return fun(static_data().password_ecx, static_data().password_edx, data, data_size);
		}
		__except (1)
		{
			return -1;
		}
		*/
	}
public:
	inline static bool Initialize(unsigned int password_ecx, unsigned int password_edx) noexcept
	{
		static_data().password_ecx = password_ecx;
		static_data().password_edx = password_edx;
		return Exist();
	}
	inline static bool Exist() noexcept
	{
		VMCALL_HEADER Data{ 0 };
		return vmcall(&Data, sizeof(Data)) == static_data().password_ecx + static_data().password_edx;
	}
	inline static int EptHookSplit(unsigned __int64 Pfn, unsigned __int64 ExecutePfn) noexcept
	{
		struct
		{
			VMCALL_HEADER Header;
			SET_PAGE_EXECUTE_PFN_DATA Param;
		} Data{};
		Data.Header.Command = VMCALL_SET_PAGE_EXECUTE_PFN;
		Data.Param.Pfn = Pfn;
		Data.Param.ExecutePfn = ExecutePfn;
		return (int)vmcall(&Data, sizeof(Data));
	}
	inline static int EptHookMerge(unsigned __int64 Pfn) noexcept
	{
		struct
		{
			VMCALL_HEADER Header;
			SET_PAGE_EXECUTE_PFN_DATA Param;
		} Data{};
		Data.Header.Command = VMCALL_SET_PAGE_EXECUTE_PFN;
		Data.Param.Pfn = Pfn;
		Data.Param.ExecutePfn = 0;
		return (int)vmcall(&Data, sizeof(Data));
	}
	inline static __int64 EptHookQuery(unsigned __int64 Pfn) noexcept
	{
		struct
		{
			VMCALL_HEADER Header;
			GET_PAGE_EXECUTE_PFN_DATA Param;
		} Data{};
		Data.Header.Command = VMCALL_GET_PAGE_EXECUTE_PFN;
		Data.Param.Pfn = Pfn;
		Data.Param.ExecutePfn = 0;
		if (vmcall(&Data,sizeof(Data)))
		{
			return Data.Param.ExecutePfn;
		}
		return 0;
	}
#ifdef WINNT
	inline static bool WriteMemory(PVOID _Des, PVOID _Src, size_t _Size)
	{
		if (!_Size)
		{
			return true;
		}
		//获取执行页面
		PULONG64 DesPageBase = (PULONG64)((ULONG64)_Des & 0xFFFFFFFFFFFFF000ULL);
		ULONG64 DesPfn = MmGetPhysicalAddress(DesPageBase).QuadPart >> 12;
		ULONG64 DesExecutePfn = VM::EptHookQuery(DesPfn);
		if (!DesExecutePfn)
		{
			//如果执行页面不存在，申请一个页面并调用DBVM分离
			PVOID DesExecutePageBase = ExAllocatePool(NonPagedPool, PAGE_SIZE);
			if (DesExecutePageBase)
			{
				//两个页面初始化内容相同
				RtlCopyMemory(DesExecutePageBase, DesPageBase, PAGE_SIZE);
				DesExecutePfn = MmGetPhysicalAddress(DesExecutePageBase).QuadPart >> 12;
				if (!DesExecutePfn || VM::EptHookSplit(DesPfn, DesExecutePfn))
				{
					ExFreePool(DesExecutePageBase);
				}
			}
		}
		if (!DesExecutePfn)
		{
			return false;
		}
		PHYSICAL_ADDRESS PhysicalAddress;
		PhysicalAddress.QuadPart = DesExecutePfn << 12;
		PUCHAR DesExecutePageBase = (PUCHAR)MmGetVirtualForPhysical(PhysicalAddress);
		if (!DesExecutePageBase)
		{
			return false;
		}
		SIZE_T Offset = (ULONG64)_Des & 0xFFF;
		SIZE_T WriteSize = PAGE_SIZE - Offset;
		WriteSize = _Size < WriteSize ? _Size : WriteSize;
		bool result = WriteMemory((PUCHAR)_Des + WriteSize, (PUCHAR)_Src + WriteSize, _Size - WriteSize);
		if (result)
		{
			RtlCopyMemory(DesExecutePageBase + Offset, _Src, WriteSize);
			//尝试合并页
			bool equal = true;
			for (int i = 0; i < PAGE_SIZE / 8; i++)
			{
				if (DesPageBase[i] != ((PULONG64)DesExecutePageBase)[i])
				{
					equal = false;
					break;
				}
			}
			if (equal)
			{
				VM::EptHookMerge(DesPfn);
				ExFreePool(DesExecutePageBase);
			}
		}
		return result;
	}
#endif // WINNT
};

#pragma warning(pop)