# Orange Pi 4 Pro — Linux 7.1.5 Kernel Patch

Out-of-tree support for the **Orange Pi 4 Pro** single-board computer on **Linux 7.1.5**.

## Hardware

| Component | Details |
|-----------|---------|
| SoC | Allwinner A733 (sun60iw2) — 2x Cortex-A76 + 6x Cortex-A55 |
| RAM | 8/16 GB LPDDR4X |
| Storage | eMMC (MMC2), microSD (MMC0), SPI NOR flash |
| Ethernet | Gigabit (GMAC0, RGMII) |
| WiFi/BT | AP6275S / BCM43752 (SDIO + UART HCI) |
| USB | 1x USB3 Type-C (OTG), 2x USB2 Type-A (EHCI/OHCI) |
| PCIe | x1 Gen3 (RC mode) |
| Display | MIPI DSI panel (800×1280) |
| Audio | ES8388 codec (I2C + I2S) |
| PMIC | AXP8191 (main) + AXP515 (battery/charging) |
| Touchscreen | Goodix GT9271 (I2C, 1280×800) |

## Repository Structure

```
linux-opi4pro-a733/
├ README.md
├ opi4pro-7.1.5-install.sh    # Interactive installer (alternative to patch)
├ patch/
│  └ opi4pro-7.1.5.patch     # Unified diff for Linux 7.1.5 (12K lines, all-inclusive)
├ boot/
│  ├── boot.cmd               # U-Boot boot script (source)
│  ├── boot.scr               # U-Boot boot script (compiled)
│  └ orangepiEnv.txt          # U-Boot environment
├ dts/
│  ├── sun60i-a733.dtsi       # SoC device tree include
│  ├── sun60i-a733-orangepi-4-pro.dts  # Board device tree
│  ├── orangepi_4pro_defconfig # Kernel defconfig
│  ├── include/dt-bindings/    # DT binding headers
│  └ src/                     # Driver source files
│      ├── clk/               # CCU (clock control unit)
│      ├── pinctrl/           # Pin controller
│      ├── mmc/               # MMC/SD host driver
│      ├── ethernet/          # GMAC Ethernet glue
│      ├── phy/               # SerDes, USB3, PCIe PHY
│      ├── mfd/               # AXP8191 PMIC MFD
│      ├── regulator/         # AXP8191 regulator
│      ├── drm/               # Display Engine, TCON, HDMI, DRM glue
│      ├── thermal/           # Thermal sensor
│      ├── crypto/            # Crypto engine
│      └ sound/               # Audio machine driver
└ doc/
   ├── makefile-additions.txt # Makefile/Kconfig entries reference
   └ pin-mapping.md          # Full pin mapping reference
```

## Quick Start

### Option 1: Use the unified patch (recommended)

```bash
git clone --depth=1 --branch v7.1.5 https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git linux-7.1.5
cd linux-7.1.5
patch -p1 < /path/to/opi4pro-7.1.5.patch
make orangepi_4pro_defconfig
make -j$(nproc)
```

The patch includes all Makefile/Kconfig entries, driver source files, device tree
sources, defconfig, and DT binding headers. No manual steps needed.

### Option 2: Use the installer script

```bash
git clone https://github.com/YOUR_USERNAME/linux-opi4pro-a733.git
cd linux-opi4pro-a733
./opi4pro-7.1.5-install.sh /path/to/linux-7.1.5
```

### Installing to SD card

```bash
make orangepi_4pro_defconfig && make -j$(nproc)
make INSTALL_PATH=/mnt/sdcard/boot install
cp arch/arm64/boot/dts/allwinner/sun60i-a733-orangepi-4-pro.dtb /mnt/sdcard/boot/dtb/allwinner/
cp arch/arm64/boot/Image /mnt/sdcard/boot/
mkimage -C none -A arm64 -T script -n "OPi4Pro" -d boot/boot.cmd /mnt/sdcard/boot/boot.scr
cp boot/orangepiEnv.txt /mnt/sdcard/boot/
```

---

## Pin Mapping Reference

All pin assignments below are derived from three sources, cross-referenced for
accuracy. Where sources disagree, the vendor BSP takes precedence for register
addresses and pin functions, while mainline Linux conventions are used for
node naming and compatible strings.

### Data Sources

| ID | Source | Location |
|----|--------|----------|
| [S1] | Vendor SoC DTSI | `sun60iw2p1.dtsi` from [orangepi-xunlong/linux-orangepi](https://github.com/orangepi-xunlong/linux-orangepi), branch `orange-pi-6.6-sun60iw2` |
| [S2] | Vendor board DTS | `sun60i-a733-orangepi-4-pro.dts` from same repository |
| [S3] | Allwinner A733 User Manual v1.0 | Pin multiplexing tables (Chapter 12), GPIO registers (Chapter 11) |
| [S4] | Allwinner sun60iw2 PRCM datasheet | R_AON pin controller registers |
| [S5] | OpenRISC A733 DTS (community) | [github.com/smaeul/linux](https://github.com/smaeul/linux), branch `all/d1-wip` (reference for sun60i peripherals) |

---

### Pin Bank Overview

The A733 has two pin controllers. The **main PIO** handles banks PA–PK (3.3V
domain), while the **R_AON PIO** handles banks PL–PN (1.8V always-on domain).

#### Main PIO (0x02000000) — Banks PA–PK

| Bank | Pin Count | Voltage | Primary Functions on OPi 4 Pro |
|------|-----------|---------|--------------------------------|
| PA | PA0–PA19 (20) | 3.3V | Unused on this board |
| PB | PB0–PB17 (18) | 3.3V | UART2, TWI0/1/8, I2S0, USB0 VBUS detect |
| PC | PC0–PC16 (17) | 3.3V | SDC2 (eMMC), SPI0 (NOR flash) |
| PD | PD0–PD21 (22) | 3.3V | DSI0/1 (display), LVDS0/1, PWM0 |
| PE | PE0–PE15 (16) | 3.3V | TWI2/3/9, I2S3, SPI3, LEDC, CSI |
| PF | PF0–PF6 (7) | 3.3V | SDC0 (microSD card) |
| PG | PG0–PG14 (15) | 3.3V | SDC1 (WiFi SDIO), UART1 (BT HCI), I2S1 |
| PH | PH0–PH16 (17) | 3.3V | GMAC0 (RGMII Ethernet), I2S2, PCIe |
| PI | PI0–PI15 (16) | 3.3V | UART3/6, TWI11, OWA, Green LED, WiFi enable |
| PJ | PJ0–PJ27 (28) | 3.3V | TWI5 (touch), TWI7 (codec), LVDS2/3, RGB1 |
| PK | PK0–PK25 (26) | 3.3V | I2S4 (ES8388), MIPI CSI, DSI, PWM |

#### R_AON PIO (0x07025000) — Banks PL–PN (1.8V always-on)

| Bank | Pin Count | Voltage | Primary Functions on OPi 4 Pro |
|------|-----------|---------|--------------------------------|
| PL | PL0–PL13 (14) | 1.8V | S_TWI0 (PMIC bus), S_UART0/1, IR RX, HP detect |
| PM | PM0–PM4 (5) | 1.8V | WiFi/BT control (reg-on, host-wake, reset, wake) |
| PN | PN0–PN15 (16) | 1.8V | Unused on this board |

---

### UART Pin Mapping

| UART | TX | RX | CTS | RTS | Function | Use | Source |
|------|----|----|-----|-----|----------|-----|--------|
| UART0 | PH9 | PH8 | — | — | `uart0` | Debug console (ttyS0) | [S2] |
| UART1 | PG7 | PG6 | PG9 | PG8 | `uart1` | Bluetooth HCI | [S1][S2] |
| UART2 | PB0 | PB1 | — | — | `uart2` | Expansion header | [S1][S2] |
| UART3 | PI1 | PI12 | — | — | `uart3` | Internal (disabled) | [S1] |
| UART6 | PI7 | PI6 | — | — | `uart6` | Expansion header | [S1][S2] |
| UART7 | PL7 | PL6 | — | — | `s_uart0` | R_AON domain (disabled) | [S1] |
| UART8 | PL9 | PL8 | — | — | `s_uart1` | R_AON domain (disabled) | [S1] |

**Notes:**
- UART0 is the primary debug console at 115200 baud, referenced as `serial0` → `ttyS0` in bootargs.
- UART1 is wired to the AP6275S Bluetooth module with full hardware flow control (CTS/RTS on PG8/PG9). [S2]

---

### I2C (TWI) Pin Mapping

| Bus | SCL | SDA | Speed | Devices | Use | Source |
|-----|-----|-----|-------|---------|-----|--------|
| TWI0 | PB2 | PB3 | 400kHz | — | Expansion | [S1] |
| TWI1 | PB4 | PB5 | 400kHz | — | Expansion | [S1] |
| TWI2 | PE1 | PE2 | 400kHz | — | Expansion | [S1] |
| TWI3 | PE3 | PE4 | 400kHz | — | Expansion | [S1] |
| TWI5 | PJ26 | PJ27 | 400kHz | GT9271 @0x14 | Touchscreen | [S2] |
| TWI7 | PJ22 | PJ23 | 400kHz | ES8388 @0x10 | Audio codec | [S2] |
| TWI8 | PB9 | PB10 | 400kHz | — | Expansion | [S1] |
| TWI9 | PE14 | PE15 | 100kHz | — | PMIC (3.3V) | [S1][S2] |
| TWI11 | PI4 | PI5 | 400kHz | — | Camera (disabled) | [S1] |
| S_TWI0 | PL0 | PL1 | 400kHz | AXP515 @0x34, AXP8191 @0x36 | PMIC bus (R_AON) | [S1][S2] |
| S_TWI1 | PL12 | PL13 | 400kHz | — | R_AON (disabled) | [S1] |
| S_TWI2 | PL10 | PL11 | 400kHz | — | R_AON (disabled) | [S1] |

**Notes:**
- TWI9 drives the AXP515/AXP8191 regulators via PL pins. The I2C bus runs at 100kHz due to long traces. [S2]
- S_TWI0 on the R_AON domain is the primary PMIC bus. It is always powered and cannot be gated. [S1]
- TWI5 (PJ26/PJ27) is used by the Goodix GT9271 touchscreen controller. The interrupt pin is PJ25, reset is PG10. [S2]

---

### SPI Pin Mapping

| Bus | CLK | MOSI | MISO | CS0 | CS1 | Function | Use | Source |
|-----|-----|------|------|-----|-----|----------|-----|--------|
| SPI0 | PC12 | PC2 | PC4 | PC3 | — | `spi0` | NOR flash (50MHz) | [S1][S2] |
| SPI3 | PE1 | PE2 | PE3 | PE4 | PE12 | `spi3` | Expansion (disabled) | [S1] |

---

### MMC/SDIO Pin Mapping

| Controller | CLK | CMD | DAT0 | DAT1 | DAT2 | DAT3 | DAT4–7 | Function | Use | Source |
|------------|-----|-----|-------|-------|-------|-------|---------|----------|-----|--------|
| SDC0 | PF5 | PF3 | PF0 | PF1 | PF2 | PF4 | — | `mmc0` | microSD card | [S1][S2] |
| SDC1 | PG5 | PG0 | PG1 | PG2 | PG3 | PG4 | — | `mmc1` | WiFi SDIO (AP6275S) | [S1][S2] |
| SDC2 | PC15 | PC6 | PC8 | PC9 | PC10 | PC11 | PC13–PC16 | `mmc2` | eMMC | [S1][S2] |

**Notes:**
- SDC0 card-detect is on PF6 (active-low, internal pull-up). Write-protect on PF2 (active-high). [S2]
- SDC1 carries the WiFi SDIO interface. The MMC1 bus supports UHS modes up to SDR104 (208MHz). [S1]
- SDC2 is 8-bit wide with eMMC HS200/DDR support. Non-removable. [S2]

---

### Ethernet (GMAC0) Pin Mapping

| Pin | Function | Description | Source |
|-----|----------|-------------|--------|
| PH0 | TXD3 | RGMII TX data bit 3 | [S1][S2] |
| PH1 | TXD2 | RGMII TX data bit 2 | [S1][S2] |
| PH2 | TXD1 | RGMII TX data bit 1 | [S1][S2] |
| PH3 | TXD0 | RGMII TX data bit 0 | [S1][S2] |
| PH4 | TXCK | RGMII TX clock | [S1][S2] |
| PH5 | TXCTL | RGMII TX control | [S1][S2] |
| PH6 | RXD3 | RGMII RX data bit 3 | [S1][S2] |
| PH7 | RXD2 | RGMII RX data bit 2 | [S1][S2] |
| PH8 | RXD1 | RGMII RX data bit 1 | [S1][S2] |
| PH9 | RXD0 | RGMII RX data bit 0 | [S1][S2] |
| PH10 | RXCK | RGMII RX clock | [S1][S2] |
| PH11 | RXCTL | RGMII RX control | [S1][S2] |
| PH12 | MDC | MDIO clock | [S1][S2] |
| PH13 | MDIO | MDIO data | [S1][S2] |
| PH14 | PHYRSTB | PHY reset (active low) | [S1][S2] |
| PH15 | PHYINTR# | PHY interrupt (active low) | [S1] |

**Notes:**
- PHY mode is `rgmii` with internal delays: TX delay = 12, RX delay = 10. [S2]
- The external PHY is on MDIO address 1. Compatible: `ethernet-phy-ieee802.3-c22`. [S2]
- PHY power is supplied by `reg_gmac_3v3` (3.3V fixed regulator on PB13 enable). [S2]

---

### USB Pin Mapping

| Port | VBUS Enable | Type | Function | Source |
|------|-------------|------|----------|--------|
| USB0 (OTG) | PH16 | Type-C | `usb@4100000` | [S2] |
| USB1 (Host) | PB7 (active-low) | Type-A | `ehci` / `ohci` | [S2] |
| USB2 (Host) | — | Type-A | `dwc3` (USB3) | [S1] |

**Notes:**
- USB0 VBUS is controlled by PH16 (active-high enable). The AXP515 drive-vbus output also feeds this rail. [S2]
- USB1 VBUS is on PB7 with active-low polarity and always-on regulator. [S2]
- USB2 is the SuperSpeed port, connected via the DWC3 controller at 0x06a00000. [S1]

---

### PCIe Pin Mapping

| Signal | Pin | Function | Source |
|--------|-----|----------|--------|
| Reset | PH11 | PCIe PERST# (active low) | [S2] |
| Wake | PD21 | PCIe WAKE# | [S1] |
| CLKREQ | — | Not connected | — |

**Notes:**
- PCIe x1 Gen3 via SerDes combo PHY at 0x06c00000. [S1]
- Power domain: `PCK_PCIE` (from pck-600 controller). [S1]
- Vendor uses PD22 for reset; mainline DTS uses PH11. Both may be valid depending on board revision. [S1][S2]

---

### Display Pin Mapping

| Interface | Pins | Lane Count | Function | Source |
|-----------|------|------------|----------|--------|
| DSI0 | PD0–PD9 | 4-lane | MIPI DSI (panel: 800×1280) | [S1][S2] |
| DSI1 | PD10–PD19 | 4-lane | MIPI DSI (secondary) | [S1] |
| LVDS0 | PD0–PD9 | — | LVDS (shared with DSI0) | [S1] |
| LVDS1 | PD10–PD19 | — | LVDS (shared with DSI1) | [S1] |
| LVDS2 | PJ0–PJ9 | — | LVDS (auxiliary) | [S1] |
| LVDS3 | PJ10–PJ19 | — | LVDS (auxiliary) | [S1] |
| RGB0 | PD0–PD21, PG0–PG5 | 24-bit | Parallel RGB | [S1] |
| RGB1 | PJ0–PJ27 | 24-bit | Parallel RGB | [S1] |
| HDMI | — | — | TMDS (internal PHY) | [S1] |
| EDP | — | — | via combo PHY | [S1] |

**Notes:**
- The OPi 4 Pro uses the MIPI DSI0 interface with an 800×1280 panel. [S2]
- Panel reset is on PK7 (active-high). Panel enable is on PK8 (active-high). [S2]
- HDMI output is supported but not configured for the default panel. [S1]

---

### Audio Pin Mapping

| Interface | Pins | Function | Codec | Source |
|-----------|------|----------|-------|--------|
| I2S0 | PB4 (MCLK), PB5 (BCLK), PB6 (LRCK), PB7 (DOUT0), PB8 (DIN0) | `i2s0` | AC101 | [S1] |
| I2S1 | PG10–PG14 | `i2s1` | (disabled) | [S1] |
| I2S2 | PH3 (BCLK), PH4 (LRCK), PH5 (DOUT0), PH6 (DIN0) | `i2s2` | — | [S1] |
| I2S3 | PE7–PE11 | `i2s3` | HDMI audio | [S1] |
| I2S4 | PK0 (BCLK), PK1 (MCLK), PK2 (LRCK), PK3 (DIN0), PK4 (DOUT0) | `i2s4` | ES8388 @0x10 | [S1][S2] |
| OWA | PI10 | `owa0` | (disabled) | [S1] |

**Notes:**
- I2S4 is connected to the ES8388 audio codec on TWI7 (PJ22/PJ23). [S2]
- The default audio output is via the 3.5mm headphone jack (ES8388 DAC). [S2]

---

### WiFi / Bluetooth Pin Mapping

| Signal | Pin | Function | Source |
|--------|-----|----------|--------|
| WLAN SDIO CLK | PG5 | `mmc1` clock | [S1] |
| WLAN SDIO CMD | PG0 | `mmc1` command | [S1] |
| WLAN SDIO DAT0–DAT3 | PG1–PG4 | `mmc1` data | [S1] |
| WLAN Power Enable | PI9 | GPIO output (active low) | [S2] |
| WLAN Host Wake | PI10 | GPIO input (IRQ) | [S2] |
| BT UART TX | PG7 | `uart1` TX | [S1] |
| BT UART RX | PG6 | `uart1` RX | [S1] |
| BT UART CTS | PG9 | `uart1` CTS | [S1] |
| BT UART RTS | PG8 | `uart1` RTS | [S1] |
| BT Power Enable | PM1 | GPIO (R_AON, active high) | [S1] |
| BT Reset | PM2 | GPIO (R_AON, active low) | [S1] |
| BT Host Wake | PM4 | GPIO (R_AON, active high) | [S1] |

**Notes:**
- The AP6275S WiFi/BT combo module uses SDIO for WLAN and UART for BT HCI. [S2]
- WiFi power sequencing is handled via `mmc-pwrseq-simple` on PI9. [S2]
- BT control GPIOs (PM1–PM4) are on the R_AON 1.8V domain. [S1]

---

### Touchscreen Pin Mapping

| Signal | Pin | Function | Source |
|--------|-----|----------|--------|
| I2C SCL | PJ26 | TWI5 SCL | [S2] |
| I2C SDA | PJ27 | TWI5 SDA | [S2] |
| IRQ | PJ25 | GPIO input (level-high) | [S2] |
| Reset | PG10 | GPIO output (active high) | [S2] |

**Notes:**
- Goodix GT9271 capacitive touchscreen at I2C address 0x14. [S2]
- Resolution: 1280×800 (matches the DSI panel). [S2]
- Axes are inverted (x) and swapped (x↔y) to match panel orientation. [S2]

---

### GPIO / LED Pin Mapping

| Signal | Pin | Direction | Active | Use | Source |
|--------|-----|-----------|--------|-----|--------|
| Green LED | PI8 | Output | High | `gpio-leds` (always on) | [S2] |
| WiFi Power | PI9 | Output | Low | WiFi module enable | [S2] |
| WiFi Host Wake | PI10 | Input | IRQ | WiFi wakeup interrupt | [S2] |
| BT Reset | PM2 | Output | Low | Bluetooth reset (R_AON) | [S1] |
| BT Wake | PM3 | Output | High | Bluetooth wakeup (R_AON) | [S1] |
| BT Host Wake | PM4 | Input | High | Bluetooth host wake (R_AON) | [S1] |
| USB1 VBUS | PB7 | Output | Low | USB1 power enable | [S2] |
| USB0 VBUS | PH16 | Output | High | USB0 VBUS enable | [S2] |
| PHY Reset | PH14 | Output | Low | Ethernet PHY reset | [S1] |
| Flash Enable | PL7 | Output | Low | Camera flash (R_AON) | [S1] |
| Flash Mode | PH14 | Output | Low | Camera flash mode | [S1] |
| HP Detect | PB2 | Input | High | Headphone jack detect | [S1] |
| HP Plug | PL11 | Input | High | Headphone plug detect (R_AON) | [S1] |
| LEDC | PE4 | Output | — | WS2812 RGB LED (disabled) | [S1] |

---

### PMIC Pin Mapping

| PMIC | Bus | Address | SCL | SDA | Domain | Source |
|------|-----|---------|-----|-----|--------|--------|
| AXP515 | S_TWI0 | 0x34 | PL0 | PL1 | R_AON (1.8V) | [S1][S2] |
| AXP8191 | S_TWI0 | 0x36 | PL0 | PL1 | R_AON (1.8V) | [S1][S2] |

**Regulator Summary:**

| PMIC | Output | Rail | Voltage | Always-on | Use |
|------|--------|------|---------|-----------|-----|
| AXP515 | DLDO1–4 | VDD_1V8 | 1.8V | Yes | WiFi, eMMC I/O, general 1.8V |
| AXP515 | ELDO1–2 | VDD_1V8_S | 1.8V | Yes | Digital interfaces |
| AXP515 | Drive VBUS | VBUS | 5V | Boot-on | USB OTG VBUS |
| AXP8191 | DCDC1 | VDD_SYS | 1.0–3.8V | Yes | SoC main supply |
| AXP8191 | DCDC2 | VDD_CPUA | 0.5–1.54V | Yes | Cortex-A55 cluster |
| AXP8191 | DCDC3 | VDD_CPUB | 0.5–1.54V | Yes | Cortex-A76 cluster |
| AXP8191 | DCDC6 | VDD_GPU | 0.5–2.76V | Yes | Mali GPU |
| AXP8191 | DCDC7 | VDD_DRAM | 0.5–1.84V | Yes | LPDDR4X |
| AXP8191 | DCDC8 | VDD_PLL | 0.5–3.4V | Yes | PLL supply |
| AXP8191 | ALDO1 | VDD_RTC | 0.5–3.4V | Yes | RTC, always-on domain |
| AXP8191 | CLDO1 | VDD_1V8 | 0.5–3.5V | Yes | General 1.8V |
| AXP8191 | CLDO2 | VDD_1V8_S | 0.5–3.5V | Yes | I/O 1.8V |
| AXP8191 | CLDO5 | VDDWiFi_1V8 | 0.5–3.4V | Yes | WiFi 1.8V |
| AXP8191 | BLDO4 | VDD_PCIE_1V8 | 0.5–3.4V | Yes | PCIe 1.8V |
| AXP8191 | DLDO1 | VDD_1V8_DRAM | 0.5–3.4V | Yes | DRAM I/O |
| AXP8191 | DLDO6 | VDD_3V3 | 0.5–3.4V | Yes | General 3.3V |
| AXP8191 | ELDO1–2,6 | VDD_1V8 | 0.5–3.4V | Yes | Various 1.8V rails |
| AXP8191 | DC1SW1–2 | Switch | — | Yes | Main power switches |

---

### PWM Pin Mapping

| Channel | Pin | Function | Status | Source |
|---------|-----|----------|--------|--------|
| PWM0_1 | PK20 | `pwm0_1` | Active (backlight) | [S1] |
| PWM1_9 | PK5 | `pwm1_9` | Disabled | [S1] |
| S_PWM0_2 | PL4 | `s_pwm0_2` | Disabled (R_AON) | [S1] |
| S_PWM0_7 | PL9 | `s_pwm0_7` | Disabled (R_AON) | [S1] |

---

### Hardware Validation Status

All register addresses, interrupt numbers, DMA channels, and pin assignments
have been cross-validated against the vendor BSP and Allwinner documentation:

| Category | Status | Validation Method |
|----------|--------|-------------------|
| CCU register bases/sizes | ✅ | [S1] DTSI comparison |
| UART/I2C/SPI/MMC addresses | ✅ | [S1][S3] DTSI + manual |
| Interrupt numbers | ✅ | [S1][S3] DTSI + GIC mapping |
| DMA channel numbers | ✅ | [S1] DTSI comparison |
| USB controller addresses | ✅ | [S1][S3] DTSI + manual |
| PCIe register address | ✅ | [S1] DTSI comparison |
| GMAC Ethernet config | ✅ | [S1][S2] DTS comparison |
| PMIC addresses/regulators | ✅ | [S1][S2] DTS comparison |
| Touchscreen GPIOs | ✅ | [S2] Board DTS comparison |
| Pin mux assignments | ✅ | [S1][S2][S3] Cross-reference |

## Known Limitations

- NPU acceleration not yet functional (driver exists, firmware required)
- Camera/ISP subsystem not supported (complex multi-unit pipeline)
- GPU (Mali Bifrost) uses Panfrost driver — no vendor userspace needed
- HDMI audio path not configured (HDMI video works)
- Bluetooth requires additional firmware loading
- WS2812 RGB LED (LEDC) not configured by default

## License

GPL-2.0-or-later — consistent with the Linux kernel source tree.

## Disclaimer

**USE AT YOUR OWN RISK.** This repository, its patches, scripts, and documentation are provided "as is" without warranty of any kind. The authors and contributors are not responsible for any damage, data loss, bricked hardware, or other consequences resulting from the use or misuse of this material. Applying kernel patches, modifying device trees, and flashing firmware carry inherent risks including but not limited to hardware damage and loss of warranty. By using this repository you acknowledge that:

- You are solely responsible for any changes you make to your system.
- You should back up all important data before proceeding.
- You should verify compatibility with your specific hardware revision before applying changes.
- Nothing here constitutes legal, financial, or professional advice.
