# Maintainer Publishing Guide

This document is for the repository owner and anyone explicitly authorized to
maintain releases. It is not part of the normal clock-building procedure.

## Repository metadata

- Name: `offline-gps-ham-clock`
- Description: `Offline GPS-synchronized UTC/local ham clock for the ESP32 CYD with touchscreen, microSD geographic databases, and Maidenhead locator.`
- Suggested topics: `esp32`, `arduino`, `gps`, `ham-radio`, `amateur-radio`,
  `cyd`, `esp32-2432s028`, `timezone`, `offline`, `maidenhead`, `touchscreen`

## First source release

Use tag `v1.0.0` for the source state documented in `CHANGELOG.md`. The first
release should be source-only; do not attach a compiled firmware image until
the LGPL binary-distribution obligations described in
`THIRD_PARTY_NOTICES.md` have been intentionally addressed.

Before tagging:

1. Compile `firmware/GPSClock_v12_options_grid` with the tested versions.
2. Confirm the exact build memory totals or update the documentation.
3. Test the Options grid, callsign, brightness persistence, GPS fix and retry,
   local timezone, SD lookup, alarm, and R21 AUTO Night Mode on hardware.
4. Search for personal coordinates, credentials, private paths, and temporary
   files.
5. Confirm `LICENSE`, `NOTICE`, third-party notices, and data notices agree.
6. Commit the validated source, create signed/annotated tag `v1.0.0`, and create
   a GitHub Release from that tag.

## Optional SD-data release asset

Do not commit the generated `.bin` files. `places.bin` exceeds GitHub's normal
100 MB Git-object limit. Package the card data separately as, for example:

```text
GPSClock_SD_Data_2026-08.zip
├── timezone/
│   ├── index.bin
│   ├── zones.bin
│   ├── tiles.bin
│   └── rules.bin
├── places/
│   ├── places.bin
│   └── places_index.bin
├── marine/
│   └── marine.bin
├── DATA_LICENSES.txt
└── DATA_VERSION.txt
```

Complete both text files before packaging. Record a SHA-256 checksum for the
ZIP in the release notes. A data asset has its own upstream ODbL, CC BY 4.0,
public-domain, and attribution requirements; it is not licensed solely under
the project's PolyForm license.

## Screenshots and privacy

Useful public screenshots include the main clock, Options grid, and rear GPS
wiring. Inspect images for home/work coordinates, callsigns, serial numbers,
reflections, and background information. Crop or obscure anything the builder
does not want made public.

## Visibility

Prepare and verify the project in a private repository. Changing the repository
to public is a separate publication step and should happen only after the owner
reviews the source, documentation, license choice, and screenshots.

