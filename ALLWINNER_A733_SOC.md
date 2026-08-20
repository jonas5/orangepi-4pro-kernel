# Allwinner A733 SoC Reference

Allwinner A733 (also marketed as T527 in some variants) is an octa-core SoC combining two ARM Cortex-A55 efficiency cores and six ARM Cortex-A76 performance cores. It targets edge computing, NAS, and embedded Linux applications. This document covers the full hardware layout, register map, power/thermal subsystem, and board-level integration as discovered from the vendor device tree and running kernel on an Orange Pi 4 Pro.

---

## Table of Contents

1. [CPU Topology](#1-cpu-topology)
2. [Clock and OPP Tables](#2-clock-and-opp-tables)
3. [DSU (Shared Cache) OPPs](#3-dsu-shared-cache-opps)
4. [GPU OPPs](#4-gpu-opps)
5. [Memory Map — Peripheral Base Addresses](#5-memory-map--peripheral-base-addresses)
6. [GPIO Pin Mapping](#6-gpio-pin-mapping)
7. [PMICs](#7-pmics)
8. [Thermal Zones](#8-thermal-zones)
9. [Power Domains](#9-power-domains)
10. [Idle States](#10-idle-states)
11. [CCU Clocks and Resets](#11-ccu-clocks-and-resets)
12. [Board Integration — Orange Pi 4 Pro](#12-board-integration--orange-pi-4-pro)
13. [Running Vendor Kernel State](#13-running-vendor-kernel-state)

---

## 1. CPU Topology

The A733 implements a **big.LITTLE** architecture with two physically distinct clusters:

| Cluster | Cores | CPU part | Variant | Implementer | physical_package_id |
|---------|-------|----------|---------|-------------|---------------------|
| Cluster 0 (LITTLE) | cpu0–cpu5 (6x Cortex-A55) | 0xd05 | 0x2 | 0x41 (ARM) | 0 |
| Cluster 1 (Big) | cpu6–cpu7 (2x Cortex-A76) | 0xd0b | 0x4 | 0x41 (ARM) | 1 |

**Performance characteristics:**

| Property | A55 | A76 |
|----------|-----|-----|
| capacity-dmips-mhz | 0x1ae (430) | 0x26a (618) |
| dynamic-power-coefficient | 0xa2 (162) | 0x1b5 (437) |

- **Enable method:** PSCI (both clusters)
- **Shared clock:** `clocks = <&ccu CLK_CPU>` (phandle 0x05, clock 0x01)
- **A55 supply:** dcdc5 (phandle 0x07)
- **A76 supply:** dcdc3 (phandle 0x08)
- **GICv3:** base address 0x3400000
- **Wakeupgen:** base address 0x0

---

## 2. Clock and OPP Tables

### A55 (Cluster 0) — Scaling Frequencies

Available from vendor kernel at `/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`:

```
416000  780000  1014000  1196000  1404000  1508000  1612000  1716000  1794000
```

**VF Bin: opp-microvolt-26m-vf0401 (26 MHz crystal)**

All OPP entries in the vendor DTB A55 table share a single voltage-scaled row; the firmware resolves per-OPP voltage at runtime. Representative values from the DTB:

| Frequency (MHz) | opp-hz (hex) | Voltage (uV) |
|-----------------|---------------|---------------|
| 416 | 0x18cba800 | 0x00 (low-power mode) |
| 780 | 0x2e7ddb00 | (minimum) |
| 1014 | 0x3c706980 | (minimum) |
| 1196 | 0x47498300 | 840000 |
| 1404 | 0x53af5700 | 1310000 |
| 1508 | 0x59e24100 | 1350000 |
| 1612 | 0x60152b00 | 1404000 |
| 1716 | 0x66481500 | 1487000 |
| 1794 | 0x6aee4480 | 1560000 |

### A76 (Cluster 1) — Scaling Frequencies

Available from vendor kernel:

```
416000  780000  1014000  1196000  1404000  1508000  1612000  1716000  1794000  1898000  1950000  2002000
```

**VF Bin: opp-microvolt-26m-vf0401**

| Frequency (MHz) | Voltage (uV) |
|-----------------|---------------|
| 416 | 800000 |
| 780 | 800000 |
| 1014 | 800000 |
| 1196 | 800000 |
| 1404 | 1310000 |
| 1508 | 1350000 |
| 1586 | 800000 |
| 1612 | 1404000 |
| 1690 | 800000 |
| 1716 | 1487000 |
| 1794 | 1560000 |
| 1898 | 1608000 |
| 1950 | 1650000 |
| 2002 | 1760000 |

**Additional DTB-only OPPs (not exposed by vendor kernel):**

408, 720, 792, 1008, 1200, 1296, 1392, 1512, 1608, 1704, 1800 MHz

Plus vendor-specific subset:
416, 728, 780, 1014, 1196, 1300, 1404, 1508, 1586, 1612, 1690, 1716, 1794, 1898, 1920, 1944, 1950, 1976, 1992, 2002 MHz

---

## 3. DSU (Shared Cache) OPPs

The DynamIQ Shared Unit l3-cache frequency can be set independently:

```
288  312  528  600  744  792  840  888  912  984  1032  1056  1152  1200  1224  1248  1296
```

All values in MHz.

---

## 4. GPU OPPs

The integrated GPU supports four operating points:

| Frequency (MHz) |
|-----------------|
| 400 |
| 600 |
| 800 |
| 1008 |

---

## 5. Memory Map — Peripheral Base Addresses

All base addresses are physical, as reported by the vendor DTB aliases.

### System

| Block | Base Address |
|-------|--------------|
| SoC SRAM Controller | 0x3000000 |
| HW Spinlock | 0x3005000 |
| SID (eFuse) | 0x3006000 |
| Msgbox | 0x3004000 |
| DMCFreq | 0x3120000 |
| Timer | 0x3009000 |
| GICv3 | 0x3400000 |
| Wakeupgen | 0x0 |
| NMI INTC | 0x7010320 |
| DMA Controller | 0x4601000 |
| Crypto Engine (CE) | 0x4603000 |
| IOMMU | 0x3900000 |
| PCK-600 | 0x7060000 |

### Clock Control Units (CCU)

| Block | Base Address |
|-------|--------------|
| CCU | 0x2002000 |
| R-CCU | 0x7010000 |
| RTC-CCU | 0x7090000 |
| CPUPLL-CCU | 0x8870000 |

### Pin Controllers

| Block | Base Address |
|-------|--------------|
| PIO (pinctrl) | 0x2000000 |
| R_PIO | 0x7025000 |

### Thermal and Watchdog

| Block | Base Address |
|-------|--------------|
| THS (thermal sensor) | 0x2522000 |
| Watchdog | 0x2050000 |

### UART

| Port | Base Address | Notes |
|------|--------------|-------|
| UART0 | 0x2500000 | Debug console (PH8) |
| UART1 | 0x2501000 | PG6 (with RTS/CTS) |
| UART2 | 0x2502000 | PB0/PB1 |
| UART3 | 0x2503000 | |
| UART4 | 0x2504000 | |
| UART5 | 0x2505000 | |
| UART6 | 0x2506000 | |
| UART7 | 0x7080000 | R_UART |
| UART8 | 0x7081000 | R_UART |

### TWI / I²C

| Port | Base Address | Notes |
|------|--------------|-------|
| TWI0 | 0x2510000 | |
| TWI1 | 0x2511000 | |
| TWI2 | 0x2512000 | |
| TWI3 | 0x2513000 | |
| TWI4 | 0x2514000 | |
| TWI5 | 0x2515000 | I2C0 on board |
| TWI6 | 0x2516000 | |
| TWI7 | 0x2517000 | I2C1 on board |
| TWI8 | 0x2518000 | |
| TWI9 | 0x2519000 | |
| TWI10 | 0x251A000 | |
| TWI11 | 0x251B000 | |
| TWI12 | 0x251C000 | |
| S_TWI0 (R_I2C) | 0x7083000 | PMICs (i2c-13) |
| S_TWI1 | 0x7084000 | |
| S_TWI2 | 0x7085000 | |

### SPI

| Port | Base Address |
|------|--------------|
| SPI0 | 0x2540000 |
| SPI1 | 0x2541000 |
| SPI2 | 0x2542000 |
| SPI3 | 0x2543000 |
| SPI4 | 0x2544000 |
| R_SPI | 0x7092000 |

### SD/MMC Host Controllers

| Port | Base Address | Usage |
|------|--------------|-------|
| SDC0 | 0x4020000 | SD card (PI1–PI6) |
| SDC1 | 0x4021000 | SDIO WiFi (PH2–PH7) |
| SDC2 | 0x4022000 | eMMC (PC1–PC6) |
| SDC3 | 0x4023000 | |

### USB

| Block | Base Address | Notes |
|-------|--------------|-------|
| UDC (OTG) | 0x4100000 | |
| EHCI0 | 0x4101000 | Host 0 |
| OHCI0 | 0x4101400 | Host 0 |
| EHCI1 | 0x4200000 | Host 1 |
| OHCI1 | 0x4200400 | Host 1 |
| XHCI2 | 0x6A00000 | Host 2 (USB3) |
| U2PHY | 0x6B00000 | USB2 PHY |

### Ethernet

| Block | Base Address |
|-------|--------------|
| GMAC0 | 0x4500000 |
| GMAC1 | 0x4510000 |

### PCIe / SerDes / Combo PHY

| Block | Base Address | Notes |
|-------|--------------|-------|
| PCIe | 0x6000000 | |
| SerDes | 0x6C00000 | |
| Combo PHY0 | 0x6C01000 | DP + USB |
| Combo PHY1 | 0x6C02000 | PCIe + USB |

### Display / Video Output

| Block | Base Address | Notes |
|-------|--------------|-------|
| Display Engine (DE) | 0x5000000 | |
| G2D (2D Engine) | 0x5440000 | |
| Deinterlace | 0x5400000 | |
| VO0 | 0x5500000 | Video Output 0 |
| VO1 | 0x5510000 | Video Output 1 |
| TCON0 | 0x5501000 | |
| TCON1 | 0x5502000 | |
| TCON2 | 0x5503000 | |
| TCON3 (TV) | 0x5730000 | HDMI path |
| TCON4 | 0x5731000 | |
| DSI0 | 0x5506000 | |
| DSI0 Combo PHY | 0x5507000 | |
| DSI1 | 0x5508000 | MIPI LCD |
| DSI1 Combo PHY | 0x5509000 | |
| HDMI | 0x5520000 | |
| EDP | 0x5720000 | |

### Camera / Video Input

| Block | Base Address | Notes |
|-------|--------------|-------|
| VIND | 0x5800800 | Video Input |
| CSI0 | 0x5820000 | |
| CSI1 | 0x5821000 | |
| CSI2 | 0x5822000 | |
| MIPI0 | 0x5810100 | |
| MIPI1 | 0x5810200 | |
| MIPI2 | 0x5810300 | |
| TDM | 0x5908000 | |

### Video Engine (Encode/Decode)

| Block | Base Address |
|-------|--------------|
| VE1 | 0x1C0E000 |
| VE2 | 0x1C10000 |

### Audio

| Block | Base Address | Notes |
|-------|--------------|-------|
| I2S0 | 0x2532000 | |
| I2S1 | 0x2533000 | |
| I2S2 | 0x2534000 | |
| I2S3 | 0x2535000 | |
| I2S4 | 0x2536000 | |
| DMIC | 0x2531000 | |
| OWA (One Wire Audio) | 0x2537000 | |

### Analog / ADC

| Block | Base Address |
|-------|--------------|
| LEDC (LED Controller) | 0x2520000 |
| GPADC | 0x2521000 |
| LRADC (Low-Res ADC) | 0x2524000 |
| THS | 0x2522000 |

### Infrared

| Block | Base Address | Notes |
|-------|--------------|-------|
| IRRX (MIR) | 0x2526000 | Main domain |
| R_IRRX | 0x7040000 | R domain |
| IRTX | 0x2525000 | |

### PWM

| Block | Base Address |
|-------|--------------|
| PWM0 | 0x2527000 |
| PWM1 | 0x2528000 |
| R_PWM | 0x7023000 |

### Other

| Block | Base Address | Notes |
|-------|--------------|-------|
| UFS | 0x4520000 | Universal Flash Storage |
| NSI Controller | 0x2020000 | |
| NPU | 0x3600000 | Neural Processing Unit |
| IR | 0x2526000 | Infrared RX |

---

## 6. GPIO Pin Mapping

The A733 has 12 GPIO ports (A–L). Alternate function selection is via pin mux registers in the PIO block.

| Port | Pins | Primary Uses |
|------|------|--------------|
| PA | PA0–PA7 | NOR flash, TWI2 |
| PB | PB0–PB13 | UART2, TWI3, I2S2, GMAC |
| PC | PC0–PC6 | SDC2 (eMMC) |
| PD | PD0–PD17 | DSI, LVDS |
| PE | PE0–PE18 | CSI camera interface |
| PF | PF0–PF6 | SDC0 (SD card) |
| PG | PG0–PG15 | UART1, JTAG, SDC1 (SDIO/WiFi) |
| PH | PH0–PH19 | UART0 (debug), I2C, USB, LED, HDMI |
| PI | PI0–PI16 | SD card detect, USB, LED, key |
| PJ | PJ0–PJ5 | RGB LCD |
| PK | PK0–PK17 | LVDS |
| PL | PL0–PL11 | R_I2C (PMICs), RTC, power key, IR |

---

## 7. PMICs

Two PMICs are connected on R_TWI0 (i2c-13 bus) via the R_I2C controller.

### AXP515 — Battery Management

- **I²C address:** 0x34
- **Bus:** R_TWI0 (i2c-13)
- **Function:** Battery power supply, USB power supply, power key, DriveVBUS regulator
- **Power supplies:**
  - `bat-power-supply` — battery charging/discharging
  - `usb_power_supply` — USB VBUS input

### AXP8191 — System PMIC

- **I²C address:** 0x36
- **Bus:** R_TWI0 (i2c-13)
- **Function:** Primary system power supply

**Regulators:**

| Regulator | Role |
|-----------|------|
| DCDC1 | |
| DCDC2 | |
| DCDC3 | CPU A76 supply (phandle for cpu6, cpu7) |
| DCDC4 | |
| DCDC5 | CPU A55 supply (phandle for cpu0–cpu5) |
| DCDC6 | |
| DCDC7 | |
| DCDC8 | |
| DCDC9 | |
| ALDO1–ALDO6 | |
| BLDO1–BLDO5 | |
| CLDO1–CLDO5 | |
| DLDO1–DLDO6 | |
| ELDO1–ELDO6 | |
| DC1SW1 | |
| DC1SW2 | |
| RTC_LDO | |
| **ALDO5** | **VDD-SYS** (main SoC core supply) |
| **CLDO4** | **USB0-VBUS** |
| **ELDO3** | **USB1-VBUS** |

---

## 8. Thermal Zones

The A733 exposes 8 thermal zones through the `thermal-zone` framework. Each zone has a trip point and an associated cooling device (CPU/GPU frequency throttle).

| Zone | Name | Description | Sample Temp (C) |
|------|------|-------------|------------------|
| 1 | cpul_thermal_zone | CPU Little (A55) cluster | 67.8 |
| 2 | cpub_thermal_zone | CPU Big (A76) cluster | 64.4 |
| 3 | cpul_idle_zone | A55 idle thermal | — |
| 4 | cpub_idle_zone | A76 idle thermal | — |
| 5 | gpu_thermal_zone | GPU | 62.4 |
| 6 | npu_thermal_zone | NPU | 58.7 |
| 7 | ddr_thermal_zone | DDR memory | 64.2 |
| 8 | skin_zone | Board surface temperature | 37.6 |

Cooling devices are registered as devices 0–1 in the cooling map.

---

## 9. Power Domains

The vendor DTB defines the following power domains:

| Power Domain | Subsystem |
|--------------|-----------|
| pd_vi_test | Video Input |
| pd_ve_dec_test | Video Engine (Decode) |
| pd_ve_enc_test | Video Engine (Encode) |
| pd_npu_test | Neural Processing Unit |
| pd_gpu_top_test | GPU Top |
| pd_gpu_core_test | GPU Core |
| pd_pcie_test | PCIe |
| pd_usb2_test | USB2 |
| pd_de_sys_test | Display Engine |
| pd_vo_test | Video Output |
| pd_vo1_test | Video Output 1 |

Power domains are managed by the `sun20i-a733` power domain driver and gate individual subsystem clocks when powered down.

---

## 10. Idle States

The A733 supports two architecture-defined idle states plus per-CPU thermal-idle nodes:

| Idle State | Scope | Description |
|------------|-------|-------------|
| cpu-sleep-0 | Per-CPU | ARM core power-down via WFI/PSCI |
| cluster-sleep-0 | Per-cluster | Entire cluster power-down when all cores idle |
| thermal-idle (×6) | A55 cpus 2–7 | Thermal throttling idle node |

Idle states are used by `cpuidle` and triggered via the `arm,idle-state` DT nodes.

---

## 11. CCU Clocks and Resets

### Clocks

The CCU driver registers **126 clocks** covering all SoC subsystems. Key clock groups include:

- **CPU clocks:** CLK_CPU, PLL_CPUX
- **Bus clocks:** AHBx, APBx, AHB APB
- **Storage:** SDC0–SDC3, SMHCx
- **Display:** DE, VO, TCON0–TCON4, HDMI, DSI0, DSI1, EDP
- **Video:** VE1, VE2, VIND, CSI0–CSI2, MIPI0–MIPI2
- **Audio:** I2S0–I2S4, DMIC, OWA, audio PLL
- **Networking:** GMAC0, GMAC1
- **USB:** USB0–USB2, U2PHY, XHCI
- **Peripherals:** UART0–UART8, SPI0–SPI4, TWI0–TWI12, I2S, PWM
- **Crypto:** CE, SID
- **NPU**

### Resets

The CCU driver registers **38 reset lines** controlling subsystem reset signals.

Full clock and reset tables are available in the vendor DTB under `clocks` and `resets` nodes of the CCU node at `0x2002000`.

---

## 12. Board Integration — Orange Pi 4 Pro

This section documents the specific peripheral mapping on the Orange Pi 4 Pro carrier board.

### Serial Consoles

| Interface | Pins | Purpose |
|-----------|------|---------|
| UART0 | PH8 | Debug serial console |
| UART1 | PG6 | Serial (with RTS/CTS) |
| UART2 | PB0/PB1 | Serial |

### I²C Devices

| Bus | Controller | Address | Device | Notes |
|-----|------------|---------|--------|-------|
| i2c-5 (TWI5) | TWI5 | 0x14 | GT9271 | Touchscreen controller |
| i2c-7 (TWI7) | TWI7 | 0x10 | ES8388 | Audio codec |
| i2c-13 (S_TWI0) | R_I2C | 0x34 | AXP515 | Battery PMIC |
| i2c-13 (S_TWI0) | R_I2C | 0x36 | AXP8191 | System PMIC |
| i2c-15 | TWI15? | 0x51 | Unknown | |

### Storage

| Interface | Controller | Pins | Notes |
|-----------|------------|------|-------|
| SD card | SDC0 | PI1–PI6, CD=PI16 | Removable |
| eMMC | SDC2 | PC1–PC6 | Boot device |
| NVMe | PCIe | via Combo PHY1 | /dev/nvme0n1 |
| SPI flash | SPI0 | — | NOR flash |

### Networking

| Interface | Controller | PHY | Notes |
|-----------|------------|-----|-------|
| Ethernet | GMAC0 | RTL8211F | Gigabit |
| WiFi | SDIO (SDC1) | AIC8800 | PH2–PH7 |
| Bluetooth | UART | AIC8800 | |

### Display

| Interface | Path | Notes |
|-----------|------|-------|
| HDMI | TCON3 → HDMI | 0x5520000 |
| DSI1 MIPI LCD | DSI1 → Combo PHY | Panel via MIPI DSI |

### USB

| Port | Interface | Controller |
|------|-----------|------------|
| USB0 | OTG | UDC (0x4100000) |
| USB1 | Host | EHCI0/OHCI0 (0x4101000) |
| USB2 | Host | XHCI2 (0x6A00000) |

### Input

| Device | Interface | Address/Pin | Notes |
|--------|-----------|-------------|-------|
| Touchscreen | I2C0 (TWI5) | 0x14 | Goodix GT9271 |
| Power key | GPIO | PL2 | |
| Function key | GPIO | PB7 | |

### LEDs

| LED | GPIO | Notes |
|-----|------|-------|
| Status LED | PI8 | |

### Camera (CSI)

Camera interfaces are available via PE0–PE18 with MIPI-CSI (CSI0–CSI2) and parallel (VIND) paths.

---

## 13. Running Vendor Kernel State

The reference system was running:

- **Kernel:** `5.15.147-sun60iw2`
- **CPU governors:** All 8 CPUs online, performance governor
- **A55 max frequency:** 1794 MHz
- **A76 max frequency:** 2002 MHz

### Block Devices

| Device | Path |
|--------|------|
| NVMe | /dev/nvme0n1p1 |
| eMMC | /dev/mmcblk1p1 |

### I²C Devices (from /sys/bus/i2c/devices)

| Bus-Addr | Device |
|----------|--------|
| 13-0034 | AXP515 |
| 13-0036 | AXP8191 |
| 15-0051 | Unknown |
| 5-0014 | GT9271 Touchscreen |
| 7-0010 | ES8388 Audio Codec |

### Thermal Readings (example)

| Zone | Temperature (C) |
|------|-----------------|
| cpul_thermal_zone | 67.8 |
| cpub_thermal_zone | 64.4 |
| gpu_thermal_zone | 62.4 |
| npu_thermal_zone | 58.7 |
| ddr_thermal_zone | 64.2 |
| skin_zone | 37.6 |

Cooling devices 0–1 are active (CPU frequency throttling).

---

## Appendix: Key Physical Addresses Summary

```
0x0000000  Wakeupgen
0x1C0E000  VE1 (Video Engine)
0x1C10000  VE2 (Video Engine)
0x2000000  PIO (Pin Controller)
0x2002000  CCU
0x2020000  NSI Controller
0x2050000  Watchdog
0x2500000  UART0
0x2510000  TWI0 (I²C)
0x2520000  LEDC
0x2521000  GPADC
0x2522000  THS (Thermal Sensor)
0x2524000  LRADC
0x2525000  IRTX
0x2526000  IRRX (MIR)
0x2527000  PWM0
0x2531000  DMIC
0x2532000  I2S0
0x2540000  SPI0
0x3000000  SRAM Controller
0x3004000  Msgbox
0x3005000  HW Spinlock
0x3006000  SID (eFuse)
0x3009000  Timer
0x3120000  DMCFreq
0x3400000  GICv3
0x3600000  NPU
0x3900000  IOMMU
0x4020000  SDC0 (SD card)
0x4100000  USB UDC (OTG)
0x4101000  EHCI0
0x4200000  EHCI1
0x4500000  GMAC0
0x4510000  GMAC1
0x4520000  UFS
0x4601000  DMA
0x4603000  Crypto Engine
0x5000000  Display Engine (DE)
0x5400000  Deinterlace
0x5440000  G2D
0x5500000  VO0
0x5501000  TCON0
0x5506000  DSI0
0x5508000  DSI1
0x5520000  HDMI
0x5720000  EDP
0x5730000  TCON3 (TV)
0x5800800  VIND
0x5810100  MIPI0
0x5820000  CSI0
0x6000000  PCIe
0x6A00000  XHCI2
0x6B00000  U2PHY
0x6C00000  SerDes
0x6C01000  Combo PHY0
0x6C02000  Combo PHY1
0x7010000  R-CCU
0x7010320  NMI INTC
0x7023000  R_PWM
0x7025000  R_PIO
0x7040000  R_IRRX
0x7060000  PCK-600
0x7080000  UART7 (R_UART)
0x7081000  UART8 (R_UART)
0x7083000  S_TWI0 (R_I2C — PMICs)
0x7090000  RTC-CCU
0x7092000  R_SPI
0x8870000  CPUPLL-CCU
```
