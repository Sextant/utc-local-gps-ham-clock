# SD-Card Data

The firmware expects the following files:

```text
/timezone/index.bin
/timezone/zones.bin
/timezone/tiles.bin
/timezone/rules.bin
/places/places.bin
/places/places_index.bin
/marine/marine.bin
```

Reference-build sizes were approximately:

| File | Approx. size | Source |
|---|---:|---|
| `timezone/zones.bin` | 1 KB | Timezone Boundary Builder |
| `timezone/index.bin` | 324 KB | Timezone Boundary Builder |
| `timezone/tiles.bin` | 17.9 MB | Timezone Boundary Builder |
| `timezone/rules.bin` | 16.7 KB | IANA tzdb-derived rules |
| `places/places.bin` | 185.6 MB | GeoNames |
| `places/places_index.bin` | 8.3 MB | GeoNames |
| `marine/marine.bin` | 308 KB | Natural Earth |

## GitHub distribution

Do not commit `places.bin` to normal Git history. GitHub blocks ordinary repository objects above 100 MB. Instead, create a GitHub Release and attach a ZIP such as:

`GPSClock_SD_Data_2026-08.zip`

GitHub Releases support individual release assets up to 2 GiB, so the complete SD data package fits comfortably as one asset.

The ZIP should contain:

```text
timezone/
places/
marine/
DATA_LICENSES.txt
DATA_VERSION.txt
```

## Required data notice

A release containing the generated databases should include a notice substantially like:

```text
TIMEZONE BOUNDARIES
Contains information from Timezone Boundary Builder, made available under
ODbL 1.0, and derived substantially from OpenStreetMap contributors.
https://github.com/evansiroky/timezone-boundary-builder
https://opendatacommons.org/licenses/odbl/1-0/
https://www.openstreetmap.org/copyright

TIMEZONE RULES
Civil-time rules derived from the IANA Time Zone Database (tzdb).
https://www.iana.org/time-zones

PLACE NAMES
Derived from GeoNames and distributed with attribution under CC BY 4.0.
https://www.geonames.org/
https://creativecommons.org/licenses/by/4.0/

MARINE AREAS
Derived from Natural Earth 1:10m Geography Marine Polygons.
Natural Earth data is public domain.
https://www.naturalearthdata.com/
```

## Rebuilding data

The PC-side builder scripts belong in `tools/`. Rebuilding from source is preferable when timezone or place datasets change. Keep source-data version information in `DATA_VERSION.txt` so users can tell how current a release asset is.

