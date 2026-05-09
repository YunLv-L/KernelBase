# KernelBase

> A minimal Windows kernel driver that exports `GetKernelBase` to retrieve the base address of `ntoskrnl.exe`.  
> 极简 Windows 内核驱动，导出 `GetKernelBase` 函数，供其他内核模块获取 `ntoskrnl.exe` 基址。

---

## Why KernelBase? / 为什么需要 KernelBase？

Many kernel security tools, rootkit detection mechanisms, or calls to non‑exported kernel functions require the base address of `ntoskrnl.exe` as a starting point. This driver isolates that step into a standalone module. Other drivers only need to dynamically obtain the exported function – no need to reinvent the wheel.

很多内核安全工具、Rootkit 检测，或是调用未导出的内核函数，都需要先拿到 `ntoskrnl.exe` 的基址。这个驱动把这一步抽离成一个独立模块，其他驱动只需动态获取这个导出函数就行了，不用重复造轮子。

---

## Features / 功能

- **Exported function / 导出函数**: `PVOID GetKernelBase(void)`
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

---

## Usage in Other Drivers / 在其他驱动中使用

```c
UNICODE_STRING funcName;
RtlInitUnicodeString(&funcName, L"GetKernelBase");
PVOID (*GetKernelBase)(void) =
    (PVOID (*)(void))MmGetSystemRoutineAddress(&funcName);

if (GetKernelBase) {
    PVOID kernelBase = GetKernelBase();
    // Now you can parse the ntoskrnl export table to resolve any other kernel function.
    // 现在你可以解析 ntoskrnl 的导出表，获取任意其他内核函数地址。
}
```

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
