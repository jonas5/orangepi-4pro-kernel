<pre>
[76]HELLO! BOOT0 is starting!
[79]BOOT0 commit : {4721ad08}
[82]board gpio withstand voltage auto mode
[97]vaild para:4  select dram para3
[mmc]: mmc driver ver 2025-10-16 17:10
[mmc]: Wrong media type 0x0
[mmc]: ***Try SD card 0***
[mmc]: HSSDR52/SDR25 4 bit
[mmc]: 50000000 Hz
[mmc]: 29818 MB
[mmc]: ***SD/MMC 0 init OK!!!***
[134]boot param - magic error
[136]DRAM BOOT DRIVE INFO: V0.6034
[141]DRAM_VCC set to 560 mv
[144]DRAM CLK =1800 MHZ
[146]DRAM Type =9 (8:LPDDR4,9:LPDDR5)
[298]Training result is = 7
[300]DRAM Pstate 1 training, frequency is 1200 Mhz
[485]Training result is = 7
[488]DRAM Pstate 2 training, frequency is 800 Mhz
[845]Training result is = 7
[848]DRAM Pstate 3 training, frequency is 400 Mhz
[5848]Training result is = 7
[5851]DRAM Pstate 0 training, frequency is 1800 Mhz
[5860]Actual DRAM SIZE =12288 M
[5863]DRAM SIZE =12288 MBytes, para1 = a11a, para2 = 30001001, dram_tpr13 = 10065
[5878]DRAM simple test OK.
[5942]error: dtb not found for scp
[5946]mmc not para
[5948]Jump to ATF: monitor_base = 0x48000000, uboot_base = 0x4a000000, optee_base = 0x0
NOTICE:  BL31: OP-TEE 32bit detected
NOTICE:  BL31: U-BOOT 32bit detected
NOTICE:  BL31: v2.5(debug):7dddb263a-dirty
NOTICE:  BL31: Built : 21:40:11, Dec 30 2024
NOTICE:  BL31: No DTB found.
ERROR:   Error initializing runtime service opteed_fast
amp fw boot before!!!
amp fw boot after!!!


U-Boot 2018.07-orangepi-config-dirty (Oct 26 2026 - 15:26:23 +0100) Allwinner Technology

[06.004]CPU:   Allwinner Family
[06.007]Model: sun60iw2
I2C:   ready
[06.072]DRAM:  12 GiB
[06.081]Relocation Offset is: 6dd91000
[06.423]secure enable bit: 0
[06.427]PMU: AXP8191
[06.429]PMU: AXP8191 VER_A
[06.434]bmu_axp515_probe pmic_bus_read fail
[06.438][AXP8191] onoff status: 0x50 = 0x1, 0x51 = 0x0
[06.443][AXP8191] charge status: 0x4 = 0x0
[06.446]CPU=1014 MHz,PLL6=1200 Mhz,AHB=200 Mhz, APB1=24Mhz  MBus=600Mhz
[06.453]gic: sec monitor mode
SPI ALL:   ready
[06.496]flash init start
[06.498]workmode = 0,storage type = 1
[06.502][mmc]: mmc driver ver uboot2018:2024-10-18 9:01:00 -- uboot2018:2025-03-28 10:03:00
[06.510][mmc]: get sdc_type fail and use default host:tm1.
[06.518][mmc]: can't find node "mmc0",will add new node
[06.523][mmc]: fdt err returned <no error>
[06.527][mmc]: Using default timing para
[06.530][mmc]: SUNXI SDMMC Controller Version:0x50530
[06.548][mmc]: card_caps:0x3000000a
[06.551][mmc]: host_caps:0x3000003f
[06.555]sunxi flash init ok
get value error
[06.560]Loading Environment from EXT4... ** No device specified **
Failed (-5)
No ethernet found.
[06.599]NOT support bridge yet
[06.602]Warn: no find panel or bridge
[06.705]drm hdmi get mode: 2560x1440@60Hz
hdmi0@5520000:  detailed mode clock 241500 kHz, flags[5]
    H: 2560 2608 2640 2720
    V: 1440 1443 1448 1481
bus_format: 0
mmc0 is current device

Partition Map for MMC device 0  --   Partition Type: DOS

Part    Start Sector    Num Sectors     UUID            Type
  1     65536           60358656        aa9783e8-01     83
Trying to get file size from 0:1, file: /boot/boot.bmp
Found file: /boot/boot.bmp, size: 0x38438 (230456 bytes)
230456 bytes read in 17 ms (12.9 MiB/s)
clk_set_rate: <NULL> has NULL parent
[06.848]suxni tcon hdmi set rate: 241500000Hz and get diff: -241500000Hz
[06.916]battery temp is -1
[06.923](not enough data: expected >= 8 cells, got 4 cells)
pcie link up success
PCIe speed of Gen3
[ufs]info:Driver version 0.0.24 2024.12.30 16:00
[ufs]err:sunxi_ufs_rmmi_config: wait sram init done timeout
[ufs]err:Failed to config rmmi[ufs]err:Phy setup failed (-110)
[ufs]err:Phy init failed
[ufs]err:sunxi_ufs_rmmi_config: wait sram init done timeout
[ufs]err:Failed to config rmmi[ufs]err:Phy setup failed (-110)
[ufs]err:Phy init failed
[ufs]err:sunxi_ufs_rmmi_config: wait sram init done timeout
[ufs]err:Failed to config rmmi[ufs]err:Phy setup failed (-110)
[ufs]err:Phy init failed
[ufs]err:sunxi_ufs_rmmi_config: wait sram init done timeout
[ufs]err:Failed to config rmmi[ufs]err:Phy setup failed (-110)
[ufs]err:Phy init failed
Hit any key to stop autoboot:  0
mmc0 is current device
Scanning mmc 0:1...
Found U-Boot script /boot/boot.scr
2638 bytes read in 10 ms (256.8 KiB/s)
## Executing script at 43100000
Boot script loaded from mmc 0
218 bytes read in 8 ms (26.4 KiB/s)
22205345 bytes read in 951 ms (22.3 MiB/s)
27744832 bytes read in 1177 ms (22.5 MiB/s)
144505 bytes read in 52 ms (2.6 MiB/s)
481 bytes read in 8 ms (58.6 KiB/s)
Applying user provided DT overlay gpu-clk.dtbo
440 bytes read in 9 ms (46.9 KiB/s)
Applying user provided DT overlay dsu-clk.dtbo
## Booting kernel from Legacy Image at 41000000 ...
   Image Name:   Linux kernel
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    27744768 Bytes = 26.5 MiB
   Load Address: 41000000
   Entry Point:  41000000
   Verifying Checksum ... OK
## Loading init Ramdisk from Legacy Image at 43300000 ...
   Image Name:   uInitrd
   Image Type:   ARM Linux RAMDisk Image (gzip compressed)
   Data Size:    22205281 Bytes = 21.2 MiB
   Load Address: 00000000
   Entry Point:  00000000
   Verifying Checksum ... OK
## Flattened Device Tree blob at 43000000
   Booting using the fdt blob at 0x43000000
   Loading Ramdisk to 48ad2000, end 49fff361 ... OK
ERROR: reserving fdt memory region failed (addr=48000000 size=1000000)
[11.237]Starting kernel ...

[11.240]total: 11240 ms

[11.242][mmc]: MMC Device 2 not found
[11.246][mmc]: mmc 2 not find, so not exit
ret 0
NOTICE:  [SCP] :wait arisc ready....
NOTICE:  [SCP] :arisc version: []
NOTICE:  [SCP] :arisc startup ready
NOTICE:  [SCP] :arisc startup notify message feedback
NOTICE:  [SCP] :sunxi-arisc driver is starting
NOTICE:  BL3-1: Next image address = 0x41000000
NOTICE:  BL3-1: Next image spsr = 0x3c5
[    0.000000][    T0] AW BSP version: allwinner-bsp 656a550
AW BSP version: allwinner-bsp 656a550
done.
Begin: Mounting root file system ... Begin: Running /scripts/local-top ... done.
Begin: Running /scripts/local-premount ... done.
Begin: Will now check root file system ... fsck from util-linux 2.41.5
[/sbin/fsck.ext4 (1) -- /dev/mmcblk0p1] fsck.ext4 -a -C0 /dev/mmcblk0p1
opi_root: recovering journal
opi_root: clean, 232749/1877568 files, 5074346/7544832 blocks
done.
done.
Begin: Running /scripts/local-bottom ... done.
Begin: Running /scripts/init-bottom ... done.
SELinux:  Could not open policy file <= /etc/selinux/default/policy/policy.35:  No such file or directory
p104
Welcome to Orange Pi 1.1.0 Trixie!

[  OK  ] Created slice system-getty.slice - Slice /system/getty.
[  OK  ] Created slice system-modprobe.slice - Slice /system/modprobe.
[  OK  ] Created slice system-serial\x2dgetty.slice - Slice /system/serial-getty.
[  OK  ] Created slice system-wg\x2dquick.slice - Slice /system/wg-quick.
[  OK  ] Created slice user.slice - User and Session Slice.
[  OK  ] Started systemd-ask-password-wall.path - Forward Password Requests to Wall Directory Watch.
[  OK  ] Set up automount proc-sys-fs-binfmt_misc.automount - Arbitrary Executable File Formats File System Automount Point.
         Expecting device dev-ttyS0.device - /dev/ttyS0...
[  OK  ] Reached target nss-user-lookup.target - User and Group Name Lookups.
[  OK  ] Reached target remote-fs.target - Remote File Systems.
[  OK  ] Reached target slices.target - Slice Units.
[  OK  ] Reached target swap.target - Swaps.
[  OK  ] Listening on syslog.socket - Syslog Socket.
[  OK  ] Listening on systemd-creds.socket - Credential Encryption/Decryption.
[  OK  ] Listening on systemd-initctl.socket - initctl Compatibility Named Pipe.
[  OK  ] Listening on systemd-journald-dev-log.socket - Journal Socket (/dev/log).
[  OK  ] Listening on systemd-journald.socket - Journal Sockets.
[  OK  ] Listening on systemd-udevd-control.socket - udev Control Socket.
[  OK  ] Listening on systemd-udevd-kernel.socket - udev Kernel Socket.
         Mounting dev-hugepages.mount - Huge Pages File System...
         Mounting dev-mqueue.mount - POSIX Message Queue File System...
         Mounting run-lock.mount - Legacy Locks Directory /run/lock...
         Mounting sys-kernel-debug.mount - Kernel Debug File System...
         Starting fake-hwclock-load.service - Restore the current clock...
         Starting keyboard-setup.service - Set the console keyboard layout...
         Starting kmod-static-nodes.service - Create List of Static Device Nodes...
         Starting modprobe@configfs.service - Load Kernel Module configfs...
         Starting modprobe@drm.service - Load Kernel Module drm...
         Starting modprobe@efi_pstore.service - Load Kernel Module efi_pstore...
         Starting modprobe@fuse.service - Load Kernel Module fuse...
         Starting systemd-modules-load.service - Load Kernel Modules...
         Starting systemd-udev-load-credentials.service - Load udev Rules from Credentials...
         Starting systemd-udev-trigger.service - Coldplug All udev Devices...
[  OK  ] Mounted dev-hugepages.mount - Huge Pages File System.
[  OK  ] Mounted dev-mqueue.mount - POSIX Message Queue File System.
[  OK  ] Mounted run-lock.mount - Legacy Locks Directory /run/lock.
[  OK  ] Mounted sys-kernel-debug.mount - Kernel Debug File System.
[  OK  ] Finished kmod-static-nodes.service - Create List of Static Device Nodes.
[  OK  ] Finished modprobe@configfs.service - Load Kernel Module configfs.
[  OK  ] Finished modprobe@drm.service - Load Kernel Module drm.
[  OK  ] Finished modprobe@efi_pstore.service - Load Kernel Module efi_pstore.
         Mounting sys-kernel-config.mount - Kernel Configuration File System...
         Starting systemd-tmpfiles-setup-dev-early.service - Create Static Device Nodes in /dev gracefully...
[  OK  ] Finished fake-hwclock-load.service - Restore the current clock.
[  OK  ] Finished modprobe@fuse.service - Load Kernel Module fuse.
         Mounting sys-fs-fuse-connections.mount - FUSE Control File System...
         Starting systemd-remount-fs.service - Remount Root and Kernel File Systems...
[  OK  ] Mounted sys-kernel-config.mount - Kernel Configuration File System.
[  OK  ] Mounted sys-fs-fuse-connections.mount - FUSE Control File System.
[  OK  ] Finished systemd-udev-load-credentials.service - Load udev Rules from Credentials.
[  OK  ] Finished systemd-modules-load.service - Load Kernel Modules.
         Starting systemd-sysctl.service - Apply Kernel Variables...
[  OK  ] Finished systemd-remount-fs.service - Remount Root and Kernel File Systems.
         Starting systemd-random-seed.service - Load/Save OS Random Seed...
[  OK  ] Finished systemd-tmpfiles-setup-dev-early.service - Create Static Device Nodes in /dev gracefully.
[  OK  ] Finished keyboard-setup.service - Set the console keyboard layout.
         Starting systemd-tmpfiles-setup-dev.service - Create Static Device Nodes in /dev...
[  OK  ] Finished systemd-sysctl.service - Apply Kernel Variables.
[  OK  ] Finished systemd-tmpfiles-setup-dev.service - Create Static Device Nodes in /dev.
[  OK  ] Reached target local-fs-pre.target - Preparation for Local File Systems.
         Starting systemd-udevd.service - Rule-based Manager for Device Events and Files...
[  OK  ] Finished systemd-udev-trigger.service - Coldplug All udev Devices.
         Starting ifupdown-pre.service - Helper to synchronize boot up for ifupdown...
         Starting systemd-udev-settle.service - Wait for udev To Complete Device Initialization...
[  OK  ] Finished ifupdown-pre.service - Helper to synchronize boot up for ifupdown.
[  OK  ] Started systemd-udevd.service - Rule-based Manager for Device Events and Files.
         Starting plymouth-start.service - Show Plymouth Boot Screen...
[  OK  ] Found device dev-ttyS0.device - /dev/ttyS0.
[  OK  ] Started plymouth-start.service - Show Plymouth Boot Screen.
[  OK  ] Started systemd-ask-password-plymouth.path - Forward Password Requests to Plymouth Directory Watch.
[  OK  ] Reached target paths.target - Path Units.
[  OK  ] Reached target usb-gadget.target - Hardware activated USB gadget.
npu[158][158] vipcore, platform driver init
npu[158][158] vipcore, insmod param cpu_physical=0x0
npu[158][158] vipcore, insmod param cpu_physical=0x0
npu[158][158] vipcore, insmod param cpu_physical=0x0
npu[158][158] vipcore, insmod param cpu_physical=0x0
npu[158][158] vipcore, platform device compatible=allwinner,npu
npu[158][158] vipcore, platform driver device=0xffff0000c0dbf410
npu[158][158] clock_rate: 26000000
npu[158][158] vipcore irq number is 503.
npu[158][158] npu power domains status: 0
npu[158][158] Get NPU VF GEAR FAIL!
npu[158][158] npu set_vf: 0
npu[158][158] EXTRA: 0x, SID: 0x0, max_freq: 1008000000, default_freq: 1008000000
npu[158][158] current dvfs: 4, vf index: 400
[  OK  ] Finished systemd-random-seed.service - Load/Save OS Random Seed.
npu[158][158] successed to set opp regulator
npu[158][158] Supported vf list:
npu[158][158]     freq: 492000000, vol: 800000
npu[158][158]     freq: 852000000, vol: 800000
npu[158][158] get freq list and add opp successfully
npu[158][158] target rate: 852000000, vol: 800000, target_fscale: 100
npu[158][158] npu devfreq added
npu[158][158] successed to register cooling device
npu[158][158] enable aw vip devfreq
npu[158][158] NPU Use VF0400, use freq 852 MHz
npu[158][158] Want set pclk rate(852000000) support(852000000) real(852000000)
npu[158][158] Want set mclk rate(852000000) support(852000000) real(852000000)
npu[158][158] Get NPU Regulator Control FAIL!
npu[158][158] Want set npu vol(800000) now vol(800000)
npu[158][158] Want set pclk rate(852000000) support(852000000) real(852000000)
npu[158][158] Want set mclk rate(852000000) support(852000000) real(852000000)
npu[158][158] vipcore, device_cnt=1, core_cnt=1
npu[158][158] core_0, request irqline=503, name=vipcore_0
npu[158][158] vipcore, allocate page for video memory, size: 0x400000 bytes
npu[158][158] vipcore, heap_cnt=0, reserved video memory cpu_phy=0x49400000, vip_phy=0x49400000, size=0x400000
npu[158][158] VIPLite driver version 2.0.3.5-AW-2026-03-17
[  OK  ] Listening on systemd-rfkill.socket - Load/Save RF Kill Switch Status /dev/rfkill Watch.
[  OK  ] Reached target sound.target - Sound Card.
         Starting systemd-rfkill.service - Load/Save RF Kill Switch Status...
[  OK  ] Finished systemd-udev-settle.service - Wait for udev To Complete Device Initialization.
         Starting zfs-load-module.service - Install ZFS kernel module...
[  OK  ] Started systemd-rfkill.service - Load/Save RF Kill Switch Status.
         Mounting tmp.mount - /tmp...
[  OK  ] Mounted tmp.mount - /tmp.
[  OK  ] Finished zfs-load-module.service - Install ZFS kernel module.
         Starting zfs-import-cache.service - Import ZFS pools by cache file...
[FAILED] Failed to start zfs-import-cache.service - Import ZFS pools by cache file.
See 'systemctl status zfs-import-cache.service' for details.
[  OK  ] Reached target zfs-import.target - ZFS pool import target.
         Starting zfs-mount.service - Mount ZFS filesystems...
         Starting zfs-volume-wait.service - Wait for ZFS Volume (zvol) links in /dev...
[  OK  ] Finished zfs-mount.service - Mount ZFS filesystems.
[  OK  ] Finished zfs-volume-wait.service - Wait for ZFS Volume (zvol) links in /dev.
[  OK  ] Reached target local-fs.target - Local File Systems.
[  OK  ] Reached target zfs-volumes.target - ZFS volumes are ready.
[  OK  ] Listening on systemd-sysext.socket - System Extension Image Management.
         Starting console-setup.service - Set console font and keymap...
         Starting orangepi-zram-config.service - Orange Pi ZRAM config...
         Starting plymouth-read-write.service - Tell Plymouth To Write Out Runtime Data...
         Starting systemd-binfmt.service - Set Up Additional Binary Formats...
[  OK  ] Finished console-setup.service - Set console font and keymap.
[  OK  ] Finished plymouth-read-write.service - Tell Plymouth To Write Out Runtime Data.
         Mounting proc-sys-fs-binfmt_misc.mount - Arbitrary Executable File Formats File System...
[  OK  ] Mounted proc-sys-fs-binfmt_misc.mount - Arbitrary Executable File Formats File System.
[  OK  ] Finished systemd-binfmt.service - Set Up Additional Binary Formats.
[  OK  ] Finished orangepi-zram-config.service - Orange Pi ZRAM config.
         Starting orangepi-ramlog.service - Orange Pi memory supported logging...
[  OK  ] Finished orangepi-ramlog.service - Orange Pi memory supported logging.
         Starting systemd-journald.service - Journal Service...
[  OK  ] Started systemd-journald.service - Journal Service.
         Starting systemd-journal-flush.service - Flush Journal to Persistent Storage...
[  OK  ] Finished systemd-journal-flush.service - Flush Journal to Persistent Storage.
         Starting systemd-tmpfiles-setup.service - Create System Files and Directories...
[  OK  ] Finished systemd-tmpfiles-setup.service - Create System Files and Directories.
[  OK  ] Started haveged.service - Entropy Daemon based on the HAVEGE algorithm.
[  OK  ] Started resolvconf.service - Nameserver information manager.
[  OK  ] Reached target sysinit.target - System Initialization.
[  OK  ] Started apt-daily.timer - Daily apt download activities.
[  OK  ] Started apt-daily-upgrade.timer - Daily apt upgrade and clean activities.
[  OK  ] Started dpkg-db-backup.timer - Daily dpkg database backup timer.
[  OK  ] Started e2scrub_all.timer - Periodic ext4 Online Metadata Check for All Filesystems.
[  OK  ] Started fake-hwclock-save.timer - Periodically save current clock.
[  OK  ] Started fstrim.timer - Discard unused filesystem blocks once a week.
[  OK  ] Started logrotate.timer - Daily rotation of log files.
[  OK  ] Started man-db.timer - Daily man-db regeneration.
[  OK  ] Started sysstat-collect.timer - Run system activity accounting tool every 10 minutes.
[  OK  ] Started sysstat-rotate.timer - Rotate daily system activity data file at midnight.
[  OK  ] Started sysstat-summary.timer - Generate summary of yesterday's process accounting.
[  OK  ] Started systemd-tmpfiles-clean.timer - Daily Cleanup of Temporary Directories.
[  OK  ] Reached target timers.target - Timer Units.
[  OK  ] Listening on dbus.socket - D-Bus System Message Bus Socket.
[  OK  ] Listening on lircd.socket.
[  OK  ] Listening on sshd-unix-local.socket - OpenSSH Server Socket (systemd-ssh-generator, AF_UNIX Local).
[  OK  ] Listening on systemd-hostnamed.socket - Hostname Service Socket.
[  OK  ] Reached target sockets.target - Socket Units.
         Starting orangepi-hardware-monitor.service - Orange Pi hardware monitoring...
         Starting orangepi-hardware-optimize.service - Orange Pi hardware optimization...
[  OK  ] Finished orangepi-hardware-optimize.service - Orange Pi hardware optimization.
[  OK  ] Finished orangepi-hardware-monitor.service - Orange Pi hardware monitoring.
[  OK  ] Reached target basic.target - Basic System.
         Starting accounts-daemon.service - Accounts Service...
         Starting aic8800.service - AIC8800 Wi-Fi Bring-Up...
         Starting chrony.service - chrony, an NTP client/server...
[  OK  ] Started cron.service - Regular background program processing daemon.
         Starting dbus.service - D-Bus System Message Bus...
         Starting e2scrub_reap.service - Remove Stale Online ext4 Metadata Check Snapshots...
         Starting lircd-setup.service - lircd(8) initialization helper tool...
         Starting lm-sensors.service - Initialize hardware monitoring sensors...
         Starting rsyslog.service - System Logging Service...
         Starting smartmontools.service - Self Monitoring and Reporting Technology (SMART) Daemon...
         Starting sysfsutils.service - Apply sysfs variables...
         Starting sysstat.service - Resets System Activity Logs...
         Starting systemd-logind.service - User Login Management...
         Starting udisks2.service - Disk Manager...
         Starting zfs-share.service - ZFS file system shares...
[  OK  ] Finished zfs-share.service - ZFS file system shares.
[  OK  ] Reached target zfs.target - ZFS startup target.
[  OK  ] Started chrony.service - chrony, an NTP client/server.
[  OK  ] Finished e2scrub_reap.service - Remove Stale Online ext4 Metadata Check Snapshots.
[  OK  ] Finished sysstat.service - Resets System Activity Logs.
[  OK  ] Started systemd-logind.service - User Login Management.
[  OK  ] Finished lm-sensors.service - Initialize hardware monitoring sensors.
[  OK  ] Started rsyslog.service - System Logging Service.
[  OK  ] Started smartmontools.service - Self Monitoring and Reporting Technology (SMART) Daemon.
[  OK  ] Started dbus.service - D-Bus System Message Bus.
         Starting wpa_supplicant.service - WPA supplicant...
[  OK  ] Finished sysfsutils.service - Apply sysfs variables.
         Starting polkit.service - Authorization Manager...
[  OK  ] Started wpa_supplicant.service - WPA supplicant.
[  OK  ] Started polkit.service - Authorization Manager.
[  OK  ] Started accounts-daemon.service - Accounts Service.
[  OK  ] Finished lircd-setup.service - lircd(8) initialization helper tool.
[  OK  ] Started udisks2.service - Disk Manager.
aicbsp_init
aicbsp_init, Driver Release Tag: aic-bsp-compatible(sdio)-20250923-003
         Starting systemd-rfkill.service - Load/Save RF Kill Switch Status...
aicbsp: aicbsp_set_subsys, subsys: AIC_BLUETOOTH, state to: 1
aicbsp: aicbsp_set_subsys, power state change to 1 dure to AIC_BLUETOOTH
aicbsp: aicbsp_platform_power_on
aicbsp: aicbsp_platform_power_on disable MMC_CAP_UHS_DDR50 for host 00000000e7be2a01
aicbsp: aicbsp_sdio_probe:2
aicbsp: aicbsp_sdio_probe:2, matched chip: aic8800d80
aicbsp: aicbsp_sdio_probe after replace:1
aicbsp: aicbsp_sdio_probe host max clock frequecy: 208000000
aicbsp: aicwf_sdio_reg_init
aicbsp: Set SDIO Clock 24 MHz
aicbsp:sdio_f1_flag 0
aicbsp: aicbsp_8800d80_fw_init, rev id: 0x7, subrev id: 0x82, is_chip_id_h: 0
rwnx_request_firmware, name: aic8800d80/fw_patch_table_8800d80_u02.bin
rwnx_request_firmware, name: aic8800d80/aichw.conf
aicbt_patch_info_unpack head->len:4 base_len:4
aicbt_patch_info_unpack memcpy_len:4
aicbt_patch_info_unpack adid_addrinf:1e602c addr_adid:201940
rwnx_request_firmware, name: aic8800d80/fw_adid_8800d80_u02.bin
rwnx_request_firmware, name: aic8800d80/fw_patch_8800d80_u02.bin
aicbt_patch_table_load bt uart baud: 1500000, flowctrl: 1, lpm_enable: 1, tx_pwr: 28463, bt mode:5.
>>> rwnx_mod_init()
rwnx 20250923-019-6.4.3.0 - - 241c091M (master)
Driver Release Tag: aic-rwnx-compatible(sdio)-20250923-019
aicbsp: bt patch version: - Jul 11 2024 10:38:54 - git 73d2ce5
rwnx_request_firmware, name: aic8800d80/fmacfw_8800d80_u02.bin
[*     ] (1 of 2) Job aic8800.service/start running (7s / no limit)
rd_version_val=06090101
[  OK  ] Started systemd-rfkill.service - Load/Save RF Kill Switch Status.
aicbsp: aicbsp_set_subsys, power state no need to change, current: 1
aicbsp: aicbsp_resv_mem_alloc_skb, alloc resv_mem_txdata succuss, id: 0, size: 98304
>>> rwnx_platform_init()
>>> rwnx_cfg80211_init()
rwnx_cfg80211_init sizeof(struct rwnx_hw):18288
>>> rwnx_init_aic()
>>> rwnx_cmd_mgr_init()
tcp_ack_init
aicwf_prealloc_txq_alloc, enter
aicwf_prealloc_txq_alloc size is diff will to be kzalloc
aicwf_prealloc_txq_alloc txq kzalloc successful
>>> rwnx_send_set_stack_start_req()
is 5g support = 1, vendor_info = 0x21
Firmware Version: la Jul 03 2024 15:25:18 - g21266be
>>> rwnx_platform_on()
>>> rwnx_plat_fmac_load()
userconfig file path:aic8800d80/aic_userconfig_8800d80.txt
rwnx_plat_userconfig_parsing, enable = 1
rwnx_plat_userconfig_parsing, lvl_11b_11ag_1m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_2m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_5m5_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_11m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_6m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_9m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_12m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_18m_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11b_11ag_24m_2g4 = 16
rwnx_plat_userconfig_parsing, lvl_11b_11ag_36m_2g4 = 16
rwnx_plat_userconfig_parsing, lvl_11b_11ag_48m_2g4 = 15
rwnx_plat_userconfig_parsing, lvl_11b_11ag_54m_2g4 = 15
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs0_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs1_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs2_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs3_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs4_2g4 = 16
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs5_2g4 = 16
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs6_2g4 = 15
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs7_2g4 = 15
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs8_2g4 = 14
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs9_2g4 = 14
rwnx_plat_userconfig_parsing, lvl_11ax_mcs0_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs1_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs2_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs3_2g4 = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs4_2g4 = 16
rwnx_plat_userconfig_parsing, lvl_11ax_mcs5_2g4 = 16
rwnx_plat_userconfig_parsing, lvl_11ax_mcs6_2g4 = 15
rwnx_plat_userconfig_parsing, lvl_11ax_mcs7_2g4 = 15
rwnx_plat_userconfig_parsing, lvl_11ax_mcs8_2g4 = 14
rwnx_plat_userconfig_parsing, lvl_11ax_mcs9_2g4 = 14
rwnx_plat_userconfig_parsing, lvl_11ax_mcs10_2g4 = 13
rwnx_plat_userconfig_parsing, lvl_11ax_mcs11_2g4 = 13
rwnx_plat_userconfig_parsing, lvl_11a_6m_5g = 18
rwnx_plat_userconfig_parsing, lvl_11a_9m_5g = 18
rwnx_plat_userconfig_parsing, lvl_11a_12m_5g = 18
rwnx_plat_userconfig_parsing, lvl_11a_18m_5g = 18
rwnx_plat_userconfig_parsing, lvl_11a_24m_5g = 16
rwnx_plat_userconfig_parsing, lvl_11a_36m_5g = 16
rwnx_plat_userconfig_parsing, lvl_11a_48m_5g = 15
rwnx_plat_userconfig_parsing, lvl_11a_54m_5g = 15
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs0_5g = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs1_5g = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs2_5g = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs3_5g = 18
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs4_5g = 16
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs5_5g = 16
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs6_5g = 15
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs7_5g = 15
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs8_5g = 14
rwnx_plat_userconfig_parsing, lvl_11n_11ac_mcs9_5g = 14
rwnx_plat_userconfig_parsing, lvl_11ax_mcs0_5g = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs1_5g = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs2_5g = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs3_5g = 18
rwnx_plat_userconfig_parsing, lvl_11ax_mcs4_5g = 16
rwnx_plat_userconfig_parsing, lvl_11ax_mcs5_5g = 16
rwnx_plat_userconfig_parsing, lvl_11ax_mcs6_5g = 14
rwnx_plat_userconfig_parsing, lvl_11ax_mcs7_5g = 14
rwnx_plat_userconfig_parsing, lvl_11ax_mcs8_5g = 13
rwnx_plat_userconfig_parsing, lvl_11ax_mcs9_5g = 13
rwnx_plat_userconfig_parsing, lvl_11ax_mcs10_5g = 12
rwnx_plat_userconfig_parsing, lvl_11ax_mcs11_5g = 12
rwnx_plat_userconfig_parsing, lvl_adj_enable = 0
rwnx_plat_userconfig_parsing, lvl_adj_2g4_chan_1_4 = 0
rwnx_plat_userconfig_parsing, lvl_adj_2g4_chan_5_9 = 0
rwnx_plat_userconfig_parsing, lvl_adj_2g4_chan_10_13 = 0
rwnx_plat_userconfig_parsing, ofst_enable = 0
rwnx_plat_userconfig_parsing, ofst_2g4_11b_chan_1_4 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_11b_chan_5_9 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_11b_chan_10_13 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_ofdm_highrate_chan_1_4 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_ofdm_highrate_chan_5_9 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_ofdm_highrate_chan_10_13 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_ofdm_lowrate_chan_1_4 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_ofdm_lowrate_chan_5_9 = 0
rwnx_plat_userconfig_parsing, ofst_2g4_ofdm_lowrate_chan_10_13 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_lowrate_chan_42 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_lowrate_chan_58 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_lowrate_chan_106 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_lowrate_chan_122 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_lowrate_chan_138 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_lowrate_chan_155 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_highrate_chan_42 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_highrate_chan_58 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_highrate_chan_106 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_highrate_chan_122 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_highrate_chan_138 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_highrate_chan_155 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_midrate_chan_42 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_midrate_chan_58 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_midrate_chan_106 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_midrate_chan_122 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_midrate_chan_138 = 0
rwnx_plat_userconfig_parsing, ofst_5g_ofdm_midrate_chan_155 = 0
rwnx_plat_userconfig_parsing, xtal_enable = 0
rwnx_plat_userconfig_parsing, xtal_cap = 24
rwnx_plat_userconfig_parsing, xtal_cap_fine = 31
>>> rwnx_send_txpwr_lvl_v3_req()
>>> rwnx_send_txpwr_lvl_adj_req()
>>> rwnx_msg_free()
>>> rwnx_send_txpwr_ofst2x_req()
rwnx_send_txpwr_ofst2x_req:Do not use txpwr_ofst2x
>>> rwnx_msg_free()
>>> rwnx_send_rf_calib_req()
get macaddr: 98:8c:bb:5b:51:30
>>> rwnx_send_reset()
>>> rwnx_send_version_req()
>>> rwnx_set_vers()
>>> rwnx_send_me_config_req()
>>> rwnx_send_me_chan_config_req()
>>> rwnx_send_me_chan_config_req()
>>> rwnx_dbgfs_register()
rwnx_interface_add: wlan%d, 2, 10
interface add:98 8c bb 5b 51 30
aic_ipc_setting, enter
set_channel_access_req:edca[]= 0 138542 0 0
vif_idx: 0, retry_cnt: 1e, rts_en: 0, long_nav_en: 0, cfe_en: 0, rc_retry_cnt: 2:2:7, ccademod_th = 0 remove_1m2m = 1
>>> rwnx_open()
>>> rwnx_send_start()
>>> rwnx_send_coex_req()
rwnx_open rwnx_vif->drv_flags:4
>>> rwnx_send_add_if()
         Starting bluetooth.service - Bluetooth service...
[  OK  ] Started bluetooth.service - Bluetooth service.
[  OK  ] Reached target bluetooth.target - Bluetooth Support.
         Starting systemd-hostnamed.service - Hostname Service...
[  OK  ] Started systemd-hostnamed.service - Hostname Service.
aicbsp: aicbsp_set_subsys, subsys: AIC_BLUETOOTH, state to: 1
aicbsp: aicbsp_set_subsys, power state no need to change, current: 1
[  OK  ] Finished aic8800.service - AIC8800 Wi-Fi Bring-Up.
[  OK  ] Reached target network-pre.target - Preparation for Network.
         Starting NetworkManager.service - Network Manager...
         Starting networking.service - Raise network interfaces...
         Starting NetworkManager-dispatcher.service - Network Manager Script Dispatcher Service...
[  OK  ] Finished networking.service - Raise network interfaces.
[  OK  ] Started NetworkManager.service - Network Manager.
[  OK  ] Reached target network.target - Network.
[  OK  ] Reached target network-online.target - Network is Online.
         Starting dnsmasq.service - dnsmasq - A lightweight DHCP and caching DNS server...
         Starting lircd.service - Flexible IR remote input/output application support...
         Starting openvpn.service - OpenVPN service...
         Starting rc-local.service - /etc/rc.local Compatibility...
         Starting ssh.service - OpenBSD Secure Shell server...
         Starting systemd-user-sessions.service - Permit User Sessions...
[  OK  ] Finished openvpn.service - OpenVPN service.
[  OK  ] Started NetworkManager-dispatcher.service - Network Manager Script Dispatcher Service.
[  OK  ] Started rc-local.service - /etc/rc.local Compatibility.
[  OK  ] Finished systemd-user-sessions.service - Permit User Sessions.
         Starting gdm.service - GNOME Display Manager...
         Starting plymouth-quit-wait.service - Hold until boot process finishes up...
rwnx_virtual_interface_add: 10, p2p-dev-wlan0
rwnx_virtual_interface_add, ifname=p2p-dev-wlan0, wdev=0000000069a866a4, vif_idx=1
p2p dev addr=98 8c bb 5b 51 31
P2P interface started
>>> rwnx_cfg80211_scan()
>>> rwnx_send_scanu_req()
[  OK  ] Started ssh.service - OpenBSD Secure Shell server.
[  OK  ] Started dnsmasq.service - dnsmasq - A lightweight DHCP and caching DNS server.
[  OK  ] Reached target nss-lookup.target - Host and Network Name Lookups.
         Starting wg-quick@wg0.service - WireGuard via wg-quick(8) for wg0...
[  OK  ] Started gdm.service - GNOME Display Manager.
[  OK  ] Started lircd.service - Flexible IR remote input/output application support.
p104                                                                                                                                                                                                                                                                           p
104p104                                                                                                                                                                                                                                                                        p
104
orangepi4pro login: orangepi (automatic login)

  ___  ____  _   _  _     ____
 / _ \|  _ \(_) | || |   |  _ \ _ __ ___
| | | | |_) | | | || |_  | |_) | '__/ _ \
| |_| |  __/| | |__   _| |  __/| | | (_) |
 \___/|_|   |_|    |_|   |_|   |_|  \___/

Welcome to Orange Pi 1.1.0 Noble with Linux 6.18.58-sun60iw2
</pre>
