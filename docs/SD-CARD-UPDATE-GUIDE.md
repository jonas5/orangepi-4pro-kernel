# Orange Pi 4 Pro SD Card Offline Update Guide

This guide explains how to update the SD card from another host machine when the board cannot boot or when you need to safely apply updates without live network access to the board.

---

## 1. Safety Rule: Manual Wi-Fi Bring-Up Only
To avoid boot loops or kernel panics preventing network/SSH/WireGuard access:
- **Never enable `aic8800` modules to load automatically at boot.**
- The `update.sh` script automatically blacklists `aic8800_fdrv`, `aic8800_bsp`, `aic8800_btlpm`, and `powervr` in `/etc/modprobe.d/blacklist-aic8800.conf` and `orangepiEnv.txt`.
- When the system is booted and reachable, wireless is brought up manually on demand via:
  ```bash
  sudo /usr/local/bin/aic8800-start.sh
  ```

---

## 2. Prerequisites on Host Machine
- An SD card reader connected to the host machine.
- `curl`, `tar`, and `sudo` installed on the host.
- Host machine must have internet access to reach Pluto (`163.5.85.18`).

---

## 3. Step-by-Step Update Process

### Step 1: Identify SD Card Partitions
Insert the SD card into your host computer and list block devices:
```bash
lsblk -f
```
Locate the SD card device (for example, `/dev/sdX` or `/dev/mmcblk0`):
- **Partition 1 (FAT/ext4, boot files)**: contains `orangepiEnv.txt`, `uImage`, `dtb/`
- **Partition 2 (ext4, root filesystem)**: contains `/lib/modules/`, `/usr/`, `/etc/`

### Step 2: Mount Both Partitions
Create temporary mount directories and mount the partitions:
```bash
sudo mkdir -p /mnt/sdboot /mnt/sdroot
sudo mount /dev/sdX1 /mnt/sdboot
sudo mount /dev/sdX2 /mnt/sdroot
```
*(Replace `/dev/sdX1` and `/dev/sdX2` with your actual device names).*

### Step 3: Run the Updater Script
Run the updater and explicitly pass the boot and rootfs mount points:
```bash
curl -sSL http://163.5.85.18/update.sh | sudo bash -s -- /mnt/sdboot /mnt/sdroot
```

#### What the script does:
1. **Boot Partition (`/mnt/sdboot`)**:
   - Backs up existing boot files to `backup_<timestamp>/`.
   - Updates `uImage`, `Image`, `uInitrd`, and the `sun60i-a733-orangepi-4-pro.dtb` devicetree (with working `operating-points-v2` frequency scaling).
   - Preserves the board's existing `rootdev=` in `orangepiEnv.txt`.
   - Sets `cma=1024M` and keeps the module blacklists in `extraargs`.
2. **Root Partition (`/mnt/sdroot`)**:
   - Downloads and installs the kernel modules for `6.18.56-dirty`.
   - Installs firmware into `/lib/firmware/`.
   - Installs PowerVR Vulkan stack into `/usr/lib/aarch64-linux-gnu/`.
   - Installs `/etc/modprobe.d/blacklist-aic8800.conf` so the board always boots cleanly without hanging.
   - Installs `/usr/local/bin/aic8800-start.sh` for safe sequenced driver testing.

### Step 4: Unmount Cleanly
Ensure all data is written to disk before unplugging the card:
```bash
sync
sudo umount /mnt/sdboot /mnt/sdroot
```

### Step 5: Boot the Orange Pi
1. Insert the SD card back into the Orange Pi 4 Pro.
2. Connect power and Ethernet.
3. Once booted, log in via SSH (or WireGuard: `10.9.0.2`).
4. To test Wi-Fi manually when ready:
   ```bash
   sudo /usr/local/bin/aic8800-start.sh
   ```
