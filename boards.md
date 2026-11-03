# Supported Boards

This document tracks known Allwinner A733-based hardware targeted by the **Allwinner-A733-kernel** project, along with current validation and support status.

## Project Goal

The objective of this project is to provide and maintain modern Linux support for the **Allwinner A733 platform** rather than a single board implementation.

By validating support across multiple vendors' hardware, platform-level improvements can be identified and shared throughout the A733 ecosystem.

## Support Status

| Board | Vendor | SoC | Status | Notes |
|---------|---------|---------|---------|---------|
| Orange Pi 4 Pro | Orange Pi / Xunlong | Allwinner A733 | ✅ Validated | Primary development platform |
| Banana Pi BPI-M8 | Banana Pi | Allwinner A733 | 🔄 Hardware Incoming | Collaboration accepted, awaiting board |
| Radxa Cubie A7A | Radxa | Allwinner A733 | 🔄 Hardware Incoming | Collaboration accepted, awaiting board |
| Orange Pi Zero 3W | Orange Pi / Xunlong | Allwinner A733 | ⏳ Awaiting Hardware | Support expected but not validated |
| Radxa Cubie A7S | Radxa | Allwinner A733 | ⏳ Not Tested | Potential future target |
| Radxa Cubie A7Z | Radxa | Allwinner A733 | ⏳ Not Tested | Potential future target |

Legend:

- ✅ Validated
- 🔄 Hardware incoming / validation in progress
- ⏳ Awaiting hardware or testing
- 🚧 Under development
- ❌ Unsupported

---

# Platform Feature Status

Current validation is based primarily on the Orange Pi 4 Pro.

| Feature | Status | Notes |
|---------|--------|-------|
| Linux 6.18.x Boot | ✅ | Clean boot on upstream 6.18.y base |
| Device Tree Support | ✅ | Mainline bindings and overlay portability |
| Ethernet | ✅ | Dual GMAC Gigabit Ethernet operational |
| SD Card Boot | ✅ | Standard bootloader & rootfs storage |
| eMMC Support | ✅ | High-speed HS400 verified |
| Wi-Fi | ✅ | AIC8800 Wi-Fi 6 (802.11ax) Station & AP modes |
| Bluetooth | ✅ | Bluetooth 5.4 Dual-Mode / BlueZ |
| Vulkan Graphics | ✅ | PowerVR Rogue via Mesa 25 Zink + KMSRO |
| Desktop Environment | ✅ | Hardware-accelerated Wayland (GNOME / Wayfire) |
| Firefox/Web Browsing | ✅ | Smooth modern web rendering with WebRender |
| YouTube Playback | ✅ | 1080p60 fluid playback in desktop session |
| Local LLM Inference | ✅ | On-device llama.cpp Qwen2.5 GGUF models |
| PCIe / NVMe | 🚧 | Reads drive over PCIe lane; direct NVMe boot not supported yet (pending U-Boot patch; separate OS images required) |
| UFS | 🚧 | Controller initialization under development |
| Hardware Video Decode Validation | 🚧 | Cedrus VA-API port in progress |
| NPU Support | ✅ | Kernel driver (`vipcore.ko`) ported to 6.18 & validated live on physical A733 hardware (`/dev/vipcore` active, irq 502, 852 MHz DVFS) |
| Suspend/Resume | ❌ / ⏳ | Wi-Fi module does not support suspend/resume (causes packet loss after resume); PMIC sequencing under evaluation |
| Upstream Kernel Work | 🚧 | CCU, PMIC & DT patch series preparation for LKML |

---

# Benchmarks

Current reference platform:

**Orange Pi 4 Pro**
- Allwinner A733
- 2× Cortex-A76 @ 2.0 GHz
- 6× Cortex-A55 @ 1.8 GHz
- 12 GB LPDDR5

### Sysbench

| Test | Result |
|--------|--------:|
| Single Thread | 841.88 |
| Multi Thread (8) | 3845.67 |

### Memory Bandwidth

| Test | Result |
|--------|--------:|
| MEMCPY | 4875 MiB/s |
| DUMB | 4941 MiB/s |
| MCBLOCK | 6248 MiB/s |

### Local LLM Performance

Using llama.cpp and GGUF models.

#### Qwen2.5-1.5B-Instruct Q4_K_M

| Test | Result |
|--------|--------:|
| Prompt Processing | 31.66 t/s |
| Token Generation | 6.26 t/s |

#### Qwen2.5-3B-Instruct Q4_K_M

| Test | Result |
|--------|--------:|
| Prompt Processing | 14.84 t/s |
| Token Generation | 3.28 t/s |

---

# Acknowledgements

The project would like to thank the vendors who have expressed interest in improving Linux support for the Allwinner A733 platform and have offered development hardware for validation and testing.

Special thanks to:

- Banana Pi
- Radxa

for supporting open-source development efforts around the A733 ecosystem.

---

# Future Roadmap

## Near-Term Goals

- Validate Banana Pi BPI-M8 support
- Validate Radxa Cubie A7A support
- Extend board compatibility matrix
- Improve device tree portability
- Continue Linux 6.18 maintenance

## Mid-Term Goals

- PCIe validation
- UFS validation
- Additional A733 board support
- NPU enablement research

## Long-Term Goals

- Platform-wide A733 Linux support
- Increased upstream compatibility
- Investigation of next-generation Allwinner platforms

---

**Repository:** https://github.com/jonas5/Allwinner-A733-kernel
