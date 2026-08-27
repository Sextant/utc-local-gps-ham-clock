import os
import struct
import sys
from pathlib import Path


# ============================================================
# PATHS
# ============================================================

BASE = Path(__file__).resolve().parent.parent
OUTPUT = Path(os.environ.get("GPSCLOCK_OUTPUT_DIR", BASE / "output"))

ZONES_FILE = OUTPUT / "zones.bin"
INDEX_FILE = OUTPUT / "index.bin"
TILES_FILE = OUTPUT / "tiles.bin"


# ============================================================
# CONSTANTS
# ============================================================

CELL_DIRECT  = 0
CELL_TILE    = 1
CELL_UNKNOWN = 2

UNKNOWN_ZONE = 255


# ============================================================
# READ ZONE NAMES
# ============================================================

def load_zones():

    zones = []

    with open(ZONES_FILE, "rb") as f:

        magic = f.read(4)

        if magic != b"TZON":
            raise RuntimeError("Invalid zones.bin")

        version = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        zone_count = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        for _ in range(zone_count):

            length = struct.unpack(
                "<B",
                f.read(1)
            )[0]

            name = f.read(
                length
            ).decode("utf-8")

            zones.append(name)

    return zones


# ============================================================
# READ INDEX HEADER
# ============================================================

def load_index_header():

    with open(INDEX_FILE, "rb") as f:

        magic = f.read(4)

        if magic != b"TZIX":
            raise RuntimeError("Invalid index.bin")

        version = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        (
            coarse_cols,
            coarse_rows,
            coarse_deg,
            fine_deg,
            fine_per_coarse
        ) = struct.unpack(
            "<HHffH",
            f.read(14)
        )

        record_count = struct.unpack(
            "<I",
            f.read(4)
        )[0]

    return {
        "version": version,
        "coarse_cols": coarse_cols,
        "coarse_rows": coarse_rows,
        "coarse_deg": coarse_deg,
        "fine_deg": fine_deg,
        "fine_per_coarse": fine_per_coarse,
        "record_count": record_count,
        "header_size": 24
    }


# ============================================================
# READ TILE HEADER
# ============================================================

def load_tile_header():

    with open(TILES_FILE, "rb") as f:

        magic = f.read(4)

        if magic != b"TZTL":
            raise RuntimeError("Invalid tiles.bin")

        version = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        width, height = struct.unpack(
            "<HH",
            f.read(4)
        )

        tile_count = struct.unpack(
            "<I",
            f.read(4)
        )[0]

    return {
        "version": version,
        "width": width,
        "height": height,
        "tile_count": tile_count,
        "header_size": 14
    }


# ============================================================
# NAUTICAL TIMEZONE FALLBACK
# ============================================================

def nautical_timezone(longitude):

    """
    Returns a nominal nautical UTC offset based on longitude.

    Each nautical zone is approximately 15 degrees wide.
    """

    offset = int(
        round(longitude / 15.0)
    )

    if offset < -12:
        offset = -12

    if offset > 12:
        offset = 12

    if offset == 0:
        return "UTC", 0

    if offset > 0:
        return f"UTC+{offset}", offset

    return f"UTC{offset}", offset


# ============================================================
# TIMEZONE LOOKUP
# ============================================================

def lookup_timezone(
    latitude,
    longitude,
    zones,
    index_info,
    tile_info
):

    coarse_deg = index_info[
        "coarse_deg"
    ]

    coarse_cols = index_info[
        "coarse_cols"
    ]

    coarse_rows = index_info[
        "coarse_rows"
    ]


    # --------------------------------------------------------
    # VALIDATE
    # --------------------------------------------------------

    if latitude < -90 or latitude > 90:
        return None, None, False

    if longitude < -180 or longitude > 180:
        return None, None, False


    if longitude == 180:
        longitude = 179.999999

    if latitude == 90:
        latitude = 89.999999


    # --------------------------------------------------------
    # COARSE CELL
    # --------------------------------------------------------

    col = int(
        (longitude + 180.0)
        / coarse_deg
    )

    row = int(
        (latitude + 90.0)
        / coarse_deg
    )


    if (
        col < 0
        or col >= coarse_cols
        or row < 0
        or row >= coarse_rows
    ):
        return None, None, False


    record_number = (
        row * coarse_cols
        + col
    )

    record_offset = (
        index_info["header_size"]
        + record_number * 5
    )


    with open(INDEX_FILE, "rb") as f:

        f.seek(record_offset)

        cell_type, value = struct.unpack(
            "<BI",
            f.read(5)
        )


    # --------------------------------------------------------
    # DIRECT LAND CELL
    # --------------------------------------------------------

    if cell_type == CELL_DIRECT:

        if value >= len(zones):
            return None, None, False

        return zones[value], None, False


    # --------------------------------------------------------
    # UNKNOWN = OCEAN / NO CIVIL POLYGON
    # --------------------------------------------------------

    if cell_type == CELL_UNKNOWN:

        nautical_name, nautical_offset = nautical_timezone(
            longitude
        )

        return (
            nautical_name,
            nautical_offset,
            True
        )


    # --------------------------------------------------------
    # FINE TILE
    # --------------------------------------------------------

    if cell_type != CELL_TILE:

        return None, None, False


    fine_deg = index_info[
        "fine_deg"
    ]

    fine_per_coarse = index_info[
        "fine_per_coarse"
    ]


    coarse_lon0 = (
        -180.0
        + col * coarse_deg
    )

    coarse_lat0 = (
        -90.0
        + row * coarse_deg
    )


    fine_col = int(
        (longitude - coarse_lon0)
        / fine_deg
    )

    fine_row = int(
        (latitude - coarse_lat0)
        / fine_deg
    )


    fine_col = max(
        0,
        min(
            fine_per_coarse - 1,
            fine_col
        )
    )

    fine_row = max(
        0,
        min(
            fine_per_coarse - 1,
            fine_row
        )
    )


    fine_index = (
        fine_row
        * fine_per_coarse
        + fine_col
    )


    tile_size = (
        tile_info["width"]
        * tile_info["height"]
    )


    tile_offset = (
        tile_info["header_size"]
        + value * tile_size
        + fine_index
    )


    with open(TILES_FILE, "rb") as f:

        f.seek(tile_offset)

        raw = f.read(1)


    if not raw:

        return None, None, False


    zone_id = raw[0]


    # Fine cell unresolved -> nautical fallback
    if zone_id == UNKNOWN_ZONE:

        nautical_name, nautical_offset = nautical_timezone(
            longitude
        )

        return (
            nautical_name,
            nautical_offset,
            True
        )


    if zone_id >= len(zones):

        return None, None, False


    return zones[zone_id], None, False


# ============================================================
# MAIN
# ============================================================

print()
print("========================================")
print("CGPS CLOCK TIMEZONE DATABASE TEST")
print("========================================")
print()


zones = load_zones()
index_info = load_index_header()
tile_info = load_tile_header()


print(
    "Zones loaded:",
    len(zones)
)

print(
    "Coarse grid:",
    index_info["coarse_cols"],
    "x",
    index_info["coarse_rows"]
)

print(
    "Fine resolution:",
    index_info["fine_deg"],
    "degrees"
)

print(
    "Fine tiles:",
    tile_info["tile_count"]
)

print()


# ============================================================
# TEST LOCATIONS
# ============================================================

tests = [

    (
        "Carmel Valley, California",
        36.48,
        -121.73
    ),

    (
        "Boise, Idaho",
        43.6150,
        -116.2023
    ),

    (
        "New York City",
        40.7128,
        -74.0060
    ),

    (
        "Phoenix, Arizona",
        33.4484,
        -112.0740
    ),

    (
        "Honolulu, Hawaii",
        21.3069,
        -157.8583
    ),

    (
        "London, England",
        51.5074,
        -0.1278
    ),

    (
        "Tokyo, Japan",
        35.6762,
        139.6503
    ),

    (
        "Kathmandu, Nepal",
        27.7172,
        85.3240
    ),

    (
        "Sydney, Australia",
        -33.8688,
        151.2093
    ),

    # Offshore California
    (
        "Pacific Ocean west of Monterey",
        36.50,
        -125.00
    ),

    # Central Pacific
    (
        "Central Pacific",
        20.00,
        -150.00
    ),

    # Mid Atlantic
    (
        "Mid Atlantic",
        30.00,
        -40.00
    ),

]


print("TEST RESULTS")
print("----------------------------------------")
print()


for (
    name,
    latitude,
    longitude
) in tests:

    timezone, offset, nautical = lookup_timezone(
        latitude,
        longitude,
        zones,
        index_info,
        tile_info
    )

    print(name)

    print(
        "  Coordinates:",
        latitude,
        longitude
    )

    if timezone is None:

        print(
            "  Timezone: ERROR"
        )

    elif nautical:

        print(
            "  Timezone:",
            timezone,
            "(NAUTICAL)"
        )

    else:

        print(
            "  Timezone:",
            timezone,
            "(CIVIL)"
        )

    print()


print("========================================")
print("TEST COMPLETE")
print("========================================")
print()

if sys.stdin.isatty():
    input("Press ENTER to exit...")

