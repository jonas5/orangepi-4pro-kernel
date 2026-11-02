#!/bin/bash
# ==============================================================================
# build-arch-image.sh — Build Arch Linux ARM SD Card Image for Orange Pi 4 Pro
# Hardware: Allwinner A733 / sun60iw2 (Linux 6.18)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Dynamically resolve orangepi-4pro-kernel repository root
if [ -f "${SCRIPT_DIR}/../patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
elif [ -f "${SCRIPT_DIR}/../orangepi-4pro-kernel/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="$(cd "${SCRIPT_DIR}/../orangepi-4pro-kernel" && pwd)"
elif [ -f "/srv/kernel/orangepi-4pro-kernel/patch/0001-allwinner-a733-orangepi-4-pro-6.18-mainline.patch" ]; then
    REPO_ROOT="/srv/kernel/orangepi-4pro-kernel"
else
    echo "[-] Error: Could not locate orangepi-4pro-kernel repository."
    exit 1
fi

OUTPUT_DIR="${REPO_ROOT}/output_arch"
BUILD_DATE="$(date +%Y%m%d)"
IMAGE_NAME="orangepi4pro-archlinux-arm64-${BUILD_DATE}.img"

echo "========================================================================"
echo "  Building Arch Linux ARM (ALARM) SD Card Image: Orange Pi 4 Pro"
echo "  Kernel Repo:  ${REPO_ROOT}"
echo "========================================================================"

mkdir -p "${OUTPUT_DIR}"

if [ "$(id -u)" -ne 0 ]; then
    echo "[-] Error: This script must be run as root (or with sudo)."
    exit 1
fi

for tool in parted losetup e2fsprogs mkfs.ext4 bsdtar xz curl wget; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "[-] Missing required build tool: $tool"
        exit 1
    fi
done

echo "[*] Step 1: Checking U-Boot boot script..."
if [ ! -f "${REPO_ROOT}/boot/boot.scr" ]; then
    echo "[*] Compiling U-Boot boot.scr..."
    mkimage -C none -A arm -T script -d "${REPO_ROOT}/boot/boot.cmd" "${REPO_ROOT}/boot/boot.scr"
fi

TARGET_IMG="${OUTPUT_DIR}/${IMAGE_NAME}"
WORK_MNT="${OUTPUT_DIR}/rootfs_mnt"
mkdir -p "${WORK_MNT}"

# Create 4.5 GB raw disk image
echo "[*] Creating 4.5 GB raw disk image: ${TARGET_IMG}..."
truncate -s 4800M "${TARGET_IMG}"

# Partitioning disk:
echo "[*] Partitioning disk image..."
parted -s "${TARGET_IMG}" mklabel msdos
parted -s "${TARGET_IMG}" mkpart primary ext4 2048s 100%
parted -s "${TARGET_IMG}" set 1 boot on

LOOP_DEV=$(losetup -fP --show "${TARGET_IMG}")
ROOT_PART="${LOOP_DEV}p1"
echo "[*] Loop device mapped to ${ROOT_PART}"

cleanup() {
    echo "[*] Cleaning up mounts and loop devices..."
    sync
    umount -lf "${WORK_MNT}/proc" 2>/dev/null || true
    umount -lf "${WORK_MNT}/sys" 2>/dev/null || true
    umount -lf "${WORK_MNT}/dev/pts" 2>/dev/null || true
    umount -lf "${WORK_MNT}/dev" 2>/dev/null || true
    umount -lf "${WORK_MNT}" 2>/dev/null || true
    losetup -d "${LOOP_DEV}" 2>/dev/null || true
}
trap cleanup EXIT

echo "[*] Formatting root partition (Ext4, label: ALARM_ROOT)..."
mkfs.ext4 -F -L "ALARM_ROOT" -b 4096 -O ^metadata_csum_seed "${ROOT_PART}"
mount "${ROOT_PART}" "${WORK_MNT}"

echo "[*] Step 2: Fetching official Arch Linux ARM aarch64 generic rootfs..."
ARCH_TAR="${OUTPUT_DIR}/ArchLinuxARM-aarch64-latest.tar.gz"
if [ ! -f "${ARCH_TAR}" ]; then
    echo "[*] Downloading ArchLinuxARM-aarch64-latest.tar.gz..."
    curl -L -o "${ARCH_TAR}" "http://os.archlinuxarm.org/os/ArchLinuxARM-aarch64-latest.tar.gz"
fi

echo "[*] Step 3: Extracting Arch Linux rootfs..."
bsdtar -xpf "${ARCH_TAR}" -C "${WORK_MNT}"

echo "[*] Step 4: Installing bootloader config, DTB, and kernel..."
mkdir -p "${WORK_MNT}/boot/allwinner" "${WORK_MNT}/boot/dtb/allwinner"
cp "${REPO_ROOT}/boot/boot.scr" "${WORK_MNT}/boot/boot.scr"
cp "${REPO_ROOT}/boot/boot.cmd" "${WORK_MNT}/boot/boot.cmd"
cp "${REPO_ROOT}/boot/orangepiEnv.txt" "${WORK_MNT}/boot/orangepiEnv.txt"

if [ -f "${REPO_ROOT}/dts/sun60i-a733-orangepi-4-pro.dtb" ]; then
    cp "${REPO_ROOT}/dts/sun60i-a733-orangepi-4-pro.dtb" "${WORK_MNT}/boot/allwinner/"
    cp "${REPO_ROOT}/dts/sun60i-a733-orangepi-4-pro.dtb" "${WORK_MNT}/boot/dtb/allwinner/"
fi

# Step 5: System configuration & first-boot scripts
echo "[*] Step 5: Configuring Arch Linux system settings..."
echo "archlinux-orangepi4pro" > "${WORK_MNT}/etc/hostname"

cat << 'HOSTS' > "${WORK_MNT}/etc/hosts"
127.0.0.1   localhost
127.0.1.1   archlinux-orangepi4pro
::1         localhost ip6-localhost ip6-loopback
HOSTS

cat << 'MOTD' > "${WORK_MNT}/etc/motd"

     /\         Arch Linux ARM — Orange Pi 4 Pro (Allwinner A733)
    /  \        Kernel: Mainline Linux 6.18.y (ARM64)
   /\   \       Default user: alarm / alarm (sudo) | root / root
  /      \      Package Manager: pacman -Syu
 /   ,,   \     Hardware driver activation portal: https://raccoon.ing
/   |  |  -\
/_-''    ''-_\

MOTD

echo "Arch Linux ARM [Orange Pi 4 Pro A733 / Linux 6.18.y] \n \l" > "${WORK_MNT}/etc/issue"

# Setup first-boot script
cat << 'FIRSTBOOT_SH' > "${WORK_MNT}/usr/local/bin/arch-firstboot.sh"
#!/bin/bash
set -eu

echo "[*] Running Arch Linux ARM first-boot initialization..."

# 1. Expand root filesystem
if command -v growpart >/dev/null 2>&1; then
    echo "[*] Expanding root partition..."
    growpart /dev/mmcblk0 1 || true
    resize2fs /dev/mmcblk0p1 || true
fi

# 2. Initialize pacman keyring
if [ ! -d /etc/pacman.d/gnupg ]; then
    echo "[*] Initializing pacman keys..."
    pacman-key --init || true
    pacman-key --populate archlinuxarm || true
fi

# 3. Generate SSH host keys
if [ ! -f /etc/ssh/ssh_host_ed25519_key ]; then
    echo "[*] Generating SSH host keys..."
    ssh-keygen -A || true
fi

echo "[✓] Arch Linux first-boot setup completed."
FIRSTBOOT_SH
chmod 0755 "${WORK_MNT}/usr/local/bin/arch-firstboot.sh"

cat << 'SVC' > "${WORK_MNT}/etc/systemd/system/arch-firstboot.service"
[Unit]
Description=Arch Linux ARM First-Boot Initialization
After=local-fs.target
Before=network.target sshd.service

[Service]
Type=oneshot
ExecStart=/usr/local/bin/arch-firstboot.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
SVC

# Enable systemd services
mkdir -p "${WORK_MNT}/etc/systemd/system/multi-user.target.wants"
ln -sf /etc/systemd/system/arch-firstboot.service "${WORK_MNT}/etc/systemd/system/multi-user.target.wants/arch-firstboot.service"
ln -sf /usr/lib/systemd/system/systemd-networkd.service "${WORK_MNT}/etc/systemd/system/multi-user.target.wants/systemd-networkd.service"
ln -sf /usr/lib/systemd/system/systemd-resolved.service "${WORK_MNT}/etc/systemd/system/multi-user.target.wants/systemd-resolved.service"
ln -sf /usr/lib/systemd/system/sshd.service "${WORK_MNT}/etc/systemd/system/multi-user.target.wants/sshd.service"

# Setup dhcp on eth0 / end0
mkdir -p "${WORK_MNT}/etc/systemd/network"
cat << 'NET' > "${WORK_MNT}/etc/systemd/network/20-wired.network"
[Match]
Name=eth0 end0 en*

[Network]
DHCP=yes
NET

# Copy orangepi-config if available
if [ -f "/srv/frontend/orangepi-config" ]; then
    cp -v "/srv/frontend/orangepi-config" "${WORK_MNT}/usr/local/bin/orangepi-config"
    chmod 0755 "${WORK_MNT}/usr/local/bin/orangepi-config"
fi

echo "[*] Step 6: Finalizing image and unmounting..."
cleanup
trap - EXIT

echo "[*] Step 7: Compressing image with xz -T0 -9..."
xz -T0 -9 -c "${TARGET_IMG}" > "${TARGET_IMG}.xz"
sha256sum "${TARGET_IMG}.xz" > "${TARGET_IMG}.xz.sha256"

echo "========================================================================"
echo " [✓] Arch Linux ARM SD card image built successfully!"
echo "     Image Archive:   ${TARGET_IMG}.xz"
echo "     SHA256 Checksum: $(cat "${TARGET_IMG}.xz.sha256")"
echo "========================================================================"
