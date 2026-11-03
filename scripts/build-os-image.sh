#!/bin/bash
# ==============================================================================
# build-os-image.sh — Unified Local Image Builder for Orange Pi 4 Pro
# Supports: Full stack of Ubuntu (noble, jammy, focal), Debian (bookworm, trixie,
# bullseye, sid), and Arch Linux ARM across Minimal, Server, and Desktop flavours.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Dynamically resolve orangepi-4pro-kernel repository root
if [ -f "${SCRIPT_DIR}/../patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
elif [ -f "${SCRIPT_DIR}/../Allwinner-A733-kernel/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="$(cd "${SCRIPT_DIR}/../Allwinner-A733-kernel" && pwd)"
elif [ -f "/srv/kernel/Allwinner-A733-kernel/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="/srv/kernel/Allwinner-A733-kernel"
elif [ -f "${SCRIPT_DIR}/../orangepi-4pro-kernel/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="$(cd "${SCRIPT_DIR}/../orangepi-4pro-kernel" && pwd)"
elif [ -f "/srv/kernel/orangepi-4pro-kernel/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="/srv/kernel/orangepi-4pro-kernel"
else
    echo "[-] Error: Could not locate orangepi-4pro-kernel repository with patches."
    exit 1
fi

# Dynamically resolve orangepi-build harness directory
if [ -d "${REPO_ROOT}/orangepi-build" ]; then
    HARNESS_DIR="${REPO_ROOT}/orangepi-build"
elif [ -d "${REPO_ROOT}/../orangepi-build" ]; then
    HARNESS_DIR="$(cd "${REPO_ROOT}/../orangepi-build" && pwd)"
elif [ -d "/srv/kernel/orangepi-build" ]; then
    HARNESS_DIR="/srv/kernel/orangepi-build"
elif [ -d "${SCRIPT_DIR}/../orangepi-build" ]; then
    HARNESS_DIR="$(cd "${SCRIPT_DIR}/../orangepi-build" && pwd)"
else
    HARNESS_DIR="${REPO_ROOT}/orangepi-build"
fi

RELEASE="${1:-noble}"
FLAVOUR="${2:-minimal}"

# Bypass host OS checks on non-Ubuntu hosts (e.g. Debian 13 Trixie)
export NO_HOST_RELEASE_CHECK=yes
export EXPERT=yes

echo "========================================================================"
echo "  Orange Pi 4 Pro (A733 / Linux 6.18) OS Image Builder"
echo "  Distribution: ${RELEASE} | Flavour: ${FLAVOUR}"
echo "  Kernel Repo:  ${REPO_ROOT}"
echo "  Build Harness: ${HARNESS_DIR}"
echo "========================================================================"

if [ "$(id -u)" -ne 0 ]; then
    echo "[-] Error: This script must be run as root (or with sudo)."
    exit 1
fi

# Handle Arch Linux ARM
if [ "${RELEASE}" = "arch" ]; then
    echo "[*] Handing off to Arch Linux ARM image builder..."
    if [ -f "${SCRIPT_DIR}/build-arch-image.sh" ]; then
        exec bash "${SCRIPT_DIR}/build-arch-image.sh"
    elif [ -f "${REPO_ROOT}/scripts/build-arch-image.sh" ]; then
        exec bash "${REPO_ROOT}/scripts/build-arch-image.sh"
    else
        echo "[-] Error: Could not find build-arch-image.sh."
        exit 1
    fi
fi

BUILD_MINIMAL="no"
BUILD_DESKTOP="no"
DESKTOP_ENV_ARGS=()

case "${FLAVOUR}" in
    minimal)
        BUILD_MINIMAL="yes"
        BUILD_DESKTOP="no"
        ;;
    server)
        BUILD_MINIMAL="no"
        BUILD_DESKTOP="no"
        ;;
    desktop-xfce)
        BUILD_MINIMAL="no"
        BUILD_DESKTOP="yes"
        DESKTOP_ENV_ARGS=("DESKTOP_ENVIRONMENT=xfce" "DESKTOP_ENVIRONMENT_CONFIG_NAME=config_base" "DESKTOP_APPGROUPS_SELECTED=")
        ;;
    desktop-gnome)
        BUILD_MINIMAL="no"
        BUILD_DESKTOP="yes"
        DESKTOP_ENV_ARGS=("DESKTOP_ENVIRONMENT=gnome" "DESKTOP_ENVIRONMENT_CONFIG_NAME=config_base" "DESKTOP_APPGROUPS_SELECTED=")
        ;;
    desktop-mate)
        BUILD_MINIMAL="no"
        BUILD_DESKTOP="yes"
        DESKTOP_ENV_ARGS=("DESKTOP_ENVIRONMENT=mate" "DESKTOP_ENVIRONMENT_CONFIG_NAME=config_base" "DESKTOP_APPGROUPS_SELECTED=")
        ;;
    *)
        echo "[-] Unknown flavour: ${FLAVOUR}. Valid options: minimal, server, desktop-xfce, desktop-gnome, desktop-mate"
        exit 1
        ;;
esac

if [ ! -d "${HARNESS_DIR}" ]; then
    echo "[*] Cloning orangepi-build harness to ${HARNESS_DIR}..."
    git clone --depth 1 https://github.com/orangepi-xunlong/orangepi-build.git "${HARNESS_DIR}"
    git -C "${HARNESS_DIR}" apply "${REPO_ROOT}/patch/orangepi-build.patch"
fi

# Sync patches, defconfig, and overlays
echo "[*] Syncing kernel patches and system overlays..."
mkdir -p "${HARNESS_DIR}/external/patch/kernel/sun60iw2-next"
cp -v "${REPO_ROOT}/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" "${HARNESS_DIR}/external/patch/kernel/sun60iw2-next/"
cp -v "${REPO_ROOT}/patch/0300-packaging-armbian-builddeb.patch" "${HARNESS_DIR}/external/patch/kernel/sun60iw2-next/"

mkdir -p "${HARNESS_DIR}/external/config/kernel"
cp -v "${REPO_ROOT}/config/orangepi_4pro_defconfig" "${HARNESS_DIR}/external/config/kernel/linux-sun60iw2-next-a733.config"

mkdir -p "${HARNESS_DIR}/external/packages/bsp/common"
cp -rv "${REPO_ROOT}/overlay/"* "${HARNESS_DIR}/external/packages/bsp/common/"

echo "[*] Starting build.sh for ${RELEASE} (${FLAVOUR})..."
cd "${HARNESS_DIR}"
./build.sh \
    BOARD=orangepi4pro \
    BRANCH=next \
    BUILD_OPT=image \
    RELEASE="${RELEASE}" \
    BUILD_MINIMAL="${BUILD_MINIMAL}" \
    BUILD_DESKTOP="${BUILD_DESKTOP}" \
    NO_HOST_RELEASE_CHECK=yes \
    EXPERT=yes \
    ${DESKTOP_ENV_ARGS[@]+"${DESKTOP_ENV_ARGS[@]}"}

echo "========================================================================"
echo " [✓] Build completed! Check ${HARNESS_DIR}/output/images/ for artifacts."
echo "========================================================================"
