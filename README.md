# Orange Pi 4 Pro (Allwinner A733) — Mainline Linux 6.18 Baseline

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Get%20Driver%20License%20(€35.59)-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/jonas5/e/588598)
[![Driver License Portal](https://img.shields.io/badge/Driver%20Portal-raccoon.ing-0284c7?style=for-the-badge)](https://raccoon.ing)
[![Kernel Version](https://img.shields.io/badge/Kernel-6.18.y%20Mainline-success?style=for-the-badge)](https://kernel.org)
[![Debian](https://img.shields.io/badge/OS-Debian%2012%20Bookworm-D70A53?style=for-the-badge&logo=debian&logoColor=white)](https://debian.org)

This repository provides the working mainline Linux **6.18.y** kernel baseline, device trees, patches, and automated GitHub Actions workflows for the **Orange Pi 4 Pro (Allwinner A733 / `sun60iw2`)**.

---

## 🛒 Private Driver Pack & Hardware Licensing

The core Linux kernel (`linux-6.18.y`), device trees, and base bootable Debian 12 Bookworm SD card images in this repository are **100% open-source and clean**.

To obtain the licensed, out-of-tree hardware acceleration driver subsystems (**PowerVR Rogue BXM-4-64 GPU Mesa Vulkan 1.2 stack** and **AIC8800D80 FullMAC Wi-Fi 6 & Bluetooth 5.4 drivers**), hardware licenses are available on Buy Me a Coffee:

- **Purchase Driver Pack License**: [https://buymeacoffee.com/jonas5/e/588598](https://buymeacoffee.com/jonas5/e/588598) (€35.59)
- **Official Driver & License Portal**: [https://raccoon.ing](https://raccoon.ing)
- **Instant Activation & Receipt Lookup**: [https://raccoon.ing/success](https://raccoon.ing/success)

### How to Activate Your Drivers Once Purchased:

#### 1. Interactive TUI (Classic `raspi-config` Style):
Launch the configuration tool pre-installed on the system:
```bash
sudo orangepi-config
# or via alias:
sudo driver-config
```
Select **Option 1** (AIC8800 Wi-Fi 6 / BT 5.4), **Option 2** (PowerVR Rogue GPU Vulkan 1.2), or **Option 3** (All Modules).

#### 2. Non-Interactive 1-Click Terminal Activation:
```bash
sudo orangepi-config --activate all --payment-id <YOUR-LICENSE-KEY> --non-interactive
```

#### 3. Offline USB Sneakernet Installation (No Wi-Fi / No Network):
If your freshly imaged board does not have an active network connection, prepare an offline USB bundle on any machine with internet access:
```bash
orangepi-config --activate all --payment-id <YOUR-LICENSE-KEY> --download-only --output-dir /media/usb
```
Then plug the USB drive into your Orange Pi 4 Pro and run:
```bash
cd /media/usb
sudo bash install-offline.sh
sudo reboot
```

---

## 1. Hardware & System Status

| Component | Hardware Specification | Linux 6.18 Status | Driver Architecture |
| :--- | :--- | :--- | :--- |
| **CPU / SoC** | Allwinner A733 (6× Cortex-A55, 2× Cortex-A76) | **Working** | Mainline DT, CCU clocks, PCK-600 power domain controller |
| **Ethernet** | Motorcomm YT8531 Gigabit PHY | **Working** | `stmmac.ko` + `motorcomm.ko` (EEE & auto-sleep disabled, 0% packet loss) |
| **MicroSD** | SDC0 (`mmc@4020000`) | **Working** | Locked as `/dev/mmcblk0p1` via DT `aliases { mmc0 = &mmc0; }` |
| **HDMI Display** | Allwinner Display Engine (`sunxi-drm`) | **Working** | Kernel CMA set to `256 MiB` (`cma=256M`), `/dev/dri/card0` |
| **GPU / Vulkan** | PowerVR Rogue BXM-4-64 (`36.56.104.183`) | **Working** | `powervr.ko` on `/dev/dri/renderD128` + Mesa open-source Vulkan driver |
| **Wi-Fi** | AIC8800D80 SDIO 4-bit (`mmc@4021000`) | **Working** | Separate module on demand (`wlan0`, 802.11ax Wi-Fi 6) |
| **Bluetooth** | AIC8800D80 UART1 (`serial@2501000`) | **Working** | Native H4 1.5MBaud hardware flow control daemon (`hci0`) |
| **Power Mgmt** | Desktop / GDM3 / Systemd | **Working** | Sleep & suspend targets permanently masked for 24/7 SBC server operation |

---

## 2. Design Principles: Modular Base Image

To enable clean, reproducible builds in GitHub Actions workflows without bundling out-of-tree or vendor drivers into the kernel image:

1. **Clean Kernel Image (`Image`)**:
   Only the essential components required to boot straight into a Debian Bookworm root shell are built into the kernel (`=y`):
   - Core CPU architecture, timers, GICv3 interrupt controller
   - SoC clock tree (`sun60iw2-ccu`), resets, and pinctrl
   - Power domain controller (`allwinner,sun60i-a733-pck-600`)
   - SD card controller (`CONFIG_MMC_SUNXI=y`, `CONFIG_MMC_BLOCK=y`)
   - Root filesystem (`CONFIG_EXT4_FS=y`)
   - Serial console (`CONFIG_SERIAL_8250=y`)

2. **Peripheral Drivers as Separate Modules (`=m`)**:
   All peripheral drivers are separate loadable kernel modules (`.ko`):
   - `CONFIG_STMMAC_ETH=m`, `CONFIG_DWMAC_SUNXI=m`, `CONFIG_MOTORCOMM_PHY=m`
   - `CONFIG_DRM_POWERVR=m`
   - `CONFIG_AIC_WLAN_SUPPORT=m`, `CONFIG_AIC8800_SDIO=m`, `CONFIG_AIC8800_BTLPM_SUPPORT=m`
   - `CONFIG_SND_SOC_SUNXI_*=m`

3. **Out-of-Tree & Proprietary Driver Separation**:
   - The AIC8800 BSP driver source (`bsp/`) and bring-up daemons are excluded via `.gitignore`.
   - Advanced hardware bring-up scripts and proprietary components are packaged separately in a private driver pack, intended to be signed and bound to the device's hardware ID.

---

## 3. Repository Structure

```text
├── .github/
│   └── workflows/
│       ├── build-kernel.yml      # CI workflow: cross-compiles 6.18 kernel, DTBs & modules
│       └── build-image.yml       # CI workflow: builds bootable Debian Bookworm SD image (.img.xz)
├── boot/
│   ├── boot.cmd                  # U-Boot boot script source
│   ├── boot.scr                  # Compiled U-Boot script
│   └── orangepiEnv.txt           # Boot arguments (cma=256M, panic=5, rootdev=/dev/mmcblk0p1)
├── config/
│   └── orangepi_4pro_defconfig   # Modular Linux 6.18 kernel configuration
├── dts/
│   ├── sun60i-a733-orangepi-4-pro.dts  # Board device tree
│   ├── sun60iw2p1.dtsi                 # SoC device tree
│   ├── sun60iw2p1-cpu-vf.dtsi          # CPU voltage-frequency table
│   └── include/dt-bindings/            # Device tree binding headers
├── patch/
│   └── 0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch # Self-contained mainline 6.18 patch
└── README.md
```

---

## 4. Building with GitHub Actions

### Workflow 1: Build Debian Bookworm SD Image (`build-image.yml`)
- Go to the **Actions** tab in GitHub.
- Select **Build Orange Pi 4 Pro Debian Bookworm Image**.
- Click **Run workflow** (select `bookworm` and `minimal` or `desktop`).
- Downloads the resulting `orangepi4pro-bookworm-image.img.xz` artifact once complete.

### Workflow 2: Cross-compile Kernel & Modules (`build-kernel.yml`)
- Trigger **Build Mainline 6.18 Kernel for Orange Pi 4 Pro**.
- Builds `Image`, `uImage`, DTBs, and `modules_6.18.y.tar.gz`.

---

## 5. Local Build Instructions

### Cross-compiling the Kernel Manually:
```bash
# 1. Clone mainline kernel 6.18.y
git clone --depth 1 --branch linux-6.18.y https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git linux-6.18.y
cd linux-6.18.y

# 2. Apply the unified patch
git apply ../patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch

# 3. Configure
cp ../config/orangepi_4pro_defconfig .config
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig

# 4. Compile Image, DTBs, and Modules
make -j$(nproc) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image dtbs modules
```

---

## 6. License
Kernel patches and device trees are dual-licensed under GPL-2.0 and MIT.
EOF
