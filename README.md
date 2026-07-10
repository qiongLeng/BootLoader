# 🔄 STM32 BootLoader — 串口固件升级 + 双备份容错 + SPI Flash缓存

<p align="center">
  <img src="https://img.shields.io/badge/Platform-STM32F1-blue?style=for-the-badge&logo=stmicroelectronics" alt="STM32">
  <img src="https://img.shields.io/badge/Storage-W25Q16_(SPI)-orange?style=for-the-badge" alt="W25Q16">
  <img src="https://img.shields.io/badge/Storage-AT24C02_(I2C)-green?style=for-the-badge" alt="AT24C02">
  <img src="https://img.shields.io/badge/Verify-CRC32-red?style=for-the-badge" alt="CRC32">
  <img src="https://img.shields.io/badge/Protocol-UART-blueviolet?style=for-the-badge" alt="UART">
</p>

<p align="center">
  <b>📡 串口 IAP 在线升级 · W25Q16 固件缓存 · 双 APP 容错备份 · CRC32 完整性校验</b>
</p>

---

## 📖 项目简介

本项目是一个**工业级 STM32 串口 IAP BootLoader**，支持通过**串口 UART** 在线升级固件。使用 **W25Q16 SPI Flash** 作为固件缓存区、**AT24C02 I2C EEPROM** 存储升级标志位，实现 **双 APP 备份容错机制**（主 APP + 新版本 APP 双区），通过 **CRC32 硬件校验** 确保固件完整性。采用**状态机架构**，通信方式可灵活扩展为 CAN / LoRa / 蓝牙等。

> 🎯 适用场景：嵌入式产品在线升级、工业设备远程固件更新、STM32 IAP 实战学习。

---

## ✨ 功能特性

| 功能 | 描述 |
|------|------|
| 📡 **串口固件接收** | USART1 空闲中断 (IDLE) 接收新固件 `.bin` 文件 |
| 💾 **SPI Flash 缓存** | W25Q16 (2MB) 缓存升级固件，支持扇区擦除/页写入 |
| 🔐 **双 APP 容错** | 主 APP (0x08008000) + 新版本 APP (0x08014000) 双区备份 |
| ✅ **CRC32 校验** | STM32 硬件 CRC 模块，升级前后双重验证 |
| 🏷️ **EEPROM 标志位** | AT24C02 存储更新状态，掉电不丢失 |
| 🔄 **自动回退** | 升级失败自动回退到旧版本 APP |
| ⏱️ **3 秒超时** | 上电 3 秒内无升级指令自动跳转 APP |
| 🧩 **可扩展通信** | 抽象通信层，可替换为 CAN / LoRa / RS485 等 |

---

## 🏗️ 系统架构

```
┌──────────────────────────────────────────────────────────────────┐
│                      STM32 Flash 布局                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                    │
│  0x08000000 ┌──────────────────────┐                              │
│             │     BootLoader       │  ← 本项目 (32KB)              │
│  0x08008000 ├──────────────────────┤                              │
│             │     APP (主程序)      │  ← 用户应用程序 (48KB)        │
│  0x08014000 ├──────────────────────┤                              │
│             │  APP_New (新版本)     │  ← 新固件备份区 (48KB)        │
│  0x08020000 ├──────────────────────┤                              │
│             │     ... (保留)        │                              │
│  0x08040000 └──────────────────────┘                              │
│                                                                    │
├──────────────────────────────────────────────────────────────────┤
│                      硬件外设                                      │
├──────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐                    │
│  │ W25Q16   │    │ AT24C02  │    │  USART1  │                    │
│  │ SPI Flash│    │ I2C EEP  │    │  串口    │                    │
│  │ 固件缓存 │    │ 升级标志 │    │ 固件接收 │                    │
│  │  2MB     │    │  256B    │    │ 升级指令 │                    │
│  └──────────┘    └──────────┘    └──────────┘                    │
│                                                                    │
└──────────────────────────────────────────────────────────────────┘
```

### 升级流程

```
上电
  │
  ▼
┌─────────────┐
│ IDLE 初始态  │
└──────┬──────┘
       │ USART1 空闲中断接收
       ▼
┌─────────────┐
│ WAIT_CMD    │ ← 等待 3 秒
│ 等待升级指令 │
└──┬──────┬───┘
   │      │
   │      └── 超时 3 秒? ──→ NO_DATA ──→ 读 EEPROM 标志位
   │                                     ├─ 0x01 → JumpToApp() (主APP)
   │                                     ├─ 0x02 → JumpToBackupApp() (备用APP)
   │                                     └─ 0x00 → 回到更新模式
   │
   └── 收到 "UPDATE" 指令
       │
       ▼
┌─────────────────┐
│ UPDATE_AVAILABLE │ ← 擦除 W25Q16, 准备接收
│ 接收固件数据      │
└────────┬────────┘
         │ 接收完成? (flash_flag)
         ▼
┌─────────────────┐
│ UPDATE_TO_APP    │
│ 1. W25Q16 → APP_NEW_ADDRESS (0x08014000)
│ 2. W25Q16 → APP_ADDRESS (0x08008000)
│ 3. CRC32 校验
└────────┬────────┘
         │ CRC 通过?
         ▼
┌─────────────────┐
│ JUMP_TO_APP      │
│ 1. 读 EEPROM 标志位
│ 2. 关外设, 关中断
│ 3. 重设堆栈指针
│ 4. 跳转到 APP 复位向量
└─────────────────┘
```

---

## 💻 软件架构

```
📁 BootLoader/
├── 📁 boot_update/                 # BootLoader 核心代码
│   ├── Boot_Communication.c/.h     #   通信接收 + 状态机
│   ├── Boot_SPI.c/.h               #   W25Q16 SPI Flash 驱动
│   ├── Boot_I2C.c/.h               #   AT24C02 I2C EEPROM 驱动
│   └── Jump.c/.h                   #   Flash 擦写 + APP 跳转
└── README.md
```

### 核心模块详解

#### 1. Boot_Communication — 通信与状态机

- 使用 `HAL_UARTEx_ReceiveToIdle_IT` 空闲中断接收固件数据
- 每次接收 **4096 字节** (`BOOT_COMMUNICATION_SIZE`)
- 收到 `"UPDATE"` 字符串 → 进入升级模式
- 六状态状态机管理完整升级流程

```c
typedef enum {
    BOOT_COMMUNICATION_IDLE,              // 初始态, 启动接收
    BOOT_COMMUNICATION_WAIT_CMD,          // 等待升级指令 (3秒超时)
    BOOT_COMMUNICATION_NO_DATA,           // 超时, 读EEPROM标志判断跳转
    BOOT_COMMUNICATION_UPDATE_AVAILABLE,  // 接收固件数据 → W25Q16
    BOOT_COMMUNICATION_UPDATE_TO_APP,     // W25Q16 → Flash + CRC校验
    BOOT_COMMUNICATION_JUMP_TO_APP,       // 跳转到APP
} Boot_Communication_StateTypeDef;
```

#### 2. Boot_SPI — W25Q16 Flash 驱动

| 操作 | 函数 | 说明 |
|------|------|------|
| 扇区擦除 (4KB) | `SPI_flash_sector_erase()` | 写前必须擦除 |
| 预擦除 | `SPI_flash_pre_erase()` | 批量擦除指定长度 |
| 页写入 | `SPI_flash_page_write()` | 支持跨页自动分段 (256B/页) |
| 页读取 | `SPI_flash_page_read()` | 连续读取 |

#### 3. Boot_I2C — AT24C02 EEPROM 驱动

| 地址 | 内容 | 说明 |
|------|------|------|
| 0x00 | APP 标志 | 0x01=有效, 0x00=无效 |
| 0x02 | APP_New 标志 | 0x01=有效, 0x00=无效 |

- `Boot_I2C_readkey()` 检测三个字节判断有效固件位置
- 升级过程中先写 `UPDATE_FLAG_FAIL`, 成功后写 `UPDATE_FLAG_OK`（原子性保护）

#### 4. Jump — Flash 操作与跳转

```c
// Flash 内部操作
InternalFlash_Erase(start_addr, len);   // 页擦除
InternalFlash_Write(start_addr, data, len); // 半字写入

// W25Q16 → STM32 Flash
SPI_To_Flash_Internal(W25Q16_addr, flash_addr, len);

// CRC32 硬件校验
CRC_flash_cal(APP_ADDRESS, APP_NEW_VERSION_SIZE)  // 校验 Flash
CRC_uart_cal(Data, APP_NEW_VERSION_SIZE)          // 校验原始数据

// APP 跳转 (关外设 → 关中断 → 设MSP → 跳转)
JumpToApp();         // 跳转到主 APP (0x08008000)
JumpToBackupApp();   // 跳转到备用 APP (0x08014000)
```

---

## 🔩 硬件需求

| 模块 | 接口 | 作用 |
|------|------|------|
| W25Q16 / W25Q32 | SPI | 固件缓存 (2MB/4MB) |
| AT24C02 | I2C | 升级标志位存储 (256B) |
| USB-TTL (CH340) | USART1 | 固件发送 |

### 引脚连接（示例）

| 外设 | STM32 引脚 |
|------|-----------|
| W25Q16 SCK | PA5 (SPI1) |
| W25Q16 MISO | PA6 (SPI1) |
| W25Q16 MOSI | PA7 (SPI1) |
| W25Q16 CS | PB0 |
| AT24C02 SCL | PB6 (I2C1) |
| AT24C02 SDA | PB7 (I2C1) |
| USB-TTL TX | PA10 (USART1 RX) |
| USB-TTL RX | PA9 (USART1 TX) |

---

## 🚀 快速开始

### 1. 编译 BootLoader

- 使用 **Keil MDK5** 或 **STM32CubeIDE** 编译
- 确保 BootLoader 起始地址为 `0x08000000`，大小不超过 32KB

### 2. 编译用户 APP

- APP 起始地址设为 `0x08008000`（在 Keil 的 Target → IROM1 中修改）
- 在 `system_stm32f1xx.c` 中重定位向量表：

```c
SCB->VTOR = FLASH_BASE | 0x8000;  // 偏移 32KB
```

### 3. 生成 .bin 文件

在 Keil → Magic Wand → User → After Build/Rebuild → Run #1：

```
"D:\stm32\Keil\ARM\ARMCC\bin\fromelf.exe" --bin -o"$L@L.bin" "#L"
```

（路径根据自己 Keil 安装位置修改）

### 4. 固件升级操作

1. 将 BootLoader 烧录到 STM32
2. 上电 → BootLoader 运行, 等待 3 秒
3. 通过串口工具发送 `UPDATE` 字符串
4. 发送 `.bin` 固件文件
5. BootLoader 自动完成：W25Q16 缓存 → Flash 写入 → CRC 校验 → 跳转 APP

---

## ⚙️ 关键地址配置

```c
// Jump.h 中的关键宏定义

#define APP_ADDRESS              0x08008000   // 主 APP 起始地址
#define APP_NEW_VERSION_ADDRESS  0x08014000   // 新版本 APP 起始地址
#define SPI_FLASH_APP_ADDRESS    0x00000000   // W25Q16 中固件存储起始地址
#define APP_NEW_VERSION_SIZE     3992         // 固件大小 (字节)
#define SPI_TO_FLASH_SIZE        1024         // 每次搬运 1024 字节
```

---

## 🎯 设计亮点

### 1. 双区备份容错

```
        升级前                  升级后
  ┌─────────────────┐    ┌─────────────────┐
  │ APP (旧版本)     │    │ APP (新版本)     │ ← 写入新固件
  ├─────────────────┤    ├─────────────────┤
  │ APP_New (空/旧)  │    │ APP_New (新版本) │ ← 同时备份
  └─────────────────┘    └─────────────────┘
  
  升级失败 → EEPROM 标志未更新 → 自动回退到旧 APP
```

### 2. W25Q16 三级缓存流水线

```
USART1 接收 → Data[4096] → W25Q16 页写入 → Flash 半字写入 → CRC32 校验
   (中断)       (RAM)        (SPI Flash)       (内部Flash)      (硬件CRC)
```

### 3. 原子性标志位保护

```
写入流程: 先写 FAIL → 烧写 Flash → CRC 通过 → 写 OK
如果中途断电 → EEPROM 中标志位为 FAIL → 下次上电自动重新烧写
```

---

## 📝 待优化 (TODO)

- [ ] 支持 **加密固件** (AES 解密后烧写)
- [ ] 添加 **通信协议帧格式** (包头+长度+CRC16+包尾)
- [ ] 支持 **断点续传** (记录已接收地址)
- [ ] 扩展 **CAN 总线** 升级
- [ ] 添加 **安全启动** (签名验证)
- [ ] 支持 **压缩固件** (减少传输时间)
- [ ] 添加 **升级进度回传**

---

## 🤝 贡献与反馈

如果你觉得这个项目对你有帮助：

- ⭐ **Star** 这个项目
- 🐛 提交 **Issue** 反馈问题
- 🔧 提交 **Pull Request** 完善代码

---

## 📄 License

本项目仅供学习和研究使用。

---

<p align="center">
  <b>🔧 从 BootLoader 到 APP — 深入理解 STM32 IAP 在线升级的完整实现 🔧</b>
  <br><br>
  <i>如果本项目对你有帮助，请给一个 ⭐ Star 支持一下！</i>
</p>

