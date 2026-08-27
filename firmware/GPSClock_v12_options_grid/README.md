# GPSClock v12 - Options Grid

Based on the verified v11 Manual Brightness baseline.

## New in v12

The Options page uses a consistent two-column, four-row layout:

- Local Time | Theme
- Night Mode | Alarm
- Callsign | Set Date/Time
- Brightness | Back

Every visible control is 145 by 43 pixels. The touch handler uses the same
row and column layout, with the narrow visual gaps assigned to the nearest
button so there are no difficult dead zones. The bottom row remains active
through the lower screen edge to tolerate normal resistive-touch variation.

## Brightness

The Options page now includes a persistent manual TFT backlight setting:

- 25%
- 50%
- 75%
- 100%

Touching the BRIGHTNESS row cycles through the four levels. The change is immediate and is stored in
ESP32 Preferences, so the selected level is restored after reboot.

The CYD backlight is driven with 5 kHz, 8-bit PWM on GPIO21 using the ESP32 Arduino Core 3.x LEDC API.

Night Mode remains independent:
- ON forces the red display theme.
- OFF forces the normal theme.
- AUTO uses the onboard R21 ambient-light sensor with the v10 hysteresis/filter logic.

## Arduino IDE

Open `GPSClock_v12_options_grid.ino`, select **ESP32 Dev Module**, Verify, then upload.
Keep all `.ino` files in the same `GPSClock_v12_options_grid` folder.

