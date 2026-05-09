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

    return STATUS_SUCCESS;
}