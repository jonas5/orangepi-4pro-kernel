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

| Feature | Status |
|----------|----------|
| Linux 6.18.x Boot | ✅ |
| Device Tree Support | ✅ |
| Ethernet | ✅ |
| SD Card Boot | ✅ |
| eMMC Support | ✅ |
| Wi-Fi | ✅ |
| Bluetooth | ✅ |
| Vulkan Graphics | ✅ |
| Desktop Environment | ✅ |
| Firefox/Web Browsing | ✅ |
| YouTube Playback | ✅ |
| Local LLM Inference | ✅ |
| PCIe | 🚧 |
| UFS | 🚧 |
| Hardware Video Decode Validation | 🚧 |
| NPU Support | 🚧 |
| Suspend/Resume | ⏳ |
| Upstream Kernel Work | 🚧 |

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
