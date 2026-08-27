# Firmware and Source Build

## Tested environment

| Component | Version used for v12 verification |
|---|---:|
| Arduino IDE | 2.x |
| Arduino CLI | 1.5.1 |
| Espressif ESP32 Arduino Core | 3.3.11 |
| TFT_eSPI | 2.5.43 |
| XPT2046_Touchscreen | 1.4 |
| TinyGPSPlus | 1.0.3 |

Later compatible versions may work, but the GPIO21 backlight implementation
uses the ESP32 Arduino Core 3.x LEDC API.

## Install the Arduino environment

1. Install Arduino IDE 2.x.
2. In Boards Manager, install **esp32 by Espressif Systems**, preferably the
   tested 3.3.11 version for the first build.
3. In Library Manager, install:
   - **TFT_eSPI** by Bodmer
   - **XPT2046_Touchscreen** by Paul Stoffregen
   - **TinyGPSPlus** by Mikal Hart
4. Select **ESP32 Dev Module**.

`SPI`, `SD`, `Preferences`, `HardwareSerial`, time support, and the ESP32 ADC and
LEDC APIs come from the installed ESP32 core. If Arduino reports more than one
`SD.h`, use the copy supplied with the Espressif ESP32 core.

## Configure TFT_eSPI

TFT_eSPI uses a library-wide setup file rather than settings contained entirely
in the sketch.

1. Locate the installed `TFT_eSPI` library.
2. Copy `config/Setup_GPSClock_CYD.h` into its `User_Setups` directory.
3. Open `TFT_eSPI/User_Setup_Select.h`.
4. Comment out other active setup includes.
5. Add or enable:

```cpp
#include <User_Setups/Setup_GPSClock_CYD.h>
```

6. Save the file and restart Arduino IDE if necessary.

The supplied setup selects the ILI9341 driver, the tested CYD display pins,
HSPI, and the fonts used by the clock. Touch is intentionally handled by the
separate XPT2046 library because the tested CYD uses different touch SPI pins.

## Open and compile v12

Open:

```text
firmware/GPSClock_v12_options_grid/GPSClock_v12_options_grid.ino
```

Arduino IDE will load the other numbered `.ino` files as tabs. Keep every file
in the same sketch folder.

Use these settings:

- Board: **ESP32 Dev Module**
- Upload port: the USB serial port for the CYD, often shown as CH340
- Serial Monitor: **115200 baud**

Click **Verify** before uploading. The verified v12 build reports:

```text
Sketch uses 443589 bytes (33%) of program storage space.
Global variables use 26304 bytes (8%) of dynamic memory.
```

Memory totals can vary slightly with library or core changes.

## Upload

1. Connect the CYD with a data-capable USB cable.
2. Select its serial port.
3. Click **Upload**.
4. Wait for flash verification and the automatic reset.

A normal upload replaces the application firmware but does not erase the NVS
partition that stores Preferences. Selecting a full flash erase in board tools
will reset saved callsign, brightness, alarm, theme, and other preferences.

## Modular source layout

| Tab | Responsibility |
|---|---|
| `GPSClock_v12_options_grid.ino` | Includes, shared state, constants, declarations, setup, loop |
| `10_Settings.ino` | Load/save ESP32 Preferences |
| `20_GPS.ino` | GPS parsing, stable-fix filtering, PPS, duty cycling |
| `30_Clock_Night.ino` | Clock service and R21/schedule Night Mode |
| `35_Brightness.ino` | GPIO21 PWM backlight control |
| `40_Alarm.ino` | Alarm, snooze, timeout, indicators |
| `50_Display.ino` | Main display drawing and cached time/date fields |
| `60_UI.ino` | Touch handling, Options grid, editors |
| `70_Utilities.ino` | Formatting and general helpers |
| `80_Storage_Geo.ino` | SD access, timezone, place, and marine lookup |

Arduino concatenates `.ino` tabs during preprocessing. Shared declarations in
the primary tab are therefore available to the numbered tabs. When changing a
subsystem, inspect its shared declarations and every caller before editing.

## Command-line verification

With Arduino CLI and the required libraries installed:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/GPSClock_v12_options_grid
```

To upload, add the correct port:

```powershell
arduino-cli compile --upload --port COM4 --fqbn esp32:esp32:esp32 firmware/GPSClock_v12_options_grid
```

Replace `COM4` with the actual CYD port. Never select a port solely by number;
confirm the attached USB serial device first.

## Source validation checklist

- Compile with the tested board/core combination.
- Confirm no duplicate Preferences keys were introduced.
- Confirm displayed button rectangles and touch hit regions agree.
- Confirm pin assignments do not conflict across display, touch, SD, GPS, PPS,
  R21, and backlight PWM.
- Test cold boot, warm reboot, GPS acquisition, GPS timeout/retry, SD failure,
  Options persistence, alarms, Night Mode, brightness, and touch editors.
- Preserve `NOTICE`, `LICENSE`, and third-party attribution in redistributed
  source copies.

