# Allwinner A733 (Orange Pi 4 Pro) - Complete Pin Configuration Report

## Pin Controller Architecture

| Controller | Base Address | Domain | Interrupts |
|---|---|---|---|
| `pio` | 0x02000000 | Main (PA-PK) | GIC_SPI 69,71,73,75,77,79,81,83,85,87 |
| `r_pio` | 0x07025000 | R_AON (PL-PN) | GIC_SPI 198,200 |

---

## 1. UART

### UART0 (Debug Console) - `serial0`
| Property | Vendor | Mainline |
|---|---|---|
| Pins | (empty - use PH8 via standard pinctrl) | PH8 |
| Function | uart0 | uart0 |
| Node | `&uart0` | `&uart0` |
| Status | okay | okay |

### UART1 (BT HCI)
| Property | Vendor | Mainline |
|---|---|---|
| Pins | PG6 (RX), PG7 (TX), PG8 (CTS), PG9 (RTS) | PG6-PG9 |
| Function | uart1 | uart1 |
| Node | `&uart1` | `&uart1` |
| Drive-strength | 10mA | - |
| Sleep | io_disabled | - |
| Status | okay | okay |

### UART2
| Property | Vendor | Mainline |
|---|---|---|
| Pins | PB0, PB1, PB2, PB3 | PB0, PB1 |
| Function | uart2 | uart2 |
| Node | `&uart2` | `&uart2` |
| Drive-strength | 10mA | - |
| Sleep | io_disabled | - |
| Status | disabled | okay |

### UART3
| Property | Vendor |
|---|---|
| Pins | PI1 (TX), PI12 (RX) |
| Function | uart3 |
| Node | `&uart3` |
| Drive-strength | 10mA |
| Sleep | io_disabled (PI11, PI12) |
| Status | disabled |

### UART5
| Property | Vendor |
|---|---|
| Pins | (empty - internal loopback) |
| Function | uart5 |
| Node | `&uart5` |
| Drive-strength | 10mA |
| Sleep | io_disabled |
| Status | disabled |

### UART6
| Property | Vendor |
|---|---|
| Pins | PI6 (RX), PI7 (TX) |
| Function | uart6 |
| Node | `&uart6` |
| Drive-strength | 10mA |
| Sleep | io_disabled |
| Status | okay |

### UART7 (S_UART0) - R_AON Domain
| Property | Vendor |
|---|---|
| Pins | PL6 (RX), PL7 (TX) |
| Function | s_uart0 |
| Node | `&uart7` |
| Drive-strength | 10mA |
| Sleep | io_disabled |
| Status | disabled |

### UART8 (S_UART1) - R_AON Domain
| Property | Vendor |
|---|---|
| Pins | PL8 (RX), PL9 (TX) |
| Function | s_uart1 |
| Node | `&uart8` |
| Drive-strength | 10mA |
| Sleep | io_disabled |
| Status | disabled |

---

## 2. I2C (TWI)

### TWI0
| Property | Vendor |
|---|---|
| Pins | PB2 (SCL), PB3 (SDA) |
| Function | twi0 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Supply | reg_cldo3 |
| Status | disabled |

### TWI1
| Property | Vendor |
|---|---|
| Pins | PB4 (SCL), PB5 (SDA) |
| Function | twi1 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Status | disabled |

### TWI2
| Property | Vendor |
|---|---|
| Pins | PE1 (SCL), PE2 (SDA) |
| Function | twi2 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Supply | reg_bldo1 |
| Status | disabled |

### TWI3
| Property | Vendor |
|---|---|
| Pins | PE3 (SCL), PE4 (SDA) |
| Function | twi3 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Supply | reg_bldo1 |
| Status | disabled |

### TWI5 (Touchscreen)
| Property | Vendor | Mainline |
|---|---|---|
| Pins | PJ26 (SCL), PJ27 (SDA) | - |
| Function | twi5 | - |
| Drive-strength | 10mA | - |
| Bias | pull-up | - |
| Sleep | gpio_in | - |
| Clock | 400kHz | - |
| Supply | reg_dc1sw2 | - |
| Status | okay | - |
| Devices | Goodix GT9271 touchscreen @0x14 | - |

### TWI7 (ES8388 Audio Codec)
| Property | Vendor |
|---|---|
| Pins | PJ22 (SCL), PJ23 (SDA) |
| Function | twi7 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Supply | reg_bldo2 |
| Status | okay |
| Devices | ES8388 audio codec @0x10 |

### TWI8
| Property | Vendor |
|---|---|
| Pins | PB9 (SCL), PB10 (SDA) |
| Function | twi8 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Status | okay |

### TWI9 (PMIC I2C)
| Property | Vendor | Mainline |
|---|---|---|
| Pins | PE14 (SCL), PE15 (SDA) | - |
| Function | twi9 | - |
| Drive-strength | 40mA | - |
| Bias | pull-up | - |
| Sleep | gpio_in | - |
| Clock | 100kHz | - |
| Supply | reg_bldo1 | - |
| Status | okay | - |

### TWI11 (Camera)
| Property | Vendor |
|---|---|
| Pins | PI4 (SCL), PI5 (SDA) |
| Function | twi11 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Supply | reg_dc1sw2 |
| Status | disabled |

### S_TWI0 (R_AON Domain) - PMIC Bus
| Property | Vendor | Mainline |
|---|---|---|
| Pins | PL0 (SCL), PL1 (SDA) | PL0, PL1 |
| Function | s_twi0 | r_i2c0 |
| Drive-strength | 10mA | 40mA |
| Bias | pull-up | - |
| Sleep | gpio_in | - |
| Clock | 400kHz | - |
| Status | okay | okay |
| Devices | AXP515 @0x34, AXP8191 @0x36 | AXP515 @0x34, AXP8191 @0x36 |

### S_TWI1 (R_AON Domain)
| Property | Vendor |
|---|---|
| Pins | PL12 (SCL), PL13 (SDA) |
| Function | s_twi1 |
| Drive-strength | 30mA |
| Sleep | gpio_in |
| Clock | 400kHz |
| Status | disabled |

### S_TWI2 (R_AON Domain)
| Property | Vendor |
|---|---|
| Pins | PL10 (SCL), PL11 (SDA) |
| Function | s_twi2 |
| Drive-strength | 10mA |
| Bias | pull-up |
| Sleep | gpio_in |
| Clock | 400kHz |
| Status | okay |

---

## 3. SPI

### SPI0 (NOR Flash)
| Property | Vendor | Mainline |
|---|---|---|
| Pins | PC12 (CLK), PC2 (MOSI), PC4 (MISO), PC3 (CS) | PC pins |
| Function | spi0 | spi0 |
| Drive-strength | 1 (allwinner,drive) | - |
| CS | 1 chip select | - |
| Bus-mode | NOR | - |
| CS-mode | soft | - |
| Clock | 50MHz | - |
| Status | okay | okay |

### SPI3
| Property | Vendor |
|---|---|
| Pins | PE1 (CLK), PE2 (MOSI), PE3 (MISO), PE4 (CS0), PE12 (CS1) |
| Function | spi3 |
| Drive-strength | - |
| CS | 2 chip selects |
| Bus-mode | master |
| CS-mode | auto |
| Clock | 100MHz |
| Status | disabled |

---

## 4. MMC/SDIO

### SDC0 (SD Card) - `mmc0`
| Property | Vendor | Mainline |
|---|---|---|
| Pins (active) | PF0, PF1, PF3, PF4, PF5 | mmc0_pins |
| Pins (sleep) | PF0-PF5 (gpio_in, 3.3V) | - |
| Pins (1v8) | PF0-PF5 (1.8V) | - |
| Pins (jtag) | PF0,PF1,PF3,PF5 (jtag), PF2,PF4 (uart0) | - |
| Function | sdc0 | mmc0 |
| Drive-strength | 10mA (active) | - |
| Bias | pull-up | - |
| Power-source | 3300mV (default), 1800mV (1v8) | - |
| CD GPIO | PF6 (active-low, pull-up) | PF6 (active-low) |
| WP GPIO | - | PF2 (active-high) |
| Bus-width | 4 | 4 |
| Supply | reg_dc1sw2, reg_cldo5 (1.8v sw) | reg_dldo1 |
| Max-freq | 150MHz | - |
| UHS modes | sdr50, ddr50, sdr104 | - |
| Status | okay | okay |

### SDC1 (WiFi SDIO) - `mmc1`
| Property | Vendor | Mainline |
|---|---|---|
| Pins (active) | PG1-PG5 (sdc1), PG0 (cmd) | mmc1_pins |
| Pins (sleep) | PG0-PG5 (gpio_in) | - |
| Function | sdc1 | mmc1 |
| Drive-strength | 10mA (dat), 20mA (cmd) | - |
| Bias | pull-up | - |
| Bus-width | 4 | 4 |
| UHS modes | sdr25, sdr50, ddr50, sdr104 | - |
| Cap-sdio-irq | yes | yes |
| Keep-power | yes | yes |
| Max-freq | 208MHz | - |
| Status | okay | okay |

### SDC2 (eMMC) - `mmc2`
| Property | Vendor | Mainline |
|---|---|---|
| Pins (active) | PC6, PC8-PC11, PC13-PC16 | mmc2_pins |
| Pins (sleep) | PC0-PC16 (gpio_in) | - |
| Pins (cmd) | PC0 (pull-down), PC1, PC5 (pull-up) | - |
| Function | sdc2 | mmc2 |
| Drive-strength | 20mA (dat), 40mA (cmd/clk) | - |
| Bias | pull-up (dat/cmd), pull-down (clk) | - |
| Bus-width | 8 | 8 |
| Capabilities | hs200, hs400, ddr-1v8 | hs200, ddr-1v8 |
| Max-freq | 200MHz | - |
| Non-removable | yes | yes |
| Status | okay | okay |

### SDC3
| Property | Vendor DTSI |
|---|---|
| Pins (active) | PC6, PC8-PC11, PC13-PC16 (sdc3) |
| Pins (sleep) | PC0-PC16 (gpio_in) |
| Function | sdc3 |
| Drive-strength | 20mA (dat), 40mA (cmd/clk) |
| Bias | pull-up (dat/cmd), pull-down (clk) |
| Bus-width | 8 |
| Non-removable | yes |
| Status | disabled |

---

## 5. Ethernet (GMAC)

### GMAC0 - `ethernet0`
| Property | Vendor | Mainline |
|---|---|---|
| Pins (active) | PH0-PH15 (rgmii0) | rgmii0_pins |
| Pins (sleep) | PH0-PH15 (io_disabled) | - |
| Function | rgmii0 | rgmii0 |
| Drive-strength | 1 (allwinner,drive) | - |
| Bias | pull-up | - |
| PHY-mode | rgmii | rgmii |
| TX-delay | 12 | 12 |
| RX-delay | 10 | 10 |
| Supply | reg_dc1sw2, reg_dc1sw1 | reg_gmac_3v3 |
| Reset GPIO | PH16 (active-low, commented out) | - |
| Status | okay | okay |

---

## 6. USB

### USB0 (OTG)
| Property | Vendor | Mainline |
|---|---|---|
| VBUS GPIO | PL2 (commented out) | PH16 (PH bank, pin 16) |
| Type | OTG | OTG |
| Detect | vbus/id (axp_ctrl) | - |
| Status | okay | okay |

### USB1
| Property | Mainline |
|---|---|
| VBUS GPIO | PB7 (active-low) |
| Always-on | yes |
| Status | okay |

### USB2 (XHCI2)
| Property | Vendor |
|---|---|
| PHY | u2phy + combo0_usb |
| Dr-mode | host |
| Status | okay |

---

## 7. Display

### DSI0
| Property | Vendor |
|---|---|
| Pins | PD0-PD9 (10 pins, 4-lane) |
| Function | dsi0 |
| Drive-strength | 30mA |
| Bias | disable |
| Sleep | io_disabled |
| Status | disabled |

### DSI1
| Property | Vendor |
|---|---|
| Pins | PD10-PD19 (10 pins, 4-lane) |
| Function | dsi1 |
| Drive-strength | 30mA |
| Bias | disable |
| Sleep | io_disabled |
| Status | disabled |
| Panel | allwinner,virtual-panel |

### HDMI0
| Property | Vendor |
|---|---|
| Compatible | allwinner,sunxi-hdmi |
| DDC | index 20 |
| CEC | enabled |
| HDCP1x/2x | enabled |
| Power | dcdc2, cldo2 |
| Status | okay |

### LVDS0
| Property | Vendor DTSI |
|---|---|
| Pins | PD0-PD9 |
| Function | lvds0 |
| Drive-strength | 30mA |

### LVDS1
| Property | Vendor DTSI |
|---|---|
| Pins | PD10-PD19 |
| Function | lvds1 |
| Drive-strength | 30mA |

### LVDS2
| Property | Vendor DTSI |
|---|---|
| Pins | PJ0-PJ9 |
| Function | lvds2 |
| Drive-strength | 30mA |

### LVDS3
| Property | Vendor DTSI |
|---|---|
| Pins | PJ10-PJ19 |
| Function | lvds3 |
| Drive-strength | 30mA |

### RGB0 (24-bit)
| Property | Vendor DTSI |
|---|---|
| Pins | PG0,PG1,PG2,PG3,PG4,PG5,PD0-PD21 (28 pins) |
| Function | lcd0 |
| Drive-strength | 10mA |

### RGB1 (24-bit)
| Property | Vendor DTSI |
|---|---|
| Pins | PJ0-PJ27 (28 pins) |
| Function | lcd1 |
| Drive-strength | 10mA |

### EDP0
| Property | Vendor DTSI |
|---|---|
| Compatible | allwinner,drm-edp |
| PHY | combo0_dp, aux_hpd_phy |
| PCLK limit | 200MHz |
| Status | disabled |

---

## 8. Audio (I2S/PCM)

### I2S0 (AC101 Codec)
| Property | Vendor |
|---|---|
| Pins | PB4 (mclk), PB5 (bclk), PB6 (lrck), PB7 (dout0), PB8 (din0) |
| Functions | i2s0_mclk, i2s0_bclk, i2s0_lrck, i2s0_dout0, i2s0_din0 |
| Sleep | io_disabled (PB4-PB8) |
| Drive-strength | 1 (allwinner,drive) |
| Bias | disable (mclk/bclk/lrck), pull-down (dout/din) |
| Format | I2S |
| Slot-num | 4 |
| Slot-width | 16 |
| Codec | AC101 @0x1a (on TWI5 via TWI7) |
| Status | okay |

### I2S1
| Property | Vendor |
|---|---|
| Pins | PG10 (mclk), PG11 (bclk), PG12 (lrck), PG13 (dout0), PG14 (din0) |
| Functions | i2s1_mclk, i2s1_bclk, i2s1_lrck, i2s1_dout0, i2s1_din0 |
| Sleep | io_disabled (PG10-PG14) |
| Drive-strength | 1 (allwinner,drive) |
| Format | DSP_A |
| Status | disabled |

### I2S2
| Property | Vendor |
|---|---|
| Pins | PH3 (bclk), PH4 (lrck), PH5 (dout0), PH6 (din0) |
| Functions | i2s2_bclk, i2s2_lrck, i2s2_dout0, i2s2_din0 |
| Sleep | io_disabled (PH3-PH6) |
| Drive-strength | 1 (allwinner,drive) |
| Format | I2S |
| Status | disabled |

### I2S3 (HDMI Audio)
| Property | Vendor |
|---|---|
| Pins | PE7 (bclk), PE8 (lrck), PE9 (mclk), PE10 (din3), PE11 (dout2) |
| Functions | i2s3_bclk, i2s3_lrck, i2s3_mclk, i2s3_din3, i2s3_dout2 |
| Sleep | io_disabled (PE7-PE11) |
| Drive-strength | 1 (allwinner,drive) |
| DAI-type | hdmi |
| Format | I2S |
| Codec | hdmi_codec |
| Status | okay |

### I2S4 (ES8388)
| Property | Vendor |
|---|---|
| Pins | PK1 (mclk), PK0 (bclk), PK2 (lrck), PK4 (dout0), PK3 (din0) |
| Functions | i2s4_mclk, i2s4_bclk, i2s4_lrck, i2s4_dout0, i2s4_din0 |
| Sleep | io_disabled (PK0-PK4) |
| Drive-strength | 20mA |
| Format | I2S |
| Codec | ES8388 @0x10 (on TWI7) |
| Status | okay |

### OWA (One Wire Audio)
| Property | Vendor |
|---|---|
| Pins | PI10 |
| Function | owa0 |
| Sleep | io_disabled |
| Drive-strength | 1 (allwinner,drive) |
| Status | disabled |

---

## 9. PMIC

### AXP515 (Primary PMIC)
| Property | Value |
|---|---|
| Bus | S_TWI0 (PL0/PL1) |
| Address | 0x34 |
| Compatible | x-powers,axp515 |
| Interrupts | NMI |
| Drive-vbus | enabled |
| Regulators | dldo1-4 (1.8V fixed), eldo1-2 (1.8V fixed), drivevbus |
| Status | okay |

### AXP8191 (Secondary PMIC)
| Property | Value |
|---|---|
| Bus | S_TWI0 (PL0/PL1) |
| Address | 0x36 |
| Compatible | x-powers,axp8191 |
| Interrupts | NMI |
| Reset | pmu_reset |
| Drive-vbus | enabled |
| Regulators | dcdc1-9, rtcldo, aldo1-6, bldo1-5, cldo1-5, dldo1-6, eldo1-6, dc1sw1-2 |
| Status | okay |

---

## 10. Touchscreen

### Goodix GT9271
| Property | Vendor | Mainline |
|---|---|---|
| Bus | TWI5 | I2C9 |
| Address | 0x14 | 0x14 |
| IRQ GPIO | PJ25 (active-high) | PJ25 |
| Reset GPIO | PG10 (active-high) | PG10 |
| Size | 1280x800 | 1280x800 |
| Inverted-x | yes | yes |
| Swapped-x-y | yes | yes |

---

## 11. GPIO / LED

### Green LED
| Property | Mainline |
|---|---|
| GPIO | PI8 (active-high) |
| Default-state | on |
| Compatible | gpio-leds |

### WiFi Enable
| Property | Vendor | Mainline |
|---|---|---|
| GPIO | PM1 (r_pio, active-high, commented) | PI9 (active-low) |
| Function | wifi_regon | wifi-enable-pin |
| Drive-strength | - | 10mA |
| Output | - | low |

### WiFi Host Wake
| Property | Vendor |
|---|---|
| GPIO | PM0 (r_pio, active-high) |
| Function | wlan_hostwake |

### Bluetooth Reset
| Property | Vendor |
|---|---|
| GPIO | PM2 (r_pio, active-low) |
| Function | bt_rst_n |

### Bluetooth Wake
| Property | Vendor |
|---|---|
| GPIO | PM3 (r_pio, active-high) |
| Function | bt_wake |

### Bluetooth Host Wake
| Property | Vendor |
|---|---|
| GPIO | PM4 (r_pio, active-high) |
| Function | bt_hostwake |

### Headphone Detect
| Property | Vendor |
|---|---|
| GPIO | PB2 (active-high, TWI SDA sharing) |
| Function | hp-det-gpio |

### Headphone Plug
| Property | Vendor |
|---|---|
| GPIO | PL11 (r_pio, active-high) |
| Function | hp-plug-gpio |

### LED Controller (WS2812)
| Property | Vendor |
|---|---|
| Pins | PE4 |
| Function | ledc |
| Drive-strength | 10mA |
| LED count | 34 |
| Output mode | GRB |
| Status | disabled |

### Flash LED
| Property | Vendor |
|---|---|
| Enable GPIO | PL7 (r_pio, active-low) |
| Mode GPIO | PH14 (active-low) |
| Status | disabled |

### Camera Sensor GPIOs
| Sensor | Reset | PWDN | Active |
|---|---|---|---|
| imx219 (sensor0) | - | PD8 (active-high) | okay |
| ov13850 (sensor1) | PE13 (active-low) | PE6 (active-high) | okay |
| imx219_2 (sensor2) | - | PE3 (active-high) | okay |

---

## 12. PCIe

### PCIE RC
| Property | Vendor | Mainline |
|---|---|---|
| Reset GPIO | PD22 (active-high) | PH11 (active-low) |
| Wake GPIO | PD21 (active-high) | - |
| PHY | combo1_pcie | serdes0_phy |
| Lanes | 1 | - |
| Max-link-speed | 3 (Gen3) | - |
| Supply | pcie1v8: reg_bldo1, pcie3v3: reg_dcdc1 | - |
| Power-domain | SUN60IW2_PCK_PCIE | - |
| Status | okay | okay |

---

## 13. PWM

### PWM0_1 (Active)
| Property | Vendor |
|---|---|
| Pin | PK20 |
| Function | pwm0_1 |
| Drive-strength | 10mA |
| Sleep | gpio_in, pull-down |
| Status | okay |

### PWM0_0 through PWM0_7
| Pin | Function | Status |
|---|---|---|
| PD0 | pwm0_0 | disabled |
| PK20 | pwm0_1 | okay |
| PD2 | pwm0_2 | disabled |
| PD3 | pwm0_3 | disabled |
| PD4 | pwm0_4 | disabled |
| PD5 | pwm0_5 | disabled |
| PD6 | pwm0_6 | disabled |
| PD7 | pwm0_7 | disabled |

### PWM1 Channels
| Pin | Function | Status |
|---|---|---|
| PG2 | pwm1_3 | disabled |
| PI15 | pwm1_6 | disabled |
| PJ26 | pwm1_8 | disabled |
| PK5 | pwm1_9 | disabled |

### S_PWM0 (R_AON Domain)
| Pin | Function | Status |
|---|---|---|
| PL4 | s_pwm0_2 | disabled |
| PL9 | s_pwm0_7 | disabled |

---

## 14. WiFi / Bluetooth

### AP6275S (BCM43752)
| Property | Vendor | Mainline |
|---|---|---|
| WLAN SDIO | SDC1 (PG0-PG5) | mmc1 (PG pins) |
| WLAN Power | axp8191-bldo5 (1.8V), axp8191-cldo1 (1.8V) | reg_dldo2 |
| WLAN Reg-on | PM1 (r_pio, active-high) | PI9 (via wifi_pwrseq) |
| WLAN Host-wake | PM0 (r_pio, active-high) | PI10 (interrupt) |
| BT UART | UART1 (PG6-PG9) | UART1 (PG6-PG9) |
| BT Power | axp8191-bldo5 (1.8V), axp8191-cldo1 (1.8V) | - |
| BT Reset | PM2 (r_pio, active-low) | - |
| BT Wake | PM3 (r_pio, active-high) | - |
| BT Host-wake | PM4 (r_pio, active-high) | - |
| Bus-num | 1 | - |

---

## 15. Other Peripherals

### IR Receiver
| Property | Vendor |
|---|---|
| S_IRRX Pin | PL4 (r_pio) |
| Function | s_ir_rx |
| Sleep | gpio_in |
| Status | disabled |

### LRADC (Key Input)
| Property | Vendor |
|---|---|
| Keys | 5 |
| Key mapping | 210->115, 410->114, 590->139, 750->28, 880->172 |
| Status | okay |

### GPADC
| Property | Vendor |
|---|---|
| Channels | 8 |
| Mode | 0x2 |
| Status | okay |

### MIPI CSI
| Property | Vendor |
|---|---|
| MIPIC pins | PK20-PK25 (mcsic) |
| Status | okay (mipi0) |

### UFS
| Property | Vendor |
|---|---|
| VCC supply | reg_dldo6 |
| VCCQ supply | reg_dcdc8 |
| VCCQ2 supply | reg_dcdc8 |
| Status | okay |

### Crypto Engine
| Property | Mainline |
|---|---|
| Status | okay |

---

## Pin Bank Summary

### Main PIO Banks (pio controller @ 0x02000000)

| Bank | Pins | Voltage Domain | Key Functions |
|---|---|---|---|
| PA | PA0-PA19 | 3.3V/1.8V | - |
| PB | PB0-PB17 | 3.3V | UART2, TWI0/1/8, I2S0, USB VBUS, HS-PHY |
| PC | PC0-PC16 | 3.3V | SDC0/1/2/3 (eMMC/SD), SPI0 |
| PD | PD0-PD21 | 3.3V | DSI0, DSI1, LVDS0/1, RGB0, PWM0 |
| PE | PE0-PE15 | 3.3V | CSI, TWI2/3/9, I2S3, SPI3, LEDC, TWI9 |
| PF | PF0-PF6 | 3.3V | SDC0 (SD card), JTAG, UART0 mux |
| PG | PG0-PG14 | 3.3V | SDC1 (WiFi SDIO), UART1, I2S1, GMAC0 mux |
| PH | PH0-PH16 | 3.3V | GMAC0 (RGMII), I2S2, USB, PCIe |
| PI | PI0-PI15 | 3.3V | UART3/6, TWI11, OWA, PWM1, LED |
| PJ | PJ0-PJ27 | 3.3V | LVDS2/3, RGB1, TWI5/7, PWM1_8 |
| PK | PK0-PK25 | 3.3V | I2S4, MIPI CSI, PWM, MIPI |

### R_AON PIO Banks (r_pio controller @ 0x07025000)

| Bank | Pins | Voltage Domain | Key Functions |
|---|---|---|---|
| PL | PL0-PL13 | 1.8V | S_TWI0/1/2, S_UART0/1, IR RX, S_PWM, USB, HP detect |
| PM | PM0-PM4 | 1.8V | WiFi/BT control (reg-on, host-wake, reset, wake) |
| PN | PN0-PN15 | 1.8V | - |

---

## Cross-Reference: Vendor vs Mainline

| Feature | Vendor DTS | Mainline DTS |
|---|---|---|
| UART0 | uart0_pins_active (empty) | uart0_ph8_pins |
| UART1 | uart1_pins_active (PG6-9) | uart1_pg6_pins + rts_cts |
| UART2 | uart2_pins_active (PB0-3) | uart2_pins (PB0-1) |
| I2C0 (PMIC) | s_twi0 (PL0/PL1) | r_i2c0 (PL0/PL1) |
| I2C9 (Touch) | twi5 (PJ26/PJ27) | i2c9 |
| SPI0 | spi0_pins (PC2-4,12,3) | spi0_pc_pins |
| SD Card | sdc0_pins_a (PF0-5) | mmc0_pins + mmc0_cd_pin |
| WiFi SDIO | sdc1_pins_a (PG0-5) | mmc1_pins |
| eMMC | sdc2_pins_a (PC6-16) | mmc2_pins |
| Ethernet | gmac0_pins_default (PH0-15) | rgmii0_pins |
| PCIe | reset=PD22, wake=PD21 | reset=PH11 |
| WiFi Pwrseq | PM1 (commented) | PI9 |
| USB0 VBUS | PL2 (commented) | PH16 |
| USB1 VBUS | - | PB7 |
