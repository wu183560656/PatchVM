#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "kdmapper.hpp"

#pragma comment(lib, "ntdll.lib")

bool fileExists(const std::wstring& filename) {
	std::ifstream file(filename);
	return file.good(); // 如果文件可以打开，返回 true，否则返回 false
}

extern"C" BOOL WINAPI RtlDosPathNameToNtPathName_U(
	PCWSTR DosPath,
	PUNICODE_STRING NtPath,
	PCWSTR* FilePart,
	PVOID RelativeName
);

std::wstring ConvertDosPathToNtPath(const std::wstring& pDosPath)
{
	std::wstring result;
	UNICODE_STRING DosPath = { 0 };
	UNICODE_STRING NtPath = { 0 };
	BOOL bResult = FALSE;
	WCHAR* pBuffer = NULL;

	// 初始化UNICODE_STRING
	RtlInitUnicodeString(&DosPath, pDosPath.c_str());
	// 转换路径
	if (RtlDosPathNameToNtPathName_U(DosPath.Buffer, &NtPath, NULL, NULL))
	{
		result = std::wstring(NtPath.Buffer, NtPath.Length);
		// 必须释放函数内部分配的缓冲区
		RtlFreeUnicodeString(&NtPath);
	}
	return result;
}

int wmain(const int argc, wchar_t** argv)
{
	bool success = false;
	HANDLE iqvw64e_device_handle = NULL;
	do
	{
		wchar_t module_file_name[MAX_PATH] = { 0 };
		GetModuleFileNameW(nullptr, module_file_name, sizeof(module_file_name) / sizeof(*module_file_name));
		for (int i = wcslen(module_file_name) - 1; i >= 0; i--)
		{
			if (module_file_name[i] == L'\\')
				break;
			else
				module_file_name[i] = L'\0';
		}
		std::wstring driver_path = std::wstring(module_file_name) + L"Loader.sys";
		std::wstring hvm_path = std::wstring(module_file_name) + L"vmdisk.img";
		if (!fileExists(driver_path))
		{
			std::wcout << "Loader.sys not found in " << driver_path.c_str() << "\n";
			break;
		}
		if (!fileExists(hvm_path))
		{
			std::wcout << "vmdisk.img not found in " << hvm_path.c_str() << "\n";
			break;
		}
		//路径转换
		std::wstring hvm_path_nt = ConvertDosPathToNtPath(hvm_path);
		std::vector<unsigned char> raw_image;
		if (!utils::ReadFileToMemory(driver_path, &raw_image))
		{
			std::wcout << "Driver file" << driver_path << " read failed\n";
			break;
		}
		iqvw64e_device_handle = intel_driver::Load();
		if (iqvw64e_device_handle == INVALID_HANDLE_VALUE) {
			std::wcout << "Load Intel Driver Fail!\n";
			break;
		}
		auto mode = kdmapper::AllocationMode::AllocatePool;

		NTSTATUS exitCode = 0;
		unsigned __int64 driver_address = kdmapper::MapDriver(iqvw64e_device_handle, raw_image.data(), 0, (ULONG64)hvm_path_nt.c_str(), true, true, mode, false, nullptr, &exitCode);
		if (driver_address == 0)
		{
			std::wcout << "MapDriver Fail!\n";
			break;
		}
		if (exitCode == -1)
		{
			std::wcout << "MapDriver Success!\n";
		}
		else
		{
			std::wcout << "MapDriver Fail with exit code: " << exitCode << "\n";
			break;
		}
		success = true;
	} while (false);
	if (iqvw64e_device_handle)
	{
		intel_driver::Unload(iqvw64e_device_handle);
	}
	system("pause");
	return 0;
}