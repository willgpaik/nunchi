# nunchi

*Nunchi* (눈치) is Korean for sensing what is going on in a room without being told. This project tries to give a small camera that sense.

A thermal + RGB + radar fusion camera on ESP32-P4. The MLX90642 thermal image is upscaled and blended over the OV5647 RGB image and shown on the board's 7-inch LCD, with on-device detection using ESP-DL.

An HLK-LD2450 mmWave radar adds what the camera images lack: distance and speed of moving people. The two sensors fail in different cases. The radar loses people who sit still, which the thermal image still sees. The thermal image has no distance and cannot tell a person from a warm object like a heater, while the radar reports distance and motion. Combining them is the point of the project.

> Status: early setup. The firmware is a board bring-up app for now. It builds with ESP-IDF v5.5.4 but has not been run on hardware yet.

## Hardware

| Part | Spec |
|---|---|
| MCU board | ESP32-P4, dual-core RISC-V at 360 MHz (chip revision v1.x), 32 MB PSRAM, 32 MB flash |
| Display | 7-inch 1024x600 IPS touch LCD on the board (MIPI DSI) |
| Wireless | Wi-Fi 6 / BLE 5 through an ESP32-C6 co-processor on the board |
| Thermal sensor | MLX90642, 32x24, 2 to 16 Hz, I2C |
| RGB camera | OV5647, 5 MP, MIPI CSI (2-lane) |
| mmWave radar | HLK-LD2450, 24 GHz, up to 3 moving targets with X/Y and speed, UART 256000 baud, 5 V |

Details and pin notes: [docs/hardware.md](docs/hardware.md)

## Repository layout

```
.
├── CMakeLists.txt       ESP-IDF project
├── sdkconfig.defaults   committed config (target, chip revision, flash, PSRAM)
├── main/                application code
├── docs/
│   └── hardware.md
└── .gitignore
```

Planned, not created yet:

- `components/`: MLX90642 driver, LD2450 frame parser, thermal processing (interpolation, colormap, blending)
- `ml/`: training and quantization scripts for the detection model

## Build

Requires ESP-IDF v5.5.x (checked with v5.5.4).

```sh
. $IDF_PATH/export.sh
idf.py build
idf.py -p <PORT> flash monitor
```

- The target (`esp32p4`) comes from `sdkconfig.defaults`, so `idf.py set-target` is not needed.
- `sdkconfig` is generated on the first build and is not tracked. To change a setting for good, put it in `sdkconfig.defaults`, then delete `sdkconfig` and rebuild.
- `<PORT>` is usually `/dev/ttyACM0` or `/dev/ttyUSB0` on Linux.

On boot, check these lines in the monitor output:

```
boot: chip revision: v1.x
cpu_start: cpu freq: 360000000 Hz
bringup: flash 32 MB (build set to 32 MB)
bringup: psram 32 MB
```

## Roadmap

- [ ] 0. Board bring-up (chip revision, CPU clock, flash, PSRAM)
- [ ] 1. OV5647 frames through MIPI CSI and the ISP
- [ ] 2. Camera frames on the 7-inch LCD
- [ ] 3. MLX90642 frames over I2C
- [ ] 4. Upscale the thermal frame to the RGB frame size (bilinear first)
- [ ] 5. Align and blend thermal over RGB
- [ ] 6. LD2450 targets over UART
- [ ] 7. Draw radar targets on the image (map radar X/Y into camera and thermal coordinates)
- [ ] 8. On-device detection with ESP-DL
- [ ] 9. Fuse thermal, radar and detection into one decision per person
- [ ] 10. Stream to a browser over Wi-Fi (optional)

## License

MIT. See [LICENSE](LICENSE).

Third party code keeps its own license. Each one will be listed here as it is added.
