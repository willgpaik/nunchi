# Hardware

## Parts

| Part | Spec |
|---|---|
| ESP32-P4 board with 7-inch touch LCD (Waveshare) | Dual-core RISC-V, 360 MHz max (chip revision v1.x). 32 MB PSRAM in the chip package, 32 MB NOR flash. 7-inch 1024x600 IPS LCD over MIPI DSI with capacitive touch. 2-lane MIPI CSI camera connector. ESP32-C6 co-processor for Wi-Fi 6 / BLE 5 over SDIO. Audio codec with two mics, microSD slot, USB-to-UART port, I2C/UART headers. |
| MLX90642 | 32x24 IR array, I2C, 3.3 V supply, refresh rate 2 to 16 Hz. Two FOV versions exist (45x35 and 110x75 degrees). |
| OV5647 camera | 5 MP sensor, raw Bayer output over MIPI CSI. FOV depends on the lens. |
| HLK-LD2450 radar (V1.1) | 24 GHz FMCW, tracks up to 3 moving targets with X/Y position and speed. About 6 m range, azimuth ±60°, pitch ±35°. 5 V supply (more than 200 mA), 3.3 V UART I/O, 256000 baud 8N1, 10 reports per second. |

Still to confirm on the real parts:

- [ ] MLX90642 FOV version
- [ ] OV5647 lens FOV
- [ ] I2C header: on GPIO7/8? Pin order and VCC voltage
- [ ] UART header: on GPIO37/38 (UART0)? If yes, keep the radar off it
- [ ] Expansion header: GPIO numbers and which pins carry 5 V

The FOVs matter when matching the thermal image, the RGB image and the radar coordinates to each other.

## Chip revision

ESP32-P4 has two silicon generations that need different builds:

| Revision | ESP-IDF setting | Max CPU clock |
|---|---|---|
| v1.x (this board) | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` | 360 MHz |
| v3.x | leave it off (default). Needs ESP-IDF v5.5.3+ or v6.0+ | 400 MHz |

The chip on this board has no `X` at the end of its part number, which is how Espressif marks v1.x parts. `sdkconfig.defaults` is set for v1.x.

A build for the wrong revision fails at flash time with an error like:

```
bootloader/bootloader.bin requires chip revision in range [v3.1 - v3.99] (this chip is revision v1.3)
```

ESP-IDF has an `ESP_FORCE_400MHZ_ON_REV_LESS_V3` option. Its help text says to use it only on chips that Espressif has confirmed for 400 MHz, so it stays off here.

## Camera path

- OV5647 goes through MIPI CSI into the P4 ISP, which converts raw Bayer data to RGB/YUV.
- The P4 ISP accepts at most 1920x1080 input, so the OV5647 full 2592x1944 mode cannot go through it. 1080p and smaller modes are the ones to use.
- Driver stack: `esp_cam_sensor` (sensor driver) and `esp_video` from Espressif's esp-video-components.

## Display

Waveshare publishes a BSP for this board on the ESP Component Registry (`waveshare/esp32_p4_wifi6_touch_lcd_7b`). It covers the LCD, touch and audio. To be checked when we get to step 2.

## Board headers

The I2C header, the UART header and the 2x12 GPIO header on this board are **PH2.0 (2.0 mm)** connectors, not 2.54 mm pin headers and not Qwiic/STEMMA QT. Waveshare's pages do not list their pin order, so check the silkscreen and measure 3.3 V / 5 V / GND with a multimeter before plugging anything in.

## MLX90642 wiring

Plan: the PH2.0 I2C header. Waveshare's wiki gives the I2C pins as GPIO7 (SDA) and GPIO8 (SCL), and the schematic text shows the header on that bus with 4.7 kΩ pull-ups already on the board. To be confirmed on the board.

The same bus also carries the touch controller and the ES8311 audio codec. Addresses do not overlap:

| Device | 7-bit address |
|---|---|
| GT911 touch | 0x5D (or 0x14) |
| ES8311 codec | 0x18 (written as 0x30 in the esp_codec_dev driver) |
| MLX90642 | 0x66 |

Waveshare's `03_i2c_tools` example can scan the bus before and after connecting the sensor.

- MLX90642 needs 3.3 V. Measure the header's VCC pin before connecting.
- Trust the pin labels printed on the sensor board, not the cable colors.
- After power up, the sensor needs up to about 3 minutes to reach its specified accuracy.

## LD2450 wiring

Plan: two free pins on the 12-pin expansion header, with **UART2** routed to them through the GPIO matrix. The ESP32-P4 has 5 HP UARTs, and any GPIO can carry a UART signal.

**Not the PH2.0 UART header.** The schematic text shows GPIO37/GPIO38 next to the USB-to-UART chip (CH343P), and the UART header is most likely on the same pins. GPIO37/38 are UART0, the default console. A radar there would receive boot logs, its TX would fight the USB chip's TX on GPIO38, and GPIO37/38 are strapping pins. To be confirmed on the board.

How to check: flash any ESP-IDF app, connect a USB-to-TTL adapter to the UART header (GND, and the header's TX to the adapter's RX only), and open it at 115200 baud. If the boot log shows up, the header is the console UART.

Candidates seen in the schematic text for the expansion headers: GPIO2 to GPIO5, GPIO28 to GPIO31, GPIO49 to GPIO52 (GPIO34 is also there but is a strapping pin). Read the silkscreen for the real pin order.

Pins to avoid when picking the two expansion-header pins:

| Pins | Used by |
|---|---|
| 7, 8 | I2C (touch, codec, MLX90642) |
| 9 to 13 | I2S audio |
| 24, 25 | USB-JTAG |
| 26, 27 | RS485 (UART1) |
| 32, 33 | LCD backlight and reset |
| 34 to 38 | strapping pins (37/38 are also UART0) |
| 39 to 44 | microSD |
| 53 | speaker amplifier enable |

- Power the radar from 5 V. If range looks short, use a separate 5 V supply with a shared GND.
- The radar's own header is 2.0 mm pitch.
- Check the radar firmware version with the HLKRadarTool app first (V2.02.23090617 or newer is the usual reference).

### Known radar limitations

- It tracks moving targets. A person who sits completely still may drop out, which is where the thermal image helps.
- Fans, curtains and large reflective surfaces cause ghost targets. A ghost target with no heat in the thermal image at that spot is a candidate for rejection.
- 24 GHz passes through drywall, so it can report people in the next room.

## Coordinate frames

The radar reports X/Y on a plane in front of it (X left/right, Y forward). The camera and the thermal sensor report pixels. To draw radar targets on the image, all three sensors need a known mounting position and orientation relative to each other. Mounting them on one bracket, facing the same direction and as close together as possible, keeps the math simple.

## Wi-Fi

The ESP32-P4 has no radio. Wi-Fi and BLE go through the ESP32-C6 over SDIO. The C6 runs its own firmware and is not used for application processing.

## References

- Board page: https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-7b.htm
- Board BSP: https://components.espressif.com/components/waveshare/esp32_p4_wifi6_touch_lcd_7b
- Board wiki: https://www.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-7B
- Board schematic: https://files.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-7B/ESP32-P4-WIFI6-Touch-LCD-7B.pdf
- Board examples (including 03_i2c_tools): https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B
- ESP-IDF GPIO notes for ESP32-P4 (strapping pins, USB-JTAG): https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html
- ESP32-P4 chip revision v3.x user guide: https://documentation.espressif.com/esp32-p4-chip-revision-v3.x_user_guide_en.html
- MLX90642 datasheet: https://media.melexis.com/-/media/files/documents/datasheets/mlx90642-datasheet-melexis.pdf
- Melexis MLX90642 reference driver (Apache-2.0): https://github.com/melexis/mlx90642-library
- esp_cam_sensor: https://components.espressif.com/components/espressif/esp_cam_sensor
- LD2450 serial protocol v1.03: https://make.net.za/wp-content/datasheets/HLK%20LD2450%20Serial%20Communication%20Protocol%20v1.03.pdf
- LD2450 instruction manual: https://www.tinytronics.nl/product_files/006000_HLK-LD2450-Instruction-Manual.pdf
- ESP-DL: https://github.com/espressif/esp-dl
