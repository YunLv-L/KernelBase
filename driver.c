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

    // 可选自检
    BOOLEAN inHal = IsAddressInModule(base, L"hal.dll"); // 基址当然不在 hal 内，应返回 FALSE
    DbgPrint("[KernelBase] IsAddressInModule(ntos base, hal.dll) = %d (expected 0)\n", inHal);
    return STATUS_SUCCESS;
}