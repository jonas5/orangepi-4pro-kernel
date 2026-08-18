#!/bin/bash
# Orange Pi 4 Pro / Allwinner A733 Kernel Patch Installer
# Installs all driver files, DTS/DTSI, defconfig, and boot assets
# into a Linux kernel source tree.
#
# Usage: ./opi4pro-7.1.5-install.sh [KERNEL_SRC_DIR]
#   KERNEL_SRC_DIR defaults to current directory if not specified.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_SRC="${1:-.}"

if [ ! -f "$KERNEL_SRC/Makefile" ]; then
    echo "ERROR: $KERNEL_SRC does not appear to be a Linux kernel source tree." >&2
    echo "Usage: $0 [KERNEL_SRC_DIR]" >&2
    exit 1
fi

echo "=== Orange Pi 4 Pro Kernel Patch Installer ==="
echo "Kernel source: $KERNEL_SRC"
echo "Patch repo:    $SCRIPT_DIR"
echo ""

# ------------------------------------------------------------------
# 1. Device Tree Source / Include
# ------------------------------------------------------------------
echo "[1/9] Installing Device Tree files..."
mkdir -p "$KERNEL_SRC/arch/arm64/boot/dts/allwinner"
cp "$SCRIPT_DIR/dts/sun60i-a733.dtsi" \
   "$KERNEL_SRC/arch/arm64/boot/dts/allwinner/sun60i-a733.dtsi"
cp "$SCRIPT_DIR/dts/sun60i-a733-orangepi-4-pro.dts" \
   "$KERNEL_SRC/arch/arm64/boot/dts/allwinner/sun60i-a733-orangepi-4-pro.dts"

DTS_MAKEFILE="$KERNEL_SRC/arch/arm64/boot/dts/allwinner/Makefile"
DTB_LINE="dtb-\$(CONFIG_ARCH_SUNXI) += sun60i-a733-orangepi-4-pro.dtb"
if [ -f "$DTS_MAKEFILE" ]; then
    if ! grep -qF "sun60i-a733-orangepi-4-pro.dtb" "$DTS_MAKEFILE"; then
        echo "" >> "$DTS_MAKEFILE"
        echo "# Orange Pi 4 Pro (Allwinner A733)" >> "$DTS_MAKEFILE"
        echo "$DTB_LINE" >> "$DTS_MAKEFILE"
        echo "  Added DTB entry to $DTS_MAKEFILE"
    fi
fi

# ------------------------------------------------------------------
# 2. DT binding headers
# ------------------------------------------------------------------
echo "[2/9] Installing DT binding headers..."
mkdir -p "$KERNEL_SRC/include/dt-bindings/clock"
mkdir -p "$KERNEL_SRC/include/dt-bindings/gpio"
mkdir -p "$KERNEL_SRC/include/dt-bindings/thermal"
cp "$SCRIPT_DIR/dts/include/dt-bindings/clock/sun60i-a733-ccu.h" \
   "$KERNEL_SRC/include/dt-bindings/clock/"
cp "$SCRIPT_DIR/dts/include/dt-bindings/clock/sun60i-a733-r-ccu.h" \
   "$KERNEL_SRC/include/dt-bindings/clock/"
cp "$SCRIPT_DIR/dts/include/dt-bindings/gpio/sun60i-a733-gpio.h" \
   "$KERNEL_SRC/include/dt-bindings/gpio/"
cp "$SCRIPT_DIR/dts/include/dt-bindings/thermal/sun60i-a733-thermal.h" \
   "$KERNEL_SRC/include/dt-bindings/thermal/"

# ------------------------------------------------------------------
# 3. CCU (Clock Control Unit)
# ------------------------------------------------------------------
echo "[3/9] Installing CCU driver..."
mkdir -p "$KERNEL_SRC/drivers/clk/sunxi-ng"
cp "$SCRIPT_DIR/dts/src/clk/clk-sun60i-a733.c" \
   "$KERNEL_SRC/drivers/clk/sunxi-ng/"
CCU_MKFILE="$KERNEL_SRC/drivers/clk/sunxi-ng/Makefile"
if [ -f "$CCU_MKFILE" ]; then
    if ! grep -qF "clk-sun60i-a733" "$CCU_MKFILE"; then
        echo "" >> "$CCU_MKFILE"
        echo "# Allwinner sun60i-a733 CCU" >> "$CCU_MKFILE"
        echo "obj-\$(CONFIG_AW_SUN60IW2_CCU) += clk-sun60i-a733.o" >> "$CCU_MKFILE"
    fi
fi

# ------------------------------------------------------------------
# 4. Pin Control
# ------------------------------------------------------------------
echo "[4/9] Installing pinctrl driver..."
mkdir -p "$KERNEL_SRC/drivers/pinctrl/sunxi"
cp "$SCRIPT_DIR/dts/src/pinctrl/pinctrl-sun20i-a733.c" \
   "$KERNEL_SRC/drivers/pinctrl/sunxi/"
PINCTRL_MKFILE="$KERNEL_SRC/drivers/pinctrl/sunxi/Makefile"
if [ -f "$PINCTRL_MKFILE" ]; then
    if ! grep -qF "pinctrl-sun20i-a733" "$PINCTRL_MKFILE"; then
        echo "" >> "$PINCTRL_MKFILE"
        echo "# Allwinner sun60i-a733 pinctrl" >> "$PINCTRL_MKFILE"
        echo "obj-\$(CONFIG_AW_PINCTRL_SUN60IW2) += pinctrl-sun20i-a733.o" >> "$PINCTRL_MKFILE"
    fi
fi

# ------------------------------------------------------------------
# 5. MMC
# ------------------------------------------------------------------
echo "[5/9] Installing MMC host driver..."
mkdir -p "$KERNEL_SRC/drivers/mmc/host"
cp "$SCRIPT_DIR/dts/src/mmc/sun20i-d1-mmc.c" \
   "$KERNEL_SRC/drivers/mmc/host/"
cp "$SCRIPT_DIR/dts/src/mmc/sun20i-d1-mmc.h" \
   "$KERNEL_SRC/drivers/mmc/host/"

# ------------------------------------------------------------------
# 6. PHY drivers
# ------------------------------------------------------------------
echo "[6/9] Installing PHY drivers..."
mkdir -p "$KERNEL_SRC/drivers/phy/allwinner"
cp "$SCRIPT_DIR/dts/src/phy/phy-sun60i-serdes.c" \
   "$KERNEL_SRC/drivers/phy/allwinner/"
cp "$SCRIPT_DIR/dts/src/phy/phy-sun60i-usb3.c" \
   "$KERNEL_SRC/drivers/phy/allwinner/"
cp "$SCRIPT_DIR/dts/src/phy/phy-sun60i-pcie.c" \
   "$KERNEL_SRC/drivers/phy/allwinner/"

# ------------------------------------------------------------------
# 7. DRM / Display
# ------------------------------------------------------------------
echo "[7/9] Installing display drivers..."
mkdir -p "$KERNEL_SRC/drivers/gpu/drm/sun4i"
cp "$SCRIPT_DIR/dts/src/drm/sun60i-de.c" \
   "$KERNEL_SRC/drivers/gpu/drm/sun4i/"
cp "$SCRIPT_DIR/dts/src/drm/sun60i-tcon.c" \
   "$KERNEL_SRC/drivers/gpu/drm/sun4i/"
cp "$SCRIPT_DIR/dts/src/drm/sun60i-hdmi.c" \
   "$KERNEL_SRC/drivers/gpu/drm/sun4i/"
cp "$SCRIPT_DIR/dts/src/drm/sun60i-drm.c" \
   "$KERNEL_SRC/drivers/gpu/drm/sun4i/"

# ------------------------------------------------------------------
# 8. Remaining drivers
# ------------------------------------------------------------------
echo "[8/9] Installing remaining drivers..."

# Ethernet
mkdir -p "$KERNEL_SRC/drivers/net/ethernet/stmicro/stmmac"
cp "$SCRIPT_DIR/dts/src/ethernet/sun60i-gmac.c" \
   "$KERNEL_SRC/drivers/net/ethernet/stmicro/stmmac/"

# PMIC
mkdir -p "$KERNEL_SRC/drivers/mfd"
cp "$SCRIPT_DIR/dts/src/mfd/mfd-axp8191.c" \
   "$KERNEL_SRC/drivers/mfd/"
mkdir -p "$KERNEL_SRC/drivers/regulator"
cp "$SCRIPT_DIR/dts/src/regulator/regulator-axp8191.c" \
   "$KERNEL_SRC/drivers/regulator/"

# Thermal
mkdir -p "$KERNEL_SRC/drivers/thermal"
cp "$SCRIPT_DIR/dts/src/thermal/sun60i-thermal.c" \
   "$KERNEL_SRC/drivers/thermal/"

# Crypto
mkdir -p "$KERNEL_SRC/drivers/crypto/allwinner"
cp "$SCRIPT_DIR/dts/src/crypto/sun60i-ce.c" \
   "$KERNEL_SRC/drivers/crypto/allwinner/"

# Sound
mkdir -p "$KERNEL_SRC/sound/soc/sunxi"
cp "$SCRIPT_DIR/dts/src/sound/sun60i-audio.c" \
   "$KERNEL_SRC/sound/soc/sunxi/"

# Defconfig
mkdir -p "$KERNEL_SRC/arch/arm64/configs"
cp "$SCRIPT_DIR/dts/orangepi_4pro_defconfig" \
   "$KERNEL_SRC/arch/arm64/configs/"

# ------------------------------------------------------------------
# 9. Makefile / Kconfig entries
# ------------------------------------------------------------------
echo "[9/9] Adding Makefile/Kconfig entries..."
for pair in \
    "drivers/clk/sunxi-ng/Kconfig|source \"drivers/clk/sunxi-ng/Kconfig\"" \
    "drivers/clk/sunxi-ng/Makefile|obj-\$(CONFIG_AW_SUN60IW2_CCU) += clk-sun60i-a733.o"; do
    file="${pair%%|*}"; line="${pair#*|}"
    target="$KERNEL_SRC/$file"
    if [ -f "$target" ] && ! grep -qF "sun60i-a733" "$target" 2>/dev/null; then
        echo "" >> "$target"
        echo "# Orange Pi 4 Pro (Allwinner A733)" >> "$target"
        echo "$line" >> "$target"
    fi
done

echo ""
echo "NOTE: For a complete installation with all Makefile/Kconfig entries,"
echo "the unified patch (opi4pro-7.1.5.patch) is recommended instead."

echo ""
echo "=== Installation complete ==="
echo ""
echo "Build with:"
echo "  cd $KERNEL_SRC"
echo "  make orangepi_4pro_defconfig"
echo "  make -j\$(nproc)"
echo ""
echo "Output:"
echo "  arch/arm64/boot/dts/allwinner/sun60i-a733-orangepi-4-pro.dtb"
echo "  arch/arm64/boot/Image"
