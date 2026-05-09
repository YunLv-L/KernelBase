# KernelBase - Minimal Kernel Driver Exports

一个极简的 Windows 内核驱动（`.sys`），只做一件事：**导出 `GetKernelBase` 函数**，供其他内核模块动态获取 `ntoskrnl.exe` 的基址。

## 为什么需要它？

许多内核安全工具、Rootkit 检测或未导出函数调用都需要先知道内核基址。本驱动把这一步骤抽离为一个独立模块，其他驱动只需通过 `MmGetSystemRoutineAddress(L"GetKernelBase")` 即可获取函数指针，避免重复造轮子。

## 功能

- 导出函数：`PVOID GetKernelBase(void)`
- 返回 `ntoskrnl.exe` 在内存中的加载基址（失败返回 `NULL`）
- 纯内核导出，无设备对象，无 IOCTL

## 编译要求

- **Visual Studio Build Tools**（2019 或更高版本，含 MSVC 工具链）
- **Windows Driver Kit (WDK)**，版本与 SDK 匹配，例如 `10.0.26100.0`
- 安装后，WDK 路径通常为 `D:\WindowsKit`（请根据实际情况修改）

## 编译方法

1. 打开 **x64 Native Tools Command Prompt for VS**。
2. 设置 WDK 根目录环境变量（若 WDK 在其他盘或目录须修改）：
   ```cmd
   set WDKBuildRoot=D:\WindowsKit
   ```
3.进入仓库根目录，运行：   
```cmd
build.bat
```
4.成功后会在 bin\x64\Debug\ 下生成 KernelBase.sys。
