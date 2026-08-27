# Hardware Build

## Read first

Disconnect USB power before wiring or soldering. Verify the pin labels and
voltage requirements on the exact CYD and GPS module in hand; inexpensive board
revisions are not always identical. ESP32 GPIO is not 5 V tolerant.

The tested reference clock is powered from USB. Battery backup, charging, and
power-path hardware are not part of this release.

## Parts and tools

Use the [Bill of Materials](BOM.md). You will also need a fine-tip soldering
iron, solder, small insulated hookup wire, wire cutters/strippers, a multimeter,
and a data-capable USB cable.

## Reference GPS wiring

Reference photos: [CYD rear board and wiring](../images/cyd-rear-board-and-gps-wiring.jpg)
and [GP-02 module wiring](../images/gp-02-gps-module-wiring.jpg).

The firmware receives NMEA data only; it does not transmit configuration data
to the receiver.

| GP-02 signal | CYD connection | ESP32 GPIO | Purpose |
|---|---|---:|---|
| VCC | 3.3 V | - | Receiver power |
| GND | GND | - | Common ground |
| TX | GPIO22 | 22 | NMEA serial into ESP32 UART2 RX |
| PPS | GPIO35 | 35 | One-pulse-per-second timing |
| N/F | GPIO27 | 27 | Receiver standby/enable control |

Do not connect a 5 V serial output to GPIO22. If using a receiver other than the
tested GP-02, verify its supply voltage, logic levels, PPS polarity, and the
meaning of its enable/standby pin.

## Assembly procedure

1. Leave the CYD disconnected from USB.
2. Identify 3.3 V, GND, GPIO22, GPIO35, and GPIO27 on the actual board.
3. Tin the wire ends and the intended pads or connector pins.
4. Connect GPS GND first, then 3.3 V, TX, PPS, and N/F according to the table.
5. Inspect every joint for solder bridges and loose strands.
6. Use a multimeter to confirm there is no short between 3.3 V and GND.
7. Insert the prepared FAT32 microSD card while power is off.
8. Connect USB and perform the functional tests before installing an enclosure.

## Built-in CYD connections used by firmware

### Display and backlight

| Function | GPIO |
|---|---:|
| TFT MISO | 12 |
| TFT MOSI | 13 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT reset | Board reset (`-1` in TFT_eSPI) |
| Backlight PWM | 21 |

### Resistive touchscreen

| Function | GPIO |
|---|---:|
| Touch SCK | 25 |
| Touch MISO | 39 |
| Touch MOSI | 32 |
| Touch CS | 33 |
| Touch IRQ | 36 |

### Onboard microSD

| Function | GPIO |
|---|---:|
| SD CS | 5 |
| SD MOSI | 23 |
| SD MISO | 19 |
| SD SCK | 18 |

### Ambient light

The onboard R21 photoresistor circuit is read through GPIO34. Do not cover it in
an opaque enclosure if AUTO Night Mode will be used. A small recessed opening or
light pipe should face the room while shielding the sensor from direct TFT
backlight. The tested hardware reads lower voltage in brighter light and higher
voltage in darkness.

## microSD preparation

1. Use a reliable card with at least 512 MB of usable capacity. A 1 GB or
   larger card is recommended for comfortable free space.
2. Create one FAT32 partition. A useful volume label is `GPSCLOCK`.
3. Create `timezone`, `places`, and `marine` folders at the card root.
4. Copy the seven generated database files using the layout in
   [Data build](DATA_BUILD.md).
5. Eject the card cleanly before inserting it into the unpowered CYD.

## Enclosure guidance

- Keep the GPS antenna facing outward with the clearest practical sky view.
- Keep the antenna away from grounded metal, the ESP32 antenna, and switching
  power converters.
- Leave access to USB, reset/program controls, and the microSD card.
- Provide ventilation and strain relief for the GPS wiring.
- Expose R21 to ambient room light while blocking direct display illumination.
- Do not allow the enclosure or fasteners to short exposed pads.

## Pre-enclosure functional test

1. Upload the verified firmware using [Firmware build](FIRMWARE_BUILD.md).
2. Confirm the display starts and the Options grid responds accurately.
3. Confirm each SD database reports ready in the 115200-baud Serial Monitor.
4. Move the GPS antenna outdoors or to a window and wait for a stable fix.
5. Confirm UTC, local time, timezone abbreviation, UTC offset, locality,
   Maidenhead grid, coordinates, satellite count, and altitude appear.
6. Confirm GPS enters its sleep/synchronized state after an accepted fix.
7. Change callsign and brightness, reboot, and confirm both settings persist.
8. Cover and uncover R21 long enough to verify AUTO Night Mode behavior.

