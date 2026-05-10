#include "kernelbase.h"

#define SystemModuleInformation 0x0B

PVOID GetKernelBase(void)
{
    NTSTATUS status;
    PRTL_PROCESS_MODULES pModules = NULL;
    ULONG bufferSize = 0;
    PVOID kernelBase = NULL;
    UNICODE_STRING routineName;

    RtlInitUnicodeString(&routineName, L"ZwQuerySystemInformation");
    NTSTATUS (*pZwQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG) =
        (NTSTATUS (*)(ULONG, PVOID, ULONG, PULONG))MmGetSystemRoutineAddress(&routineName);
    if (!pZwQuerySystemInformation)
        return NULL;

    status = pZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &bufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return NULL;

    pModules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bufferSize, 'bKgK');
    if (!pModules)
        return NULL;

    status = pZwQuerySystemInformation(SystemModuleInformation, pModules, bufferSize, NULL);
    if (NT_SUCCESS(status) && pModules->NumberOfModules > 0)
        kernelBase = pModules->Modules[0].ImageBase;

    ExFreePoolWithTag(pModules, 'bKgK');
    return kernelBase;
}

// 新增：将 ntoskrnl 内的相对虚拟地址(RVA) 转换为绝对虚拟地址(VA)
PVOID GetKernelVaByRva(ULONG_PTR Rva)
{
    PVOID kernelBase = GetKernelBase();
    if (!kernelBase)
        return NULL;
    return (PVOID)((PUCHAR)kernelBase + Rva);
}