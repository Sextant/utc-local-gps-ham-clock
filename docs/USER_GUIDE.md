# User Guide

## First start

1. Insert the prepared FAT32 microSD card before applying power.
2. Power the clock from a stable 5 V USB supply.
3. Put the GPS antenna at a window or outdoors with a clear view of the sky.
4. Wait for the bottom status line to change from acquisition to a synchronized
   or cached state. A cold start can take several minutes.
5. Tap the header at the top of the clock screen to open **OPTIONS**, then set
   the callsign and preferred display choices.

No Wi-Fi, account, cloud service, or Internet connection is used.

## Main clock screen

The normal display shows the callsign, Maidenhead locator, UTC and local date
and time, timezone abbreviation and offset, resolved place or marine-area name,
satellite count, coordinates, and GNSS altitude. During poor reception the
clock may continue using the last accepted position and identify it as cached.

## Main-screen touch controls

- Tap the top/header area to open **OPTIONS**.
- Tap the LOCAL time field to switch between 12-hour and 24-hour local time.
- Tap the bottom GPS line to switch coordinates between signed decimal degrees
  and degrees/minutes/seconds.

## Options screen

Photos: [dark Options screen](../images/options-screen-dark-theme.jpg) and
[light Options screen](../images/options-screen-light-theme.jpg).

v12 uses eight equal 145 x 43 pixel controls in a two-column grid. The visual
gaps are assigned to the nearest button to make operation easier on the
resistive touchscreen.

| Left column | Right column |
|---|---|
| Local Time | Theme |
| Night Mode | Alarm |
| Callsign | Set Date/Time |
| Brightness | Back |

### Local Time

Cycles the local-time display between 12-hour and 24-hour formats.

### Theme

Cycles between the normal dark and light themes. Night Mode remains a separate
setting and can temporarily override the normal theme.

### Night Mode

- **AUTO** reads the onboard R21 ambient-light sensor. Hysteresis and repeated
  confirmations prevent rapid switching near the threshold. The saved start
  and end times are used only if the sensor reading is invalid.
- **ON** forces the red/black night display.
- **OFF** disables the red display and uses the selected normal theme.

For AUTO to work, the enclosure must expose R21 to room light without letting
the TFT backlight shine directly onto it.

See the [Night Mode settings](../images/night-mode-settings-screen.jpg) and
[night-vision-safe red display](../images/night-vision-safe-mode.jpg).

### Alarm

Opens the visual-alarm editor. Set the local alarm hour and minute, then enable
or disable the alarm. When triggered, the display flashes and offers stop and
snooze controls. Snooze is nine minutes; an unanswered alarm times out after
two minutes. The reference build has no buzzer or speaker.

### Callsign

Opens the touchscreen editor. It supports `A-Z`, `0-9`, and `/`, with a maximum
length of 15 characters. **SAVE** writes the value to ESP32 Preferences. New
installations show `NOCALL` until changed.

### Set Date/Time

Provides manual date/time adjustment and a **NOW GPS** control. NOW GPS wakes
the receiver immediately when it is asleep and returns to Options. A later
accepted GPS time can replace manually entered time.

### Brightness

Cycles the TFT backlight through 25%, 50%, 75%, and 100%. The change is
immediate and persists across power cycles and normal firmware uploads.

### Back

Returns to the main clock screen.

## GPS behavior

The GPS supplies accurate UTC and geographic context but is duty-cycled rather
than run continuously.

- A position is accepted after three mutually consistent samples with at least
  four satellites.
- A step greater than 100 m between candidate samples restarts the filter.
- After a successful cycle, the receiver normally sleeps for about 30 minutes.
- An acquisition attempt can run for up to 120 seconds.
- A failed attempt is retried after about two minutes.
- A meaningful location change causes the timezone, place, marine area, and
  Maidenhead locator to be resolved again.

## Saved settings

Callsign, brightness, local-time format, theme, Night Mode and schedule, alarm,
coordinate format, and cached operating state are stored in ESP32 Preferences.
A normal upload does not erase them. Selecting a full flash erase does.

## Troubleshooting

### UTC works but local time or locality does not

Confirm all seven database files are present in the exact SD-card directories
shown in [Data build](DATA_BUILD.md). Check the 115200-baud Serial Monitor for
database readiness or file-open errors.

### No GPS fix indoors

Move the antenna to a window or outdoors. Metal, coated glass, the enclosure,
and nearby electronics can reduce reception. Cold starts take longer.

### AUTO Night Mode does not respond

Check that the R21 light sensor is not covered and does not see direct light
from the display. Forced ON and OFF can help distinguish a sensor problem from
a theme/display problem.

### Touch does not match the visible controls

Confirm this is the v12 Options-grid firmware and that the exact CYD board
revision matches the documented XPT2046 wiring. Touch and TFT use different
SPI pin sets on the tested board.

### Screen is blank or garbled

Verify the ILI9341 selection, TFT_eSPI setup file, board selection, backlight
GPIO21 configuration, and a stable USB supply.

