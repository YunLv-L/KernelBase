# KernelBase

> A minimal Windows kernel driver that exports `GetKernelBase` to retrieve the base address of `ntoskrnl.exe`.  
> 极简 Windows 内核驱动，导出 `GetKernelBase` 函数，供其他内核模块获取 `ntoskrnl.exe` 基址。

---

## Why KernelBase? / 为什么需要 KernelBase？

Many kernel security tools, rootkit detection, or calling of non‑exported kernel functions require the base address of `ntoskrnl.exe`. This driver isolates that step into a standalone module, so other drivers only need to dynamically obtain the exported function – no need to reinvent the wheel.

很多内核安全工具、Rootkit 检测，或是调用未导出的内核函数，都需要先拿到 `ntoskrnl.exe` 的基址。这个驱动把这一步抽离成一个独立模块，其他驱动只需动态获取这个导出函数就行了，不用重复造轮子。

---

## Features / 功能

- Exported function / 导出函数：`PVOID GetKernelBase(void)`
- Returns the load base of `ntoskrnl.exe` in memory (returns `NULL` on failure)  
  返回 `ntoskrnl.exe` 在内存中的加载基址（失败时返回 `NULL`）
- Pure kernel export – no device objects, no IOCTL  
  纯内核导出，无设备对象，无 IOCTL

---

## Build Requirements / 编译要求

- **Visual Studio Build Tools** (2019 or later, with MSVC toolchain)  
  **Visual Studio Build Tools**（2019 或更高，包含 MSVC 工具链）
- **Windows Driver Kit (WDK)** – e.g., version `10.0.26100.0`  
  **Windows Driver Kit (WDK)** – 例如版本 `10.0.26100.0`
- Typical WDK installation path / 典型 WDK 安装路径: `D:\WindowsKit` (adjust as needed / 根据实际情况修改)

---

## How to Build / 如何编译

1. Open **x64 Native Tools Command Prompt for VS**  
   打开 **x64 Native Tools Command Prompt for VS**
2. Set the WDK root environment variable (modify if your WDK is elsewhere)  
   设置 WDK 根目录环境变量（如果 WDK 装在其他位置请自行修改）:
   ```cmd
   set WDKBuildRoot=D:\WindowsKit
   ```
3. Navigate to the project root and run / 进入项目根目录运行:
   ```cmd
   build.bat
   ```
4. The compiled driver will be located at `bin\x64\Debug\KernelBase.sys`  
   编译好的驱动会放在 `bin\x64\Debug\KernelBase.sys`。

---

## Signing / 签名

The repository includes a signing script for **test certificates** only.  
本仓库仅提供**测试证书**签名脚本。  
For production, use an EV Code Signing Certificate or WHQL attestation signing.  
生产环境请使用 EV 代码签名证书或 WHQL 证明签名。

1. Place your `.pfx` certificate (e.g., `MyCert.pfx`) in the project root  
   将你的 `.pfx` 证书（例如 `MyCert.pfx`）放在项目根目录。
2. Edit `sign.bat` and update the certificate name and password  
   编辑 `sign.bat`，修改证书名和密码。
3. Run `sign.bat` as Administrator  
   以管理员身份运行 `sign.bat`。
4. Before loading the driver, enable test signing mode / 加载驱动前，先开启测试签名模式:
   ```cmd
   bcdedit /set testsigning on
   ```
   Then restart the system / 然后重启系统。

---

## Loading the Driver / 加载驱动

```cmd
sc create KernelBase type= kernel binPath= "full\path\to\KernelBase.sys"
sc start KernelBase
```

To make it start automatically at boot / 设置为系统启动时自动加载:
```cmd
sc config KernelBase start= system
```

---

## Usage in Other Drivers / 在其他驱动中使用

### ❌ Wrong approach (will always fail) / 错误用法（永远失败）

`MmGetSystemRoutineAddress` **only** searches the export tables of `ntoskrnl.exe` and `hal.dll`. It **cannot** find exports from any third‑party driver (including `KernelBase.sys`). The call below will **always** return `NULL`.

`MmGetSystemRoutineAddress` **仅**查找 `ntoskrnl.exe` 和 `hal.dll` 的导出表，**无法**解析任何第三方驱动（包括 `KernelBase.sys`）的导出。下面的调用**永远**返回 `NULL`。

```c
// DO NOT DO THIS - it will never work / 不要这样做，永远无效
PVOID (*GetKernelBase)(void) = (PVOID (*)(void))MmGetSystemRoutineAddress(L"GetKernelBase");
```

### ✅ Correct approach / 正确用法

You need to manually locate `KernelBase.sys` in the system module list and parse its PE export table. The function below does exactly that.  
你需要手动在系统模块列表中定位 `KernelBase.sys` 并解析它的 PE 导出表。下面的函数就是用来完成这一工作的。

```c
// Universal helper: get exported function address from a loaded driver
// 通用辅助函数：从已加载驱动中获取导出函数地址
PVOID GetExportFromDriver(PCWSTR DriverFileName, PCSTR ExportName)
{
    NTSTATUS status;
    ULONG size = 0;
    PRTL_PROCESS_MODULES modules = NULL;
    PVOID funcAddr = NULL;

    status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &size);
    if (status != STATUS_INFO_LENGTH_MISMATCH) return NULL;

    modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, size, 'gKwK');
    if (!modules) return NULL;

    status = ZwQuerySystemInformation(SystemModuleInformation, modules, size, NULL);
    if (!NT_SUCCESS(status)) { ExFreePoolWithTag(modules, 'gKwK'); return NULL; }

    PVOID targetBase = NULL;
    for (ULONG i = 0; i < modules->NumberOfModules; ++i) {
        PCWSTR fullPath = modules->Modules[i].FullPathName;
        PCWSTR fileName = wcsrchr(fullPath, L'\\');
        fileName = fileName ? (fileName + 1) : fullPath;
        if (_wcsicmp(fileName, DriverFileName) == 0) {
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
    DWORD* names = (DWORD*)((PUCHAR)targetBase + exports->AddressOfNames);
    WORD* ordinals = (WORD*)((PUCHAR)targetBase + exports->AddressOfNameOrdinals);
    DWORD* functions = (DWORD*)((PUCHAR)targetBase + exports->AddressOfFunctions);

    for (DWORD i = 0; i < exports->NumberOfNames; ++i) {
        PCSTR name = (PCSTR)((PUCHAR)targetBase + names[i]);
        if (strcmp(name, ExportName) == 0) {
            funcAddr = (PVOID)((PUCHAR)targetBase + functions[ordinals[i]]);
            break;
        }
    }
    return funcAddr;
}

// In your DriverEntry / 在你的 DriverEntry 中
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    // ...
    PVOID (*GetKernelBase)(void) = (PVOID (*)(void))
        GetExportFromDriver(L"KernelBase.sys", "GetKernelBase");

    if (!GetKernelBase) {
        return STATUS_NOT_FOUND;
    }

    PVOID kernelBase = GetKernelBase();
    if (!kernelBase) {
        return STATUS_UNSUCCESSFUL;
    }

    // Now you can use kernelBase...
    return STATUS_SUCCESS;
}
```

> **Note**: You still need to make sure your driver loads **after** `KernelBase.sys`, otherwise it won't be found in the module list. Use service dependency.  
> **注意**：你仍然需要确保你的驱动在 `KernelBase.sys` **之后**加载，否则模块列表中找不到它。请使用服务依赖。

---

## Loading Order / 加载顺序

Calling drivers **must** be loaded after `KernelBase.sys`. You can enforce this via service dependency:  
调用方驱动**必须**在 `KernelBase.sys` 之后加载，可通过服务依赖保证加载顺序：

```cmd
sc create YourDriver type= kernel binPath= "..." depend= KernelBase
```

---

## License / 许可证

This project is licensed under the **Apache License 2.0**.  
本项目采用 **Apache License 2.0** 授权。  
See the [LICENSE](LICENSE) file for the full license text.  
完整许可证文本请查看 [LICENSE](LICENSE) 文件。

---

## Disclaimer / 免责声明

This driver is intended for **educational and research purposes** only.  
本驱动仅用于**教育和研究**目的。  
Use in production environments requires proper Microsoft signing and compliance with applicable laws.  
在生产环境中使用需要正确的微软签名，并遵守相关法律法规。
```
