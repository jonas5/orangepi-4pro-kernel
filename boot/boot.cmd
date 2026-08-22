# Orange Pi 4 Pro - Linux 7.1.5 boot script
# Load environment
setenv fdt_high "off"
setenv initrd_high "off"
ext4load mmc 0:1 ${loadaddr} /boot/orangepiEnv.txt
env import -t ${loadaddr} ${filesize}

# Get root partition UUID
part uuid mmc 0:1 rootdev

# Build boot arguments
setenv bootargs "console=ttyS2,115200 root=PARTUUID=${rootdev} rootwait rw rootfstype=ext4 loglevel=${verbosity} sunxi_fc=fdt,${fdtfile} cma=${cma} ${extraargs}"

# Charger mode: set "charger_mode=powerbank" in orangepiEnv.txt
# to limit USB input current to 500mA (safe for power banks)
if test -n "${charger_mode}"; then
	setenv bootargs "${bootargs} charger_mode=${charger_mode}"
fi

# Load boot files
ext4load mmc 0:1 ${ramdisk_addr_r} /boot/uInitrd
setenv initrd_size ${filesize}
ext4load mmc 0:1 ${kernel_addr_r} /boot/Image
ext4load mmc 0:1 ${fdt_addr_r} /boot/dtb/allwinner/sun60i-a733-orangepi-4-pro.dtb

echo "=== U-BOOT BDINFO ==="
bdinfo

echo "=== U-BOOT ENV ==="
printenv kernel_addr_r ramdisk_addr_r fdt_addr_r initrd_size fdt_high initrd_high bootargs

echo "=== KERNEL HEADER AT ${kernel_addr_r} ==="
md.l ${kernel_addr_r} 0x10

echo "=== DTB HEADER AT ${fdt_addr_r} ==="
md.l ${fdt_addr_r} 0x10

fdt addr ${fdt_addr_r}
fdt resize 0x10000

echo "=== FDT CHOSEN NODE ==="
fdt print /chosen

echo "=== EXECUTING BOOTI ==="
booti ${kernel_addr_r} ${ramdisk_addr_r}:${initrd_size} ${fdt_addr_r}
