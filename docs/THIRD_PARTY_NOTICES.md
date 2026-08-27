# Third-Party Software and Data Notices

The project-authored firmware, builder tools, configuration, and documentation
are licensed under the PolyForm Noncommercial License 1.0.0. That license does
not replace or restrict the separate licenses of the dependencies and datasets
listed here. This document is a practical attribution record, not legal advice.

The repository does not vendor the Arduino libraries or upstream datasets.
Install or download them from their publishers and retain their notices when
redistributing them.

## Embedded software dependencies

### TFT_eSPI 2.5.43 — Bodmer

- Upstream: <https://github.com/Bodmer/TFT_eSPI>
- Original Bodmer code: FreeBSD license.
- Inherited Adafruit ILI9341 portions: MIT license.
- Selected Adafruit_GFX portions: BSD license.

The upstream `license.txt` describes these components. Preserve it if TFT_eSPI
source is included in a distribution.

### XPT2046_Touchscreen 1.4 — Paul Stoffregen

- Upstream: <https://github.com/PaulStoffregen/XPT2046_Touchscreen>
- Copyright 2015 Paul Stoffregen.
- License: MIT-style permissive license in the source header.

### TinyGPSPlus 1.0.3 — Mikal Hart

- Upstream: <https://github.com/mikalhart/TinyGPSPlus>
- Copyright 2008-2013 Mikal Hart.
- License: GNU Lesser General Public License, version 2.1 or later.

This repository publishes firmware source and requires users to install the
library separately. Before distributing a precompiled image linked with LGPL
components, review the applicable LGPL source, notice, and relinking duties.

### Arduino-ESP32 3.3.11 — Espressif Systems

- Upstream: <https://github.com/espressif/arduino-esp32>
- Repository-level package license: LGPL-2.1-or-later.
- Individual included components can carry other compatible licenses, including
  Apache-2.0.

Install the core through Arduino Boards Manager. Do not copy the core into this
repository without preserving all applicable component notices.

## Builder dependencies

The following Python packages are installed by the builder and are not bundled:

| Package | Validated version | License/notes |
|---|---:|---|
| Shapely | 2.1.2 | BSD-3-Clause; uses the separately licensed GEOS library |
| orjson | 3.12.0 | Upstream dual/combined licensing applies |
| NumPy | 2.5.2 | BSD-3-Clause |
| tzdata | 2026.3 | Supplies IANA timezone data to Python `zoneinfo` |
| PyShp | 3.1.6 | MIT |

See each installed distribution and upstream project for its complete license
text and transitive dependencies.

## Geographic and timezone data

Generated SD-card binaries are derived databases and retain the upstream data
terms below. A data ZIP must include a completed `DATA_LICENSES.txt` based on
`data/DATA_LICENSES_TEMPLATE.txt` and a `DATA_VERSION.txt` identifying the
source releases and download dates.

### Timezone Boundary Builder / OpenStreetMap

- Builder: <https://github.com/evansiroky/timezone-boundary-builder>
- Output data terms: <https://github.com/evansiroky/timezone-boundary-builder/blob/master/DATA_LICENSE>
- Open Database License 1.0: <https://opendatacommons.org/licenses/odbl/1-0/>
- OpenStreetMap attribution: <https://www.openstreetmap.org/copyright>

`timezone/index.bin`, `timezone/zones.bin`, and `timezone/tiles.bin` are derived
from Timezone Boundary Builder output, which is substantially derived from
OpenStreetMap contributors and made available under ODbL 1.0.

### IANA Time Zone Database

- Source: <https://www.iana.org/time-zones>
- License information: <https://data.iana.org/time-zones/tz-link.html>

`timezone/rules.bin` contains extracted civil-time transition facts. IANA
describes the database data as public domain; individual distribution files can
also contain BSD-licensed code or material.

### GeoNames

- Source and attribution: <https://www.geonames.org/>
- Download readme: <https://download.geonames.org/export/dump/readme.txt>
- License: <https://creativecommons.org/licenses/by/4.0/>

`places/places.bin` and `places/places_index.bin` are derived from GeoNames
`allCountries.txt` and are distributed with attribution under CC BY 4.0.

### Natural Earth

- Source: <https://www.naturalearthdata.com/>
- Terms: <https://www.naturalearthdata.com/about/terms-of-use/>

`marine/marine.bin` is derived from Natural Earth 1:10m Geography Marine
Polygons. Natural Earth identifies its raster and vector data as public domain;
this project voluntarily credits Natural Earth.

## Distribution guidance

- Keep project `LICENSE` and `NOTICE` with project-authored files.
- Keep third-party libraries outside the repository unless their complete
  license and copyright notices are included.
- Distribute generated databases separately with their data notice and version
  record.
- Do not describe the entire SD-data ZIP as covered only by the project's
  PolyForm license; ODbL, CC BY, and other upstream terms continue to apply.
- Source-only firmware releases are the simplest initial distribution. Treat a
  future compiled firmware image as a separate licensing review.

