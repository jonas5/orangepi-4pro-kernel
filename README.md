# Orange Pi 4 Pro (Allwinner A733) — Mainline Linux 6.18 Baseline

This repository provides the working mainline Linux **6.18.y** kernel baseline, device trees, patches, and automated GitHub Actions workflows for the **Orange Pi 4 Pro (Allwinner A733 / `sun60iw2`)**.

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

> **Hardware Acceleration Driver Licenses:**  
> The core Linux kernel and base Debian 12 Bookworm SD card images are 100% open source. Hardware acceleration modules (PowerVR Rogue GPU Vulkan 1.2 stack and AIC8800 Wi-Fi 6 / Bluetooth 5.4 FullMAC drivers) are maintained out-of-tree. Driver licenses can be grabbed below:  
>  
> <a href="https://buymeacoffee.com/jonas5/e/588598" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="46" style="height: 46px !important;"></a>

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

## 4. Multi-Distribution & Flavour Support

This repository supports building the full stack of **Ubuntu**, **Debian**, and **Arch Linux ARM** releases across **Minimal**, **Server**, and **Desktop (XFCE, GNOME, MATE)** flavours:

### Supported Operating System Matrix:

| Distribution Family | Release / Suite | Codename & Lifecycle | Supported Flavours |
| :--- | :--- | :--- | :--- |
| **Ubuntu** | `noble` | **Ubuntu 24.04 LTS (Noble Numbat)** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome` |
| **Ubuntu** | `jammy` | **Ubuntu 22.04 LTS (Jammy Jellyfish)** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome` |
| **Ubuntu** | `focal` | **Ubuntu 20.04 LTS (Focal Fossa)** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome` |
| **Debian** | `bookworm` | **Debian 12 (Bookworm) — Stable** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome`, `desktop-mate` |
| **Debian** | `trixie` | **Debian 13 (Trixie) — Testing** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome`, `desktop-mate` |
| **Debian** | `bullseye` | **Debian 11 (Bullseye) — Oldstable** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome`, `desktop-mate` |
| **Debian** | `sid` | **Debian Unstable (Sid) — Rolling** | `minimal`, `server`, `desktop-xfce`, `desktop-gnome` |
| **Arch Linux** | `arch` | **Arch Linux ARM (ALARM) — Rolling** | `minimal` / full `pacman` base |

---

### Flavour Profiles & Editions:
- **`minimal`**: Minimal headless CLI server (ultra-lightweight, small storage footprint, fast boot time).
- **`server`**: Standard headless server with full diagnostics, networking tools, and Docker readiness.
- **`desktop-xfce`**: Highly responsive XFCE desktop environment (**SBC community favorite**).
- **`desktop-gnome`**: Full GNOME Wayland desktop environment with native hardware compositing.
- **`desktop-mate`**: Classic, traditional MATE desktop environment (available for Debian).

### Default Credentials:
| Distribution Family | Default User | Default Password | Root Login |
| :--- | :--- | :--- | :--- |
| **Ubuntu (`noble`, `jammy`, `focal`)** | `orangepi` | `orangepi` (sudo) | `root` / `orangepi` |
| **Debian (`bookworm`, `trixie`, `bullseye`)** | `orangepi` | `orangepi` (sudo) | `root` / `orangepi` |
| **Arch Linux ARM (`arch`)** | `alarm` | `alarm` (sudo) | `root` / `root` |

---

## 5. Building with GitHub Actions CI

The unified daily pipeline (`.github/workflows/daily.yml`) supports on-demand builds for any distribution and flavour:

1. Open this repository on GitHub.
2. Go to the **Actions** tab.
3. In the left sidebar, select **Daily Build & Release**.
4. Click **Run workflow**:
   - Choose your **Target Distribution & Release** (`noble`, `bookworm`, `jammy`, `trixie`, `arch`, etc.).
   - Choose your **Flavour / Edition** (`minimal`, `server`, `desktop-xfce`, `desktop-gnome`, `desktop-mate`).
   - Click **Run workflow**.
5. Once complete, your bootable `.img.xz` image, kernel packages, and checksums will be published to the GitHub Releases tab and workflow artifacts.

---

## 6. Local Build Instructions

### Using the Unified Local Image Builder (`scripts/build-os-image.sh`):
You can build any Ubuntu, Debian, or Arch Linux image directly on your local machine:

```bash
# 1. Build Ubuntu 24.04 LTS (Noble) with XFCE Desktop:
sudo bash scripts/build-os-image.sh noble desktop-xfce

# 2. Build Ubuntu 24.04 LTS (Noble) Minimal Headless Server:
sudo bash scripts/build-os-image.sh noble minimal

# 3. Build Debian 12 (Bookworm) with GNOME Desktop:
sudo bash scripts/build-os-image.sh bookworm desktop-gnome

# 4. Build Debian 12 (Bookworm) Standard Server:
sudo bash scripts/build-os-image.sh bookworm server

# 5. Build Arch Linux ARM Rolling SD Image:
sudo bash scripts/build-os-image.sh arch
```

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
