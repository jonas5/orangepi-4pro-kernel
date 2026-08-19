# Orange Pi 4 Pro - Linux 7.1.5 boot script
# Load environment
ext4load mmc 0:1 ${loadaddr} /boot/orangepiEnv.txt
env import -t ${loadaddr} ${filesize}

# Get root partition UUID
part uuid mmc 0:1 rootdev

# Boot arguments
setenv bootargs "console=ttyS0,115200 root=PARTUUID=${rootdev} rootwait rw rootfstype=ext4 loglevel=${verbosity} sunxi_fc=fdt,${fdtfile} cma=${cma}"

# Load boot files
ext4load mmc 0:1 ${ramdisk_addr_r} /boot/uInitrd
ext4load mmc 0:1 ${kernel_addr_r} /boot/Image
ext4load mmc 0:1 ${fdt_addr_r} /boot/dtb/allwinner/sun60i-a733-orangepi-4-pro.dtb

# Boot
booti ${kernel_addr_r} ${ramdisk_addr_r}:${filesize} ${fdt_addr_r}
