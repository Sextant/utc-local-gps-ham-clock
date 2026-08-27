from pathlib import Path
import struct
import math
import time


# ============================================================
# PATHS
# ============================================================

BASE = Path(__file__).resolve().parent.parent

SOURCE_FILE = (
    BASE
    / "source"
    / "geonames"
    / "allCountries.txt"
)

OUTPUT = BASE / "output"

PLACES_FILE = OUTPUT / "places.bin"
PLACE_INDEX_FILE = OUTPUT / "places_index.bin"
PLACE_INFO_FILE = OUTPUT / "places_info.txt"


# ============================================================
# DATABASE SETTINGS
# ============================================================

# Spatial index cell size.
#
# 0.25 degree = about 27.8 km north/south.
#
# The ESP32 will search the current cell and surrounding
# cells as necessary. This keeps the index small while
# retaining every populated place.
#
CELL_DEGREES = 0.25

GRID_COLS = int(
    360.0 / CELL_DEGREES
)

GRID_ROWS = int(
    180.0 / CELL_DEGREES
)

GRID_CELL_COUNT = (
    GRID_COLS * GRID_ROWS
)


# ============================================================
# RECORD FORMAT
# ============================================================

# Each places.bin record contains:
#
# int32  latitude  * 1,000,000
# int32  longitude * 1,000,000
# uint32 population
# uint32 GeoNames ID
# uint8  country-code length
# bytes  country code
# uint8  feature-code length
# bytes  feature code
# uint16 UTF-8 name length
# bytes  UTF-8 place name
#
# Variable-length records allow us to preserve full Unicode
# place names without wasting large amounts of space.


# ============================================================
# HELPER FUNCTIONS
# ============================================================

def grid_cell(
    latitude,
    longitude
):

    col = int(
        (longitude + 180.0)
        / CELL_DEGREES
    )

    row = int(
        (latitude + 90.0)
        / CELL_DEGREES
    )

    col = max(
        0,
        min(
            GRID_COLS - 1,
            col
        )
    )

    row = max(
        0,
        min(
            GRID_ROWS - 1,
            row
        )
    )

    return (
        row * GRID_COLS
        + col
    )


def safe_uint32(value):

    try:

        number = int(value)

    except (ValueError, TypeError):

        return 0

    if number < 0:
        return 0

    if number > 0xFFFFFFFF:
        return 0xFFFFFFFF

    return number


# ============================================================
# CHECK SOURCE
# ============================================================

print()
print("========================================")
print("CGPS CLOCK PLACE DATABASE BUILDER")
print("========================================")
print()

print("Source:")
print(SOURCE_FILE)
print()


if not SOURCE_FILE.exists():

    raise FileNotFoundError(
        f"Cannot find:\n{SOURCE_FILE}"
    )


OUTPUT.mkdir(
    parents=True,
    exist_ok=True
)


source_size = SOURCE_FILE.stat().st_size

print(
    "Source size:",
    f"{source_size / (1024 * 1024):,.1f}",
    "MB"
)

print()
print(
    "Keeping ALL GeoNames feature-class P records."
)

print(
    "No population cutoff."
)

print(
    "Spatial cell size:",
    CELL_DEGREES,
    "degrees"
)

print(
    "Grid:",
    GRID_COLS,
    "x",
    GRID_ROWS
)

print()


# ============================================================
# PASS 1
#
# Count records in every geographic cell.
# ============================================================

print("PASS 1 OF 2")
print(
    "Scanning GeoNames populated places..."
)
print()

start_time = time.time()

cell_counts = [
    0
] * GRID_CELL_COUNT

total_lines = 0
place_count = 0
bad_records = 0


with open(
    SOURCE_FILE,
    "r",
    encoding="utf-8",
    errors="replace"
) as source:

    for line in source:

        total_lines += 1

        fields = line.rstrip(
            "\n"
        ).split("\t")

        # GeoNames allCountries.txt has 19 fields.
        if len(fields) < 19:

            bad_records += 1
            continue

        # Feature class.
        if fields[6] != "P":
            continue

        try:

            latitude = float(
                fields[4]
            )

            longitude = float(
                fields[5]
            )

        except ValueError:

            bad_records += 1
            continue

        cell = grid_cell(
            latitude,
            longitude
        )

        cell_counts[cell] += 1
        place_count += 1

        if (
            total_lines %
            2_000_000 == 0
        ):

            elapsed = (
                time.time()
                - start_time
            )

            print(
                f"  Lines: {total_lines:,}   "
                f"Places: {place_count:,}   "
                f"Elapsed: {elapsed:.0f}s"
            )


print()
print(
    "Total source records:",
    f"{total_lines:,}"
)

print(
    "Populated places:",
    f"{place_count:,}"
)

print(
    "Rejected malformed records:",
    f"{bad_records:,}"
)

print()


# ============================================================
# CALCULATE CELL START RECORDS
# ============================================================

print(
    "Building spatial index..."
)

cell_starts = [
    0
] * GRID_CELL_COUNT

running = 0

for i in range(
    GRID_CELL_COUNT
):

    cell_starts[i] = running

    running += (
        cell_counts[i]
    )


if running != place_count:

    raise RuntimeError(
        "Internal place count mismatch."
    )


# ============================================================
# PASS 2
#
# We need records grouped by geographic cell.
#
# To avoid keeping millions of variable-length strings in RAM,
# write temporary per-cell-group files using buckets.
# ============================================================

print()
print("PASS 2 OF 2")
print(
    "Writing indexed populated-place database..."
)
print()


# Use 256 temporary buckets instead of one file per grid cell.
BUCKET_COUNT = 256

TEMP_DIR = (
    OUTPUT
    / "_place_build_temp"
)

TEMP_DIR.mkdir(
    parents=True,
    exist_ok=True
)


bucket_paths = [

    TEMP_DIR
    / f"bucket_{i:03d}.bin"

    for i in range(
        BUCKET_COUNT
    )
]


bucket_files = [

    open(
        path,
        "wb"
    )

    for path in bucket_paths
]


written_places = 0


with open(
    SOURCE_FILE,
    "r",
    encoding="utf-8",
    errors="replace"
) as source:

    for line in source:

        fields = line.rstrip(
            "\n"
        ).split("\t")

        if len(fields) < 19:
            continue

        if fields[6] != "P":
            continue

        try:

            geoname_id = int(
                fields[0]
            )

            latitude = float(
                fields[4]
            )

            longitude = float(
                fields[5]
            )

        except ValueError:

            continue


        name = fields[1]

        country_code = fields[8]

        feature_code = fields[7]

        population = safe_uint32(
            fields[14]
        )


        name_bytes = name.encode(
            "utf-8",
            errors="replace"
        )

        country_bytes = (
            country_code.encode(
                "ascii",
                errors="replace"
            )
        )

        feature_bytes = (
            feature_code.encode(
                "ascii",
                errors="replace"
            )
        )


        # Safety limits.
        name_bytes = (
            name_bytes[:65535]
        )

        country_bytes = (
            country_bytes[:255]
        )

        feature_bytes = (
            feature_bytes[:255]
        )


        lat_scaled = int(
            round(
                latitude
                * 1_000_000
            )
        )

        lon_scaled = int(
            round(
                longitude
                * 1_000_000
            )
        )


        cell = grid_cell(
            latitude,
            longitude
        )


        # Assign the geographic cell to one of 256 buckets.
        bucket = (
            cell %
            BUCKET_COUNT
        )


        f = bucket_files[
            bucket
        ]


        # Cell number first so records can later be regrouped.
        f.write(
            struct.pack(
                "<I",
                cell
            )
        )

        f.write(
            struct.pack(
                "<iiII",
                lat_scaled,
                lon_scaled,
                population,
                geoname_id
            )
        )

        f.write(
            struct.pack(
                "<B",
                len(country_bytes)
            )
        )

        f.write(
            country_bytes
        )

        f.write(
            struct.pack(
                "<B",
                len(feature_bytes)
            )
        )

        f.write(
            feature_bytes
        )

        f.write(
            struct.pack(
                "<H",
                len(name_bytes)
            )
        )

        f.write(
            name_bytes
        )


        written_places += 1

        if (
            written_places %
            500_000 == 0
        ):

            print(
                "  Prepared:",
                f"{written_places:,}",
                "places"
            )


for f in bucket_files:

    f.close()


# ============================================================
# READ TEMP RECORD
# ============================================================

def read_temp_record(f):

    raw = f.read(4)

    if len(raw) != 4:
        return None

    cell = struct.unpack(
        "<I",
        raw
    )[0]


    fixed = f.read(16)

    if len(fixed) != 16:
        raise RuntimeError(
            "Truncated temporary record."
        )

    (
        lat_scaled,
        lon_scaled,
        population,
        geoname_id
    ) = struct.unpack(
        "<iiII",
        fixed
    )


    country_len_raw = f.read(1)

    if not country_len_raw:
        raise RuntimeError(
            "Truncated country field."
        )

    country_len = (
        country_len_raw[0]
    )

    country = f.read(
        country_len
    )


    feature_len_raw = f.read(1)

    if not feature_len_raw:
        raise RuntimeError(
            "Truncated feature field."
        )

    feature_len = (
        feature_len_raw[0]
    )

    feature = f.read(
        feature_len
    )


    name_len_raw = f.read(2)

    if len(name_len_raw) != 2:
        raise RuntimeError(
            "Truncated name length."
        )

    name_len = struct.unpack(
        "<H",
        name_len_raw
    )[0]

    name = f.read(
        name_len
    )


    return (
        cell,
        lat_scaled,
        lon_scaled,
        population,
        geoname_id,
        country,
        feature,
        name
    )


# ============================================================
# LOAD BUCKETS AND GROUP BY CELL
# ============================================================

# places.bin header:
#
# 4 bytes magic PLAC
# 2 bytes version
# 4 bytes record count
#
# Records are then variable length.

PLACES_HEADER_SIZE = 10


cell_offsets = [
    0
] * GRID_CELL_COUNT


with open(
    PLACES_FILE,
    "wb"
) as output_file:

    output_file.write(
        b"PLAC"
    )

    output_file.write(
        struct.pack(
            "<H",
            1
        )
    )

    output_file.write(
        struct.pack(
            "<I",
            place_count
        )
    )


    places_written_final = 0


    for bucket_number in range(
        BUCKET_COUNT
    ):

        path = bucket_paths[
            bucket_number
        ]

        records_by_cell = {}


        with open(
            path,
            "rb"
        ) as f:

            while True:

                record = (
                    read_temp_record(f)
                )

                if record is None:
                    break

                cell = record[0]

                records_by_cell.setdefault(
                    cell,
                    []
                ).append(
                    record
                )


        for cell in sorted(
            records_by_cell.keys()
        ):

            cell_offsets[cell] = (
                output_file.tell()
            )


            for record in records_by_cell[
                cell
            ]:

                (
                    _,
                    lat_scaled,
                    lon_scaled,
                    population,
                    geoname_id,
                    country,
                    feature,
                    name
                ) = record


                output_file.write(
                    struct.pack(
                        "<iiII",
                        lat_scaled,
                        lon_scaled,
                        population,
                        geoname_id
                    )
                )

                output_file.write(
                    struct.pack(
                        "<B",
                        len(country)
                    )
                )

                output_file.write(
                    country
                )

                output_file.write(
                    struct.pack(
                        "<B",
                        len(feature)
                    )
                )

                output_file.write(
                    feature
                )

                output_file.write(
                    struct.pack(
                        "<H",
                        len(name)
                    )
                )

                output_file.write(
                    name
                )


                places_written_final += 1


        print(
            f"  Bucket "
            f"{bucket_number + 1:03d}/"
            f"{BUCKET_COUNT:03d}   "
            f"Final records: "
            f"{places_written_final:,}"
        )


# ============================================================
# WRITE SPATIAL INDEX
# ============================================================

print()
print(
    "Writing places_index.bin..."
)


with open(
    PLACE_INDEX_FILE,
    "wb"
) as f:

    # Header
    f.write(
        b"PIDX"
    )

    f.write(
        struct.pack(
            "<H",
            1
        )
    )

    f.write(
        struct.pack(
            "<f",
            CELL_DEGREES
        )
    )

    f.write(
        struct.pack(
            "<HH",
            GRID_COLS,
            GRID_ROWS
        )
    )

    f.write(
        struct.pack(
            "<I",
            GRID_CELL_COUNT
        )
    )


    # Each cell:
    #
    # uint32 file offset in places.bin
    # uint32 number of records
    #
    for cell in range(
        GRID_CELL_COUNT
    ):

        if cell_counts[cell] == 0:

            offset = 0

        else:

            offset = cell_offsets[
                cell
            ]


        f.write(
            struct.pack(
                "<II",
                offset,
                cell_counts[cell]
            )
        )


# ============================================================
# CLEAN TEMP FILES
# ============================================================

print()
print(
    "Cleaning temporary files..."
)


for path in bucket_paths:

    try:
        path.unlink()

    except FileNotFoundError:
        pass


try:
    TEMP_DIR.rmdir()

except OSError:
    pass


# ============================================================
# WRITE INFORMATION FILE
# ============================================================

elapsed = (
    time.time()
    - start_time
)


with open(
    PLACE_INFO_FILE,
    "w",
    encoding="utf-8"
) as f:

    f.write(
        "CGPS CLOCK WORLDWIDE PLACE DATABASE\n"
    )

    f.write(
        "===================================\n\n"
    )

    f.write(
        f"Source: {SOURCE_FILE.name}\n"
    )

    f.write(
        "GeoNames feature class: P\n"
    )

    f.write(
        "Population cutoff: NONE\n"
    )

    f.write(
        f"Place records: {place_count:,}\n"
    )

    f.write(
        f"Cell resolution: {CELL_DEGREES} degrees\n"
    )

    f.write(
        f"Grid: {GRID_COLS} x {GRID_ROWS}\n"
    )

    f.write(
        f"places.bin: {PLACES_FILE.stat().st_size:,} bytes\n"
    )

    f.write(
        f"places_index.bin: {PLACE_INDEX_FILE.stat().st_size:,} bytes\n"
    )


# ============================================================
# COMPLETE
# ============================================================

print()
print(
    "========================================"
)

print(
    "PLACE DATABASE BUILD COMPLETE"
)

print(
    "========================================"
)

print()

print(
    "Places:",
    f"{place_count:,}"
)

print(
    "places.bin:",
    f"{PLACES_FILE.stat().st_size:,}",
    "bytes"
)

print(
    "places_index.bin:",
    f"{PLACE_INDEX_FILE.stat().st_size:,}",
    "bytes"
)

print(
    "Build time:",
    f"{elapsed:.1f}",
    "seconds"
)

print()

print(
    "Created:"
)

print(
    PLACES_FILE
)

print(
    PLACE_INDEX_FILE
)

print(
    PLACE_INFO_FILE
)

print()

input(
    "Press ENTER to exit..."
)

