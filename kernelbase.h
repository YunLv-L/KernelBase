#pragma once
#include <ntddk.h>

// 手动定义所需结构体
typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

// 原有
PVOID GetKernelBase(void);
PVOID GetKernelVaByRva(ULONG_PTR Rva);

// v1.2.0 新增
PVOID GetKernelExportByName(PCWSTR ModuleName, PCSTR FunctionName);
BOOLEAN IsAddressInKernelImage(PVOID Address);
BOOLEAN IsKernelAddress(PVOID Address);
PVOID GetKernelSectionByName(PCSTR SectionName, PULONG SectionSize);
BOOLEAN IsPatchGuardEnabled(void);