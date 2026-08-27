# Offline Database Build

The ESP32 does not parse the original GIS datasets. Python tools preprocess
them on a desktop computer into compact binary indexes optimized for random
access from the microSD card.

## Requirements

- 64-bit Python 3.13 was used for the reference build.
- Several gigabytes of free disk space for downloads, extraction, temporary
  structures, and outputs.
- Enough memory to process worldwide timezone polygons and GeoNames records.

Create a virtual environment from the repository root:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r tools\requirements.txt
```

Linux/macOS activation is normally `source .venv/bin/activate`.

## Obtain source datasets

Download the current versions from their upstream projects and review their
licenses before redistribution:

- [Timezone Boundary Builder](https://github.com/evansiroky/timezone-boundary-builder)
  `timezones-now` GeoJSON output; derived substantially from OpenStreetMap and
  distributed under ODbL 1.0.
- [GeoNames](https://download.geonames.org/export/dump/) worldwide
  `allCountries.zip`; CC BY 4.0.
- [Natural Earth](https://www.naturalearthdata.com/downloads/10m-physical-vectors/10m-physical-labels/)
  1:10m Geography Marine Polygons; public domain.
- IANA timezone rules supplied through Python `zoneinfo` and the pinned
  `tzdata` package.

Do not commit these large source datasets to Git.

## Source directory layout

Extract or rename files so the repository root contains:

```text
source/
├── combined-now.json
├── geonames/
│   └── allCountries.txt
└── marine/
    ├── ne_10m_geography_marine_polys.shp
    ├── ne_10m_geography_marine_polys.shx
    ├── ne_10m_geography_marine_polys.dbf
    ├── ne_10m_geography_marine_polys.prj
    └── ne_10m_geography_marine_polys.cpg
```

## Build order

Run from the repository root:

```powershell
python tools\build_timezone_db.py
python tools\build_tz_rules.py
python tools\build_places_db.py
python tools\build_marine_db.py
```

The builders create `output/` and write:

```text
output/
├── zones.bin
├── index.bin
├── tiles.bin
├── rules.bin
├── rules.txt
├── version.txt
├── places.bin
├── places_index.bin
├── places_info.txt
├── marine.bin
└── marine_info.txt
```

## Validate generated files

The included test utilities read the same binary formats as the firmware and
exercise known coordinate cases:

```powershell
python tools\test_timezone_db.py
python tools\test_tz_rules.py
python tools\test_places_db.py
python tools\test_places_ranked.py
python tools\test_marine_db.py
```

Review all output and resolve failures before copying databases to an SD card.

## Copy to the SD card

Create this exact directory layout:

```text
timezone/zones.bin       <- output/zones.bin
timezone/index.bin       <- output/index.bin
timezone/tiles.bin       <- output/tiles.bin
timezone/rules.bin       <- output/rules.bin
places/places.bin        <- output/places.bin
places/places_index.bin  <- output/places_index.bin
marine/marine.bin        <- output/marine.bin
```

Copy `data/DATA_LICENSES_TEMPLATE.txt` to `DATA_LICENSES.txt`, update it with
the actual dataset versions and retrieval dates, and include it with every data
release. Also complete `docs/DATA_VERSION_TEMPLATE.txt` as `DATA_VERSION.txt`.

## GitHub release packaging

Do not commit generated `.bin` files. Package the seven SD files, completed
license notice, and version record as a release asset such as:

```text
GPSClock_SD_Data_YYYY-MM.zip
```

The timezone boundary files remain an ODbL derivative database and the place
files require GeoNames CC BY 4.0 attribution. See
[Third-party notices](THIRD_PARTY_NOTICES.md).

