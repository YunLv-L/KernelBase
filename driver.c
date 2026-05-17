#include "kernelbase.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrint("[KernelBase] Unloaded.\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverUnload;

    PVOID base = GetKernelBase();
    DbgPrint("[KernelBase] Loaded, kernel base: 0x%p\n", base);

    // 可选：快速自检 v1.3.0 新函数
    ULONG moduleCount = GetSystemModuleCount();
    DbgPrint("[KernelBase] Loaded modules: %lu\n", moduleCount);

    PVOID halBase = GetModuleBaseByName(L"hal.dll");
    ULONG halSize = GetModuleSizeByName(L"hal.dll");
    DbgPrint("[KernelBase] hal.dll base: 0x%p, size: 0x%lx\n", halBase, halSize);

    return STATUS_SUCCESS;
}