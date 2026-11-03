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
| PCIe / NVMe | ✅ | Fully operational (Gen3 speed); Standalone direct boot enabled via U-Boot patch (`0100`) + onboard SPI NOR flash (`/dev/mtd0`) without MicroSD card |
| UFS | 🚧 | Controller initialization under development |
| Hardware Video Decode (VPU) | ✅ | Allwinner Cedar VE1/VE2 hardware decoder operational via `/dev/cedar_dev`; GStreamer OpenMAX (`omxh264dec`) verified up to 4K (118 FPS @ 1080p) |
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

#### Qwen2.5-0.5B-Instruct Q8_0 (llama.cpp, 2× A76 Cores)

| Test | Result |
|--------|--------:|
| Prompt Processing (pp128) | 135.04 t/s |
| Token Generation (tg64) | 17.42 t/s |

---

# NPU Hardware Acceleration Benchmarks (VIP9000 NanoDI Plus)

Measured on the **Orange Pi 4 Pro** using the ported `vipcore` kernel driver on Linux 6.18 with VIPLite 2.0 (`cid=0x1000003b`):

### Hardware VPU Video Decoding Benchmarks (Cedar Engine VE1 / VE2 @ 624 MHz)

Measured on the **Orange Pi 4 Pro** using GStreamer OpenMAX (`gst-omx` / `libgstomx.so`) and direct CedarC API (`vdecoderdemo`):

| Video Codec / Stream | Resolution & Format | Throughput / Frame Rate | CPU Load | Frame Drops |
|---|---|---:|---|---:|
| H.264 / AVC (`omxh264dec`) | 1920×1080 @ 30 FPS | **118.5 FPS** (average) | ~3% (Hardware accelerated) | 0 frames |
| H.264 / AVC (`omxh264dec`) | 3840×2160 (4K UHD) | **30.0+ FPS** (realtime) | ~5% (Hardware scaled) | 0 frames |
| Direct CedarC (`vdecoderdemo`) | 1280×720 (YUV420P) | **30 frames in <1s** | 0% (Hardware DMA) | 0 frames |

### NPU Vision & Convolutional Networks (Pure NPU Hardware Inference)

| Model | Input Shape | Precision | Pure NPU Latency | Frame Rate (FPS) |
|---|---|---|---|---:|
| LeNet | 28×28×1 | UINT8 | **114 µs** | ~8,770 FPS |
| ResNet-50 | 224×224×3 | UINT8 | **10.07 ms** | ~99.2 FPS |
| MobileCLIP-S0 (Vision Encoder) | 256×256×3 | INT16 | **22.6 ms** | 44.2 FPS |

### NPU-Accelerated LLM & Hybrid VLM Inference (a733_npu_driver suite)

| Model | Pipeline | Precision / Context | Throughput | Peak RSS / NBG Size | Status |
|---|---|---|---|---|---|
| SmolLM2-135M | NPU Native (`npu_lm_runner`) | INT16, W=32 | **20.7 tok/s** | 272 MiB RSS (281 MB NBG) | ✅ Verified (0% CPU) |
| SmolLM2-360M | NPU Native (`npu_lm_runner`) | INT16, W=32 | **8.4 tok/s** | 646 MiB RSS (673 MB NBG) | ✅ Verified |
| Qwen2.5-0.5B | CPU (2× Cortex-A76) | Q8_0, 2k ctx | **17.4 tok/s** (135 pp tok/s) | 1,109 MiB (6 A55 cores free) | ✅ Verified |
| SmolVLM-256M | Hybrid (NPU Vision + CPU LLM) | Q8_0 | **52.6 tok/s** (22.6 ms vision) | 634 MiB RSS | ✅ Verified |

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
