# Contributing to Orange Pi 4 Pro Kernel

## Quick Start

1. Clone the repo
2. Apply the patch to a fresh kernel source
3. Build and test on your Orange Pi 4 Pro
4. Submit a pull request

## Development

```bash
# Clone kernel and apply patch
git clone https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git --depth=1 --branch v7.1.5
cd linux
git apply /path/to/opi4pro-7.1.5.patch

# Build
make ARCH=arm64 defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# Test on hardware
scp arch/arm64/boot/Image arch/arm64/boot/dts/allwinner/sun60i-a733-orangepi-4-pro.dtb root@orangepi4pro:/boot/
```

## Reporting Issues

When reporting issues, please include:
- Board model (Orange Pi 4 Pro)
- Distro and version
- Kernel version (`uname -a`)
- Steps to reproduce
- Relevant log output (`dmesg | tail -50`)

## Code Style

- Follow the Linux kernel coding style
- Use `checkpatch.pl` before submitting
- Keep patches focused and minimal

## Testing

- Verify the patch applies cleanly against kernel 7.1.5
- Test on actual hardware if possible
- Check that the DTB compiles without warnings

## What We Need

Priority areas for contribution:
- Power management optimization
- Thermal testing and tuning
- Camera/ISP driver development
- WiFi/BT stability improvements
- Documentation improvements
