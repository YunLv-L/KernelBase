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

    // 可选：可以在这里自测一下 RVA 转换
    // PVOID testVa = GetKernelVaByRva(0);
    // DbgPrint("[KernelBase] Test RVA 0 -> VA: 0x%p\n", testVa);

    return STATUS_SUCCESS;
}