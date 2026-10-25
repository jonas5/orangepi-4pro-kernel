# AIC8800D80 SDIO Wi-Fi & Bluetooth Driver Handover & Architecture

**Target Device:** Orange Pi 4 Pro (`orangepi4pro` — `10.9.0.2` on WireGuard)  
**SoC:** Allwinner A733 (`sun60iw2`)  
**Kernel:** Mainline Linux `6.18.56-dirty` (aarch64)  
**Wi-Fi / Bluetooth Chipset:** AICSemi AIC8800D80 (SDIO Function 1 for Wi-Fi, UART `/dev/ttyS1` for Bluetooth)  
**Status:** **FULLY OPERATIONAL & CONNECTED** (Connected to Vodafone Ultra Hub 7 5GHz `No Signal` / `192.168.3.191`)  
**Date:** October 2026  

---

## 1. Executive Summary & Problem Resolved

The onboard AIC8800D80 wireless module had three distinct failure modes preventing connection:

1. **4-Way Handshake Dropping Message 2 (`4WAY_HANDSHAKE_TIMEOUT` / `reason=15`)**:
   - Despite valid credentials (`Blackpant3r` for `No Signal`, `eaks3103` for `HMD Fusion`), `wpa_supplicant` repeatedly timed out during the 4-way WPA2 handshake.
   - **Root Cause**: In `aicwf_sdio_aggr()` in `aicwf_sdio.c`, the driver constructed the 4-byte SDIO packet header containing the exact packet length and CRC-8 checksum (`sdio_header[0..3]`) required by the AIC8800D80 microcode. However, later in the aggregation function, generic legacy code executed:
     ```c
     start_ptr[0] = ((tx_priv->tail - start_ptr - 4) & 0xff);
     start_ptr[1] = (((tx_priv->tail - start_ptr - 4)>>8) & 0x0f);
     ```
     This overwrote `start_ptr[0..1]`, destroying the precalculated packet length and invalidating the CRC-8 byte (`sdio_header[3]`). The AIC8800D80 microcode discarded the corrupted EAPOL Message 2 frames as CRC errors, causing APs to never receive Message 2 and disconnect with reason 15.
   - **Fix**: NOP'd the destructive `start_ptr[0..1]` overwrites and bypassed `sw_txhdr->need_cfm = 1` for EAPOL frames (since SDIO does not send host status confirmations). Message 2 and Message 4 now transmit with bit-perfect CRC-8 headers. The 4-way handshake completes instantly in **<20 ms**.

2. **Kernel Panic on Boot (Wake Lock Null Pointer Dereference)**:
   - **Root Cause**: In `rwnx_wakelock.c`, the driver called `wakeup_source_register(usbdev->dev, ...)`. On SDIO platforms, `rwnx_hw->usbdev->dev` is NULL. This triggered a NULL pointer dereference at offset `0x18` during `aicwf_usb_wakeup_lock_init()`, causing an immediate kernel panic on boot.
   - **Fix**: Switched to `rwnx_hw->dev` (the SDIO function device pointer) with fallback validation.

3. **SDIO Bus Hang & Multi-Core Race Conditions**:
   - **Root Cause**: Loading `aic8800_bsp.ko` and `aic8800_fdrv.ko` simultaneously on multi-core boot resulted in `-34` (`-ERANGE`) register write errors and bus timeouts because the microcode was not yet initialized in the chip before the MAC probe started.
   - **Fix**: Driver modules are blacklisted from auto-load in `/etc/modprobe.d/blacklist-aic8800.conf` and `orangepiEnv.txt`. Sequenced bring-up is managed via `/usr/local/bin/aic8800-start.sh`.

---

## 2. Network Verification & Status

### Wi-Fi (`wlan0`)
- **Connected SSID:** `No Signal` (5 GHz, Channel 100, 270 Mbit/s)
- **Assigned IP:** `192.168.3.191/24` (Gateway: `192.168.3.1`)
- **Handshake Log:**
  ```text
  wlan0: Associated to a new BSS: BSSID=88:81:b9:94:aa:04
  wlan0: WPA: RX message 1 of 4-Way Handshake from 88:81:b9:94:aa:04 (ver=2)
  wlan0: WPA: Sending EAPOL-Key 2/4
  wlan0: WPA: RX message 3 of 4-Way Handshake from 88:81:b9:94:aa:04 (ver=2)
  wlan0: WPA: Sending EAPOL-Key 4/4
  wlan0: WPA: Key negotiation completed with 88:81:b9:94:aa:04 [PTK=CCMP GTK=CCMP]
  wlan0: State: GROUP_HANDSHAKE -> COMPLETED
  ```
- **Ping test over `wlan0`:**
  ```text
  4 packets transmitted, 4 received, 0% packet loss, time 3005ms
  rtt min/avg/max/mdev = 8.547/23.798/59.993/21.083 ms
  ```

### Bluetooth (`hci0`)
- **Device:** `AIC8820` on UART `/dev/ttyS1` at 1,500,000 baud
- **BD Address:** `A8:A0:92:F6:65:DC`
- **Helper Binary:** `/usr/local/bin/aic-bt-attach /dev/ttyS1`

---

## 3. Maintenance & Bring-Up Commands

### On the Orange Pi:
To bring up Wi-Fi and Bluetooth manually if rebooted:
```bash
sudo /usr/local/bin/aic8800-start.sh
```

To manage connections with NetworkManager:
```bash
# Scan APs:
nmcli dev wifi list

# Connect to Vodafone router:
nmcli connection up "No Signal"

# Connect to Phone Hotspot:
nmcli connection up "HMD Fusion"
```

---

## 4. Offline SD Card Updater Reference

If the SD card is ever updated from a separate host machine:
- Documentation: `/mnt/sd-boot/SD-CARD-UPDATE-GUIDE.md` (or `http://163.5.85.18/SD-CARD-UPDATE-GUIDE.md`)
- Command on host machine:
  ```bash
  curl -sSL http://163.5.85.18/update.sh | sudo bash -s -- /mnt/sdboot /mnt/sdroot
  ```
- The served distribution package (`http://163.5.85.18/modules_dirty/modules_6.18.56-dirty.tar.gz`) contains the patched driver binary and updated SHA256 checksums.
