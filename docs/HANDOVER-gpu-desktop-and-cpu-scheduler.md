# Handover: GPU Driver (X11 / GDM / GNOME) & CPU Scheduler Architecture

**Date:** 2026-10-19  
**Target:** `orangepi4pro` (Orange Pi 4 Pro)  
**SoC:** Allwinner A733 (`sun60iw2`) — 6× Cortex-A55 (LITTLE) + 2× Cortex-A76 (BIG)  
**GPU:** Imagination PowerVR Rogue BXM-4-64 MC1 (BVNC 36.56.104.183)  
**Kernel:** Mainline Linux `6.18.56-dirty #25`  
**Compositor:** GNOME Shell / Mutter 43.8 (Native Wayland) with on-demand Xwayland  
**Mesa Stack:** Mesa 25.0.7 with custom Zink / KMSRO patches  

---

## 1. Executive Summary

This document details the engineering work performed to resolve two critical issues on the Orange Pi 4 Pro:
1. **Desktop & Browser Hardware Acceleration**: Restored hardware-accelerated OpenGL/GLES for GNOME Shell, GDM, Xwayland, and Chromium via Mesa Zink over PowerVR Vulkan, dropping desktop load from >500% software compositing down to ~15% (< 0.5 system load).
2. **CPU Scheduler & Frequency Scaling**: Fixed a broken device tree OPP table that had permanently killed `cpufreq-dt` on mainline 6.18, trapping the CPU at half speed (1014 MHz) and packing all heavy threads onto a single LITTLE core. Restored dynamic scaling up to 1.80 GHz (A55) and 2.002 GHz (A76) with full kernel Energy Model (EAS) task placement.

---

## 2. Hardware Topology

The Allwinner A733 features an asymmetric compute and split graphics architecture:

### CPU Topology (DynamIQ big.LITTLE)
| Cluster | Cores | Architecture | Sysfs Capacity | Frequency Range | Regulator Supply |
|---|---|---|---|---|---|
| **Cluster 0** | `cpu0`–`cpu5` (6 cores) | ARM Cortex-A55 (LITTLE) | **430** | 408 MHz – 1800 MHz | PMIC `axp8191-dcdc5` |
| **Cluster 1** | `cpu6`–`cpu7` (2 cores) | ARM Cortex-A76 (BIG) | **1024** | 408 MHz – 2002 MHz | PMIC `axp8191-dcdc3` |

### DRM / Display Nodes
| Node | Kernel Driver | Bus Path | Role | Capabilities |
|---|---|---|---|---|
| `/dev/dri/card0` | `sunxi-drm` | `platform-soc@3000000:sunxi-drm` | HDMI Display Engine (DE) | Scanout only, modesetting, NO 3D rendering |
| `/dev/dri/card1` | `powervr` | `platform-1800000.gpu` | PowerVR Rogue BXM-4-64 | Primary node created by driver; NO scanout / NO CRTCs |
| `/dev/dri/renderD128` | `powervr` | `platform-1800000.gpu` | PowerVR Rogue BXM-4-64 | Render node for Vulkan / EGL / Zink 3D operations |

---

## 3. GPU Driver Stack (Mesa 25.0.7, Zink & KMSRO)

### The Core Challenge
Because the A733 has no display scanout on the 3D GPU and no 3D rendering hardware on the display controller, Gallium `kmsro` (Kernel Modesetting Render-Only) must bridge scanout on `card0` with 3D rendering on `renderD128`.

### Changes Applied to Mesa 25.0.7
Source tree: `/root/mesa-build/mesa-25.0.7/`  
Unified patch: `/root/mesa-build/mesa-25.0.7-pvr-zink-kmsro.patch`

1. **Meson Build Configuration**:
   - `src/gallium/auxiliary/pipe-loader/meson.build`: Added `if with_gallium_zink: renderonly_drivers_c_args += '-DGALLIUM_ZINK'`. Previously, `-DGALLIUM_ZINK` was never passed to `kmsrowinsys`, so `#if defined(GALLIUM_ZINK)` evaluated to false and Zink was never instantiated for KMSRO.
   - `src/gallium/winsys/kmsro/drm/meson.build`: Added `inc_gallium_drivers` to include directories so `zink/zink_public.h` can be resolved.
   - `src/gallium/drivers/zink/zink_public.h`: Added `#include <stdint.h>`.

2. **KMS / Render Split Handle Translation (`zink_drm_create_screen_renderonly`)**:
   - **Problem**: When Mutter created a GBM surface on `card0` (`sunxi-drm`), Zink allocated the buffer on `renderD128` (PowerVR). When Mutter requested the KMS GEM handle via `gbm_bo_get_handle` (`WINSYS_HANDLE_TYPE_KMS`), `zink_bo_get_kms_handle` called `drmPrimeFDToHandle(screen->drm_fd, fd, &handle)`. Because `screen->drm_fd` was initialized to `gpu_fd` (`renderD128`), the buffer was imported back into PowerVR rather than `card0` (`sunxi-drm`). Mutter's subsequent `drmModeAddFB(card0, ..., handle)` returned `ENOENT` / `Invalid argument`.
   - **Fix**: Added `zink_drm_create_screen_renderonly(gpu_fd, kms_fd, config)` in `src/gallium/drivers/zink/zink_screen.c` and wired it into `src/gallium/winsys/kmsro/drm/kmsro_drm_winsys.c`. This sets `screen->drm_fd` to `kms_fd` (`card0`), ensuring `drmPrimeFDToHandle` imports the buffer into `sunxi-drm`. `drmModeAddFB` on `card0` now succeeds with `ret=0, fb_id=166`.

3. **Driver ID & Non-Solid Fill Workarounds**:
   - In `src/gallium/drivers/zink/zink_screen.c`: Extended driver ID checks for `VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA` (matching the mainline `powervr` Vulkan driver) so wireframe emulation (`no_hw_gl_wireframe = true`), line smoothing, and point size workarounds are active.

4. **Installed Driver Symlinks**:
   In `/usr/lib/aarch64-linux-gnu/dri/` and `/gbm/`:
   - `pvr_dri.so`, `powervr_dri.so`, `sunxi-drm_dri.so`, `sun4i-drm_dri.so` -> `libdril_dri.so`
   - `pvr_gbm.so`, `powervr_gbm.so`, `sunxi-drm_gbm.so` -> `dri_gbm.so`

---

## 4. Desktop Integration (GDM, GNOME Shell / Mutter & Xwayland)

### Mutter KMS Enumeration & Udev Rule Fix
- **Problem**: Mutter enumerates all DRM devices matching `card*`. Because `card1` had the `seat` tag, Mutter attempted modesetting on `card1` first. Because `card1` has no CRTCs, `meta_kms_impl_device_init` failed with `"No suitable mode setting backend found"`. Due to an assertion failure in Mutter's error cleanup, `card1` was never released back to `systemd-logind`. When Mutter then tried to open `card0`, `systemd-logind` rejected `TakeDevice(card0)` with `EBUSY: Device or resource busy` (logind permits only one active DRM master per seat).
- **Fix**: Implemented `/etc/udev/rules.d/90-powervr-drm.rules`:
  ```udev
  # PowerVR Rogue BXM-4-64 is render-only; tag card1 with mutter-device-ignore so Mutter only uses sunxi-drm
  SUBSYSTEM=="drm", KERNEL=="card[1-9]*", KERNELS=="1800000.gpu", TAG+="mutter-device-ignore", TAG-="master-of-seat"

  # sunxi-drm does not support KMS modifiers, so disable them in Mutter
  SUBSYSTEM=="drm", KERNEL=="card0", TAG+="mutter-device-disable-kms-modifiers"
  ```
  `libmutter-11.so.0` checks for `mutter-device-ignore` and skips `card1` entirely, allowing `card0` to be acquired as the sole display master.

### Module Autoloading
- Created `/etc/systemd/system/powervr-load.service` to load `powervr.ko` on boot before GDM starts:
  ```ini
  [Unit]
  Description=Load PowerVR DRM Kernel Module before Display Manager
  DefaultDependencies=no
  After=systemd-modules-load.service
  Before=gdm.service display-manager.service

  [Service]
  Type=oneshot
  ExecStart=/sbin/modprobe powervr
  RemainAfterExit=yes

  [Install]
  WantedBy=sysinit.target
  ```

### Xwayland Support
- GNOME Shell acts as a native Wayland compositor on `wayland-0`.
- Xwayland starts on demand for legacy X11 applications, fully hardware accelerated via Zink:
  ```bash
  $ es2_info
  GL_VENDOR: Mesa
  GL_VERSION: OpenGL ES 2.0 Mesa 25.0.7
  GL_RENDERER: zink Vulkan 1.2(PowerVR B-Series BXM-4-64 MC1 (IMAGINATION_OPEN_SOURCE_MESA))
  ```

---

## 5. CPU Scheduler & Frequency Scaling Fix (`cpufreq-dt`)

### Root Cause Analysis
The device tree shipped on the board originated from Allwinner's vendor BSP. In the vendor kernel, a custom driver parsed proprietary properties: `opp-microvolt-vf0000`, `opp-microvolt-vf0100`, etc.

When running **mainline Linux 6.18**, the standard `drivers/opp/of.c` core expects standard `opp-microvolt` properties. Because `cpu-supply` was defined on `cpu@0` and `cpu@600` but `opp-microvolt` was absent, `opp_parse_microvolt` failed with `-22` (`EINVAL`), causing the kernel to throw:
```
[27402.390050] cpu cpu0: opp_parse_microvolt: opp-microvolt missing although OPP managing regulators
[27402.390063] cpu cpu0: _of_add_opp_table_v2: Failed to add OPP, -22
[27402.390070] cpu cpu0: OPP table can't be empty
```
Because of this failure:
1. `cpufreq-dt` refused to probe; `/sys/devices/system/cpu/cpufreq/` remained empty.
2. Both CPU clusters were permanently frozen at their safe power-on bootloader clock: **1014 MHz**. The Cortex-A76 big cores were locked at **half speed**.
3. With no cpufreq, the kernel's **Energy Model (EM)** could not register.
4. Energy-Aware Scheduling (EAS) treated all cores as static, packing high-load threads onto **`cpu1` (a LITTLE core)**. The Chromium Blink renderer thread was pinned at 101% CPU on `cpu1` with >20,000 nonvoluntary context switches, dropping 205 of 904 frames on YouTube.

### The Fix
1. Decompiled `/boot/dtb/allwinner/sun60i-a733-orangepi-4-pro.dtb` (backup saved to `sun60i-a733-orangepi-4-pro.dtb.bak`).
2. Replaced `cluster0-opp-table` and `cluster1-opp-table` with standard `operating-points-v2` tables carrying explicit `opp-microvolt` values:
   - **Cluster 0 (A55 Little)**: 408 MHz (0.80V) scaling up to **1800 MHz (1.05V)**.
   - **Cluster 1 (A76 Big)**: 408 MHz (0.80V) scaling up to **2002 MHz (1.05V)**.
3. Added clock ceilings to the DTB:
   - GPU `clk_rate = <1104000000>` and PMIC `dcdc4` `regulator-min-microvolt = <990000>`.
   - DSU `assigned-clock-rates = <1032000000>`.
4. Recompiled with `dtc` and installed to `/boot/dtb-6.18.56-sun60iw2/allwinner/sun60i-a733-orangepi-4-pro.dtb`.

### Verification
```bash
$ cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq
1800000   # Little cluster at 1.80 GHz
$ cat /sys/devices/system/cpu/cpufreq/policy6/scaling_cur_freq
2002000   # Big cluster at 2.002 GHz
$ ls -la /sys/kernel/debug/energy_model/
cpu0  cpu6  # Energy Model is active for both clusters
```

---

## 6. Chromium Hardware Acceleration & Video Optimization

### Chromium Configuration
File: `/etc/chromium.d/powervr-vulkan`
```bash
# PowerVR Rogue BXM-4-64 Vulkan & EGL Hardware Acceleration
export PVR_I_WANT_A_BROKEN_VULKAN_DRIVER=1
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/powervr_mesa_icd.json
export MESA_GLES_VERSION_OVERRIDE=3.0

export CHROMIUM_FLAGS="$CHROMIUM_FLAGS \
  --ignore-gpu-blocklist \
  --enable-gpu-rasterization \
  --ozone-platform=wayland \
  --use-gl=angle \
  --use-angle=gles \
  --disable-gpu-memory-buffer-video-frames \
  --video-threads=4 \
  --disable-frame-rate-limit \
  --load-extension=/usr/share/chromium/extensions/enhanced-h264ify"
```
- `MESA_GLES_VERSION_OVERRIDE=3.0`: Resolves Chromium's `eglCreateContext ES 3.0 failed with error EGL_BAD_ATTRIBUTE`.
- `--use-gl=angle --use-angle=gles`: Directs Chromium to use ANGLE over Zink OpenGLES, eliminating `--use-gl=disabled`.
- `--disable-gpu-memory-buffer-video-frames`: Prevents GpuMemoryBuffer DMA-BUF texture deadlocks on software-decoded frames.
- `--video-threads=4`: Splits macroblock slice decoding across 4 parallel threads on the A733 cores.
- `--disable-frame-rate-limit`: Prevents Wayland VSync jitter from prematurely dropping frames.

### Mesa Zink Buffer Cache Fix (`zink_bo.c`)
- **Root Cause Discovered via strace**: Chromium GPU process was trapped in 5,817 `ioctl`s/sec (spending 90.52% CPU time in kernel page-table unmapping and `drm_gem_shmem_free`).
- **The Bug**: In `src/gallium/drivers/zink/zink_bo.c`, `init_pb_cache = !pNext;` was evaluated *after* `pNext` was pointed to local `VkMemoryAllocateFlagsInfo` for device address support. As a result, `init_pb_cache` was always `false`, disabling buffer caching and freeing every single BO immediately.
- **The Fix**: Moved `init_pb_cache = !pNext;` to the very top of `bo_create_internal`. Buffer recycling was restored, completely eliminating the 5,800 ioctls/sec thrashing.

### CPU Governor Tuning (`/etc/default/cpufrequtils`)
- **Problem**: Legacy `ondemand` governor with a 416 MHz floor was downclocking the CPU during sub-frame idle gaps, causing late wake-ups and frame drops.
- **The Fix**: Switched to `schedutil` with a minimum frequency floor of **1.20 GHz** on all clusters (`MIN_SPEED=1200000`, `GOVERNOR=schedutil`). Eliminates ramp-up latency when video decode threads wake.

---

## 7. Firefox ESR Integration & Comparison

Debian 12 Bookworm ships **Firefox ESR** (v153.5.0esr), which provides an even smoother YouTube experience than Chromium on ARM64 by bypassing ANGLE and running direct WebRender on Wayland.

### Firefox ESR Configuration
File: `/etc/firefox-esr/firefox-esr.js`
```javascript
// Native Wayland & WebRender Acceleration
pref("gfx.webrender.all", true);
pref("widget.wayland.opaque-region.enabled", true);

// Natively Force YouTube to H.264 (avc1) Without Extensions
pref("media.av1.enabled", false);
pref("media.mediasource.vp9.enabled", false);
pref("media.webm.enabled", false);

// Multi-Threaded System FFmpeg with ARM NEON Assembly
pref("media.ffmpeg.enabled", true);
pref("media.ffvpx.enabled", false);
pref("media.hardware-video-decoding.enabled", false);
pref("media.navigator.mediadatadecoder_vpx_enabled", false);
```
File: `/etc/environment`
```bash
PVR_I_WANT_A_BROKEN_VULKAN_DRIVER=1
MOZ_ENABLE_WAYLAND=1
```
- **RDD Process (`rdd`)**: Firefox executes video decoding in an isolated Remote Data Decoder process with direct FFmpeg NEON multi-threading, keeping UI compositing completely fluid.

---

## 8. Handover Checklist & Verification Commands

```sh
# 1. Verify CPU Frequency Scaling & Energy Model:
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq  # 408000..1800000
cat /sys/devices/system/cpu/cpufreq/policy6/scaling_cur_freq  # 408000..2002000
ls -la /sys/kernel/debug/energy_model/                        # cpu0, cpu6

# 2. Verify GPU Rendering & Mesa Zink:
es2_info | grep -E "GL_RENDERER|GL_VERSION"
# Expected: zink Vulkan 1.2(PowerVR B-Series BXM-4-64 MC1), OpenGL ES 2.0 / 3.0

# 3. Verify Chromium Hardware Acceleration:
# Launch Chromium, check chrome://gpu
# Expected: Canvas, Compositing, Rasterization, WebGL, WebGPU = Hardware accelerated
# Verify --use-gl=disabled is absent:
ps aux | grep chromium | grep -e --use-gl=disabled           # Must return NOTHING

# 4. Verify Desktop Session:
systemctl status gdm                                         # active (running)
ps aux | grep gnome-shell                                    # CPU usage ~15%, load < 0.5
```
