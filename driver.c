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

    // 可选：快速自检新功能
    ULONG textSize = 0;
    PVOID textAddr = GetKernelSectionByName(".text", &textSize);
    DbgPrint("[KernelBase] .text section at 0x%p, size 0x%lx\n", textAddr, textSize);

    BOOLEAN pg = IsPatchGuardEnabled();
    DbgPrint("[KernelBase] PatchGuard enabled: %d\n", pg);

    return STATUS_SUCCESS;
}