# Bill of Materials

This BOM describes the USB-powered reference clock that was compiled and
tested with firmware v12. Confirm the pin labels and electrical specifications
of the exact boards received; products sold under the same names can vary.

## Required parts

| Qty | Item | Tested specification | Selection notes |
|---:|---|---|---|
| 1 | ESP32-2432S028 CYD | ESP32-WROOM-32, 2.8-inch 320x240 ILI9341 TFT, XPT2046 resistive touch, onboard microSD and R21 light sensor | Obtain the common 28-pin CYD layout or compare every pin before wiring. |
| 1 | GP-02 GPS/GNSS module | 3.3 V operation, NMEA TX, PPS, and N/F standby control | A substitute must have compatible voltage levels and documented enable behavior. |
| 1 | microSD card | 8 GB or larger, FAT32 | The current data set is about 213 MB; use a reputable card. |
| 1 | USB data cable | Connector matching the CYD | Must carry data for programming, not power only. |
| 1 | Regulated 5 V USB supply | Suitable for ESP32, TFT backlight, GPS, and SD-card peaks | A marginal supply can cause resets or SD errors. |
| 5 | Insulated hookup wires | Fine stranded or solid wire suitable for the chosen connectors | GPS: 3.3 V, GND, TX, PPS, and N/F. |
| as needed | Headers/connectors | 2.54 mm or board-specific | Optional detachable connections make service easier. |
| as needed | Solder and heat-shrink | Electronics grade | Insulate exposed joints and provide strain relief. |

## Tools and consumables

- Fine-tip temperature-controlled soldering iron
- Wire cutters and strippers
- Multimeter with continuity/voltage measurement
- Small screwdrivers and enclosure hardware
- ESD-safe work surface recommended
- Computer running Arduino IDE 2.x or Arduino CLI
- microSD card reader

## Reference wiring

| GP-02 signal | CYD connection | ESP32 GPIO |
|---|---|---:|
| VCC | 3.3 V | - |
| GND | GND | - |
| TX / NMEA out | UART2 receive | 22 |
| PPS | Pulse input | 35 |
| N/F | Standby/enable control | 27 |

The onboard display, touch controller, SD slot, R21 sensor, and backlight need
no separately purchased wiring. Their GPIO assignments are documented in
[Hardware build](HARDWARE_BUILD.md).

## Optional mechanical parts

| Qty | Item | Notes |
|---:|---|---|
| 1 | Enclosure | Leave access to USB and microSD and expose R21 to room light. |
| 1 | GPS antenna mount or nonmetallic bracket | Keep the antenna oriented toward the sky and away from noisy converters. |
| 4 | Standoffs and screws | Match the CYD mounting holes; avoid contact with pads and traces. |
| 1 | Light pipe or recessed R21 opening | Useful if the sensor is behind a front panel. |

## Not included in the tested release

Battery cells, chargers, boost converters, solar panels, fuel gauges, BH1750 or
BME280 sensors, and alarm buzzers/speakers have been discussed but are not part
of v12. Do not infer wiring or safety requirements for them from this BOM.

