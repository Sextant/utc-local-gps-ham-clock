# Changelog

## 1.0.0 - 2026-08-27

First public source release candidate, based on hardware-tested firmware v12.

- Redesigned Options as a consistent two-column, four-row grid with enlarged,
  aligned touch targets and forgiving gap/edge handling.
- Added persistent 25%, 50%, 75%, and 100% GPIO21 PWM backlight settings.
- Added editable callsign stored in ESP32 Preferences and a touchscreen
  keyboard supporting `A-Z`, `0-9`, and `/`.
- Added offline worldwide timezone polygons and IANA-derived UTC/DST rules.
- Added ranked GeoNames place lookup and Natural Earth marine-area lookup.
- Added nautical UTC-offset fallback outside civil-timezone polygons.
- Added stable GPS-fix filtering, movement-triggered geographic re-resolution,
  cached-position status, 30-minute duty cycling, and prompt no-fix retry.
- Added UTC/local display, Maidenhead locator, decimal/DMS coordinates,
  satellite count, altitude, 12/24-hour local time, dark/light themes, visual
  alarm/snooze, and manual date/time controls.
- Added R21-driven AUTO Night Mode with hysteresis/filtering and schedule
  fallback for invalid sensor readings.
- Added complete hardware, firmware, data-build, operation, attribution, and
  release documentation.

### Validated build

- ESP32 Arduino Core 3.3.11
- TFT_eSPI 2.5.43
- XPT2046_Touchscreen 1.4
- TinyGPSPlus 1.0.3
- ESP32 Dev Module: 443,589 bytes flash and 26,304 bytes global memory

