# Database Builder and Validation Tools

These scripts convert upstream geographic and timezone data into the compact
binary files read by the ESP32 firmware. They operate on repository-root
`source/` and `output/` directories; neither directory is committed.

## Environment

From the repository root:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -r tools\requirements.txt
```

The versions in `requirements.txt` are the validated build environment. Newer
versions may work but can change ordering or output bytes.

## Input layout

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

`combined-now.json` comes from the `timezones-now.geojson.zip` release of
Timezone Boundary Builder. `allCountries.txt` comes from GeoNames. The marine
shapefile comes from Natural Earth 1:10m Geography Marine Polygons. Timezone
rules are read from Python `zoneinfo` and the installed `tzdata` package.

## Build

Run from the repository root, in this order:

```powershell
python tools\build_timezone_db.py
python tools\build_tz_rules.py
python tools\build_places_db.py
python tools\build_marine_db.py
```

The build creates:

```text
output/
├── index.bin
├── zones.bin
├── tiles.bin
├── rules.bin
├── places.bin
├── places_index.bin
└── marine.bin
```

The place database is the slowest and largest step. Ensure ample free disk
space before starting.

## Validate

```powershell
python tools\test_timezone_db.py
python tools\test_tz_rules.py
python tools\test_places_db.py
python tools\test_places_ranked.py
python tools\test_marine_db.py
```

The tests expect the corresponding files in `output/`. Successful structural
tests do not prove that every upstream geographic fact is correct; spot-check
several known land, ocean, daylight-saving, and border locations before making
a public data release.

To validate an existing data directory elsewhere, set `GPSCLOCK_OUTPUT_DIR`
before running the tests. The builders always write to repository-root
`output/`.

## Copy to microSD

Map the flat output names to the card directories as described in
[`data/README.md`](../data/README.md). Include completed copies of
`data/DATA_LICENSES_TEMPLATE.txt` and `docs/DATA_VERSION_TEMPLATE.txt` in every
distributed data package.

