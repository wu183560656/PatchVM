[Defines]
    PLATFORM_NAME                  = PatchHyperV
    PLATFORM_GUID                  = 6a360feB-53Da-E75A-07Cc-936839a02402
    PLATFORM_VERSION               = 1.0
    DSC_SPECIFICATION              = 0x00010005
	SUPPORTED_ARCHITECTURES        = X64
	BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
[LibraryClasses]
    UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
	UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
    PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
    UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
	PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
	MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
	UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
	RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
	StackCheckLib|MdePkg/Library/StackCheckLib/StackCheckLib.inf
	StackCheckFailureHookLib|MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf
[Components]
    ../PatchHyperV.inf