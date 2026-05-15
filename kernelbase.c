#include "kernelbase.h"
#include <ntimage.h>

#define SystemModuleInformation 0x0B

// 内部辅助：获取 ZwQuerySystemInformation 指针
static NTSTATUS (*GetZwQuerySystemInformation(void))(ULONG, PVOID, ULONG, PULONG)
{
    UNICODE_STRING routineName;
    RtlInitUnicodeString(&routineName, L"ZwQuerySystemInformation");
    return (NTSTATUS (*)(ULONG, PVOID, ULONG, PULONG))MmGetSystemRoutineAddress(&routineName);
}

// ========== 原有函数 ==========

PVOID GetKernelBase(void)
{
    NTSTATUS status;
    PRTL_PROCESS_MODULES pModules = NULL;
    ULONG bufferSize = 0;
    PVOID kernelBase = NULL;
    NTSTATUS (*pZwQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG) = GetZwQuerySystemInformation();
    if (!pZwQuerySystemInformation) return NULL;

    status = pZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &bufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH) return NULL;

    pModules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bufferSize, 'bKgK');
    if (!pModules) return NULL;

    status = pZwQuerySystemInformation(SystemModuleInformation, pModules, bufferSize, NULL);
    if (NT_SUCCESS(status) && pModules->NumberOfModules > 0)
        kernelBase = pModules->Modules[0].ImageBase;

    ExFreePoolWithTag(pModules, 'bKgK');
    return kernelBase;
}

PVOID GetKernelVaByRva(ULONG_PTR Rva)
{
    PVOID kernelBase = GetKernelBase();
    if (!kernelBase) return NULL;
    return (PVOID)((PUCHAR)kernelBase + Rva);
}

// ========== v1.2.0 新增函数 ==========

// 1. 从任意内核模块获取导出函数地址
PVOID GetKernelExportByName(PCWSTR ModuleName, PCSTR FunctionName)
{
    NTSTATUS status;
    ULONG size = 0;
    PRTL_PROCESS_MODULES modules = NULL;
    PVOID funcAddr = NULL;
    NTSTATUS (*pZwQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG) = GetZwQuerySystemInformation();
    if (!pZwQuerySystemInformation) return NULL;

    status = pZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &size);
    if (status != STATUS_INFO_LENGTH_MISMATCH) return NULL;

    modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, size, 'gKwK');
    if (!modules) return NULL;

    status = pZwQuerySystemInformation(SystemModuleInformation, modules, size, NULL);
    if (!NT_SUCCESS(status)) { ExFreePoolWithTag(modules, 'gKwK'); return NULL; }

    PVOID targetBase = NULL;
    for (ULONG i = 0; i < modules->NumberOfModules; ++i) {
        PCWSTR fullPath = (PCWSTR)modules->Modules[i].FullPathName;
        PCWSTR fileName = wcsrchr(fullPath, L'\\');
        fileName = fileName ? (fileName + 1) : fullPath;
        if (_wcsicmp(fileName, ModuleName) == 0) {
            targetBase = modules->Modules[i].ImageBase;
            break;
        }
    }
    ExFreePoolWithTag(modules, 'gKwK');
    if (!targetBase) return NULL;

    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)targetBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PUCHAR)targetBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;
    ULONG exportRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!exportRva) return NULL;

    PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)((PUCHAR)targetBase + exportRva);
    ULONG* names = (ULONG*)((PUCHAR)targetBase + exports->AddressOfNames);
    USHORT* ordinals = (USHORT*)((PUCHAR)targetBase + exports->AddressOfNameOrdinals);
    ULONG* functions = (ULONG*)((PUCHAR)targetBase + exports->AddressOfFunctions);

    for (ULONG i = 0; i < exports->NumberOfNames; ++i) {
        PCSTR name = (PCSTR)((PUCHAR)targetBase + names[i]);
        if (strcmp(name, FunctionName) == 0) {
            funcAddr = (PVOID)((PUCHAR)targetBase + functions[ordinals[i]]);
            break;
        }
    }
    return funcAddr;
}

// 2. 判断地址是否在 ntoskrnl 映像内
BOOLEAN IsAddressInKernelImage(PVOID Address)
{
    PVOID kernelBase = GetKernelBase();
    if (!kernelBase) return FALSE;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)kernelBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PUCHAR)kernelBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return FALSE;
    ULONG_PTR start = (ULONG_PTR)kernelBase;
    ULONG_PTR end = start + nt->OptionalHeader.SizeOfImage;
    ULONG_PTR addr = (ULONG_PTR)Address;
    return (addr >= start && addr < end);
}

// 3. 判断地址是否属于内核空间
BOOLEAN IsKernelAddress(PVOID Address)
{
    return ((ULONG_PTR)Address >= 0xFFFF800000000000ULL);
}

// 4. 获取内核节区信息
PVOID GetKernelSectionByName(PCSTR SectionName, PULONG SectionSize)
{
    PVOID kernelBase = GetKernelBase();
    if (!kernelBase || !SectionSize) return NULL;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)kernelBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((PUCHAR)kernelBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    for (USHORT i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (strncmp((PCSTR)section[i].Name, SectionName, IMAGE_SIZEOF_SHORT_NAME) == 0) {
            *SectionSize = section[i].Misc.VirtualSize;
            return (PVOID)((PUCHAR)kernelBase + section[i].VirtualAddress);
        }
    }
    return NULL;
}

// 5. 诊断 PatchGuard 是否激活
BOOLEAN IsPatchGuardEnabled(void)
{
    // 获取 KdDebuggerEnabled 指针，若调试器开启则 PatchGuard 自动禁用
    UNICODE_STRING varName;
    RtlInitUnicodeString(&varName, L"KdDebuggerEnabled");
    PBOOLEAN pKdDebuggerEnabled = (PBOOLEAN)MmGetSystemRoutineAddress(&varName);
    if (pKdDebuggerEnabled) {
        return (*pKdDebuggerEnabled == FALSE);
    }
    return TRUE; // 获取失败则保守认为 PatchGuard 已启用
}