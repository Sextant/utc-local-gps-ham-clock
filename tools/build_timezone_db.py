import math
import struct
from pathlib import Path

import numpy as np
import orjson

from shapely import contains_xy
from shapely.geometry import box, Point, shape
from shapely.strtree import STRtree


# ============================================================
# PATHS
# ============================================================

BASE = Path(__file__).resolve().parent.parent

SOURCE = BASE / "source" / "combined-now.json"
OUTPUT = BASE / "output"

ZONES_FILE   = OUTPUT / "zones.bin"
INDEX_FILE   = OUTPUT / "index.bin"
TILES_FILE   = OUTPUT / "tiles.bin"
VERSION_FILE = OUTPUT / "version.txt"


# ============================================================
# DATABASE RESOLUTION
# ============================================================

# Coarse global grid.
COARSE_DEG = 1.0

# Fine resolution used only in timezone-boundary cells.
FINE_DEG = 0.02

FINE_PER_COARSE = int(
    round(COARSE_DEG / FINE_DEG)
)

COARSE_COLS = int(
    360 / COARSE_DEG
)

COARSE_ROWS = int(
    180 / COARSE_DEG
)

# Zone IDs are one byte.
UNKNOWN_ZONE = 255

# Coarse record types.
CELL_DIRECT  = 0
CELL_TILE    = 1
CELL_UNKNOWN = 2


# ============================================================
# START
# ============================================================

print()
print("========================================")
print("CGPS CLOCK TIMEZONE DATABASE BUILDER V2")
print("========================================")
print()

print("Source:")
print(SOURCE)
print()

if not SOURCE.exists():

    print("*** ERROR ***")
    print("combined-now.json was not found.")
    print()

    input("Press ENTER to exit...")

    raise SystemExit


OUTPUT.mkdir(
    parents=True,
    exist_ok=True
)


# ============================================================
# LOAD TIMEZONE BOUNDARIES
# ============================================================

print("Loading worldwide timezone boundary data...")

with open(
    SOURCE,
    "rb"
) as f:

    data = orjson.loads(
        f.read()
    )


features = data["features"]

print(
    "Features found:",
    len(features)
)


# ============================================================
# BUILD GEOMETRY LIST
# ============================================================

timezone_names = []
geometries = []

print()
print("Processing timezone geometries...")


for feature in features:

    properties = feature.get(
        "properties",
        {}
    )

    tzid = properties.get(
        "tzid",
        ""
    )

    if not tzid:
        continue

    geom = shape(
        feature["geometry"]
    )

    timezone_names.append(
        tzid
    )

    geometries.append(
        geom
    )


print(
    "Usable timezone geometries:",
    len(geometries)
)


# ============================================================
# CREATE ZONE ID TABLE
# ============================================================

unique_zones = sorted(
    set(timezone_names)
)

if len(unique_zones) >= UNKNOWN_ZONE:

    raise RuntimeError(
        "Too many timezone IDs for one-byte zone IDs."
    )


zone_id_map = {
    name: i
    for i, name in enumerate(unique_zones)
}


geometry_zone_ids = np.array(
    [
        zone_id_map[name]
        for name in timezone_names
    ],
    dtype=np.uint8
)


print(
    "Unique timezone IDs:",
    len(unique_zones)
)


# ============================================================
# WRITE ZONES.BIN
# ============================================================

print()
print("Writing zones.bin...")


with open(
    ZONES_FILE,
    "wb"
) as f:

    # Magic
    f.write(
        b"TZON"
    )

    # Version
    f.write(
        struct.pack(
            "<H",
            1
        )
    )

    # Number of zones
    f.write(
        struct.pack(
            "<H",
            len(unique_zones)
        )
    )

    for name in unique_zones:

        encoded = name.encode(
            "utf-8"
        )

        if len(encoded) > 255:

            raise RuntimeError(
                "Timezone name too long: "
                + name
            )

        f.write(
            struct.pack(
                "<B",
                len(encoded)
            )
        )

        f.write(
            encoded
        )


# ============================================================
# SPATIAL TREE
# ============================================================

print(
    "Building polygon search tree..."
)

tree = STRtree(
    geometries
)


# ============================================================
# HELPERS
# ============================================================

def find_zone_for_single_point(
    lon,
    lat,
    candidate_indices
):

    """
    Slow fallback for points that were not resolved
    by vectorized contains_xy().
    """

    p = Point(
        lon,
        lat
    )

    for index in candidate_indices:

        if geometries[index].covers(p):

            return int(
                geometry_zone_ids[index]
            )

    return UNKNOWN_ZONE


def build_fine_tile(
    lon0,
    lat0,
    candidate_indices
):

    """
    Build one 1-degree tile at FINE_DEG resolution.

    Returns a flat uint8 array of zone IDs.
    """

    n = FINE_PER_COARSE


    # Cell-center longitude coordinates
    lon_values = (
        lon0
        + (
            np.arange(n)
            + 0.5
        )
        * FINE_DEG
    )


    # Cell-center latitude coordinates
    lat_values = (
        lat0
        + (
            np.arange(n)
            + 0.5
        )
        * FINE_DEG
    )


    # Create full point grid
    xs, ys = np.meshgrid(
        lon_values,
        lat_values
    )


    xs_flat = xs.ravel()
    ys_flat = ys.ravel()


    zone_values = np.full(
        xs_flat.shape,
        UNKNOWN_ZONE,
        dtype=np.uint8
    )


    # Test each candidate polygon against all points.
    for geom_index in candidate_indices:

        mask = contains_xy(
            geometries[geom_index],
            xs_flat,
            ys_flat
        )

        unresolved = (
            zone_values
            == UNKNOWN_ZONE
        )

        assign_mask = (
            mask
            & unresolved
        )

        zone_values[
            assign_mask
        ] = geometry_zone_ids[
            geom_index
        ]


    # Rare boundary/gap fallback.
    unresolved_indices = np.where(
        zone_values
        == UNKNOWN_ZONE
    )[0]


    if len(unresolved_indices) > 0:

        for point_index in unresolved_indices:

            zone_values[
                point_index
            ] = find_zone_for_single_point(
                float(
                    xs_flat[
                        point_index
                    ]
                ),
                float(
                    ys_flat[
                        point_index
                    ]
                ),
                candidate_indices
            )


    return zone_values


# ============================================================
# BUILD GLOBAL INDEX
# ============================================================

print()
print("Building worldwide two-level grid...")
print()

print(
    "Coarse grid:",
    COARSE_COLS,
    "x",
    COARSE_ROWS
)

print(
    "Fine boundary resolution:",
    FINE_DEG,
    "degrees"
)

print(
    "Fine tile:",
    FINE_PER_COARSE,
    "x",
    FINE_PER_COARSE,
    "cells"
)

print()


# Each coarse entry:
#
# byte 0:
#   0 = direct zone
#   1 = fine tile
#   2 = unknown
#
# uint32 value:
#   direct = zone ID
#   tile   = tile number
#
# Record size = 5 bytes.

coarse_records = []

fine_tiles = []

direct_cells = 0
boundary_cells = 0
unknown_cells = 0


for row in range(
    COARSE_ROWS
):

    lat0 = (
        -90.0
        + row
        * COARSE_DEG
    )

    lat1 = (
        lat0
        + COARSE_DEG
    )


    # Progress every 10 latitude rows.
    if row % 10 == 0:

        print(
            f"Latitude row "
            f"{row:3d} / "
            f"{COARSE_ROWS}"
        )


    for col in range(
        COARSE_COLS
    ):

        lon0 = (
            -180.0
            + col
            * COARSE_DEG
        )

        lon1 = (
            lon0
            + COARSE_DEG
        )


        coarse_box = box(
            lon0,
            lat0,
            lon1,
            lat1
        )


        candidate_indices = tree.query(
            coarse_box,
            predicate="intersects"
        )


        candidate_indices = [
            int(i)
            for i in candidate_indices
        ]


        # ----------------------------------------------------
        # NO TIMEZONE FOUND
        # ----------------------------------------------------

        if not candidate_indices:

            coarse_records.append(
                (
                    CELL_UNKNOWN,
                    0
                )
            )

            unknown_cells += 1

            continue


        # ----------------------------------------------------
        # TEST FOR SIMPLE DIRECT CELL
        # ----------------------------------------------------

        direct_zone = None


        if len(candidate_indices) == 1:

            idx = candidate_indices[0]

            if geometries[idx].covers(
                coarse_box
            ):

                direct_zone = int(
                    geometry_zone_ids[idx]
                )


        if direct_zone is not None:

            coarse_records.append(
                (
                    CELL_DIRECT,
                    direct_zone
                )
            )

            direct_cells += 1

            continue


        # ----------------------------------------------------
        # BOUNDARY CELL
        # ----------------------------------------------------

        tile_number = len(
            fine_tiles
        )


        tile = build_fine_tile(
            lon0,
            lat0,
            candidate_indices
        )


        # Optimization:
        # after rasterization, a supposedly complex cell may
        # actually resolve entirely to one timezone.

        valid_values = tile[
            tile != UNKNOWN_ZONE
        ]


        if len(valid_values) > 0:

            unique_tile_values = np.unique(
                valid_values
            )

        else:

            unique_tile_values = np.array(
                [],
                dtype=np.uint8
            )


        # Every fine cell is same zone.
        if (
            len(unique_tile_values) == 1
            and
            np.all(
                tile
                == unique_tile_values[0]
            )
        ):

            coarse_records.append(
                (
                    CELL_DIRECT,
                    int(
                        unique_tile_values[0]
                    )
                )
            )

            direct_cells += 1

            continue


        # Actual fine tile required.
        fine_tiles.append(
            tile
        )

        coarse_records.append(
            (
                CELL_TILE,
                tile_number
            )
        )

        boundary_cells += 1


# ============================================================
# WRITE INDEX.BIN
# ============================================================

print()
print("Writing index.bin...")


with open(
    INDEX_FILE,
    "wb"
) as f:

    # Magic
    f.write(
        b"TZIX"
    )

    # Database version
    f.write(
        struct.pack(
            "<H",
            1
        )
    )

    # Grid geometry
    f.write(
        struct.pack(
            "<HHffH",
            COARSE_COLS,
            COARSE_ROWS,
            COARSE_DEG,
            FINE_DEG,
            FINE_PER_COARSE
        )
    )


    # Number of coarse records
    f.write(
        struct.pack(
            "<I",
            len(coarse_records)
        )
    )


    for cell_type, value in coarse_records:

        f.write(
            struct.pack(
                "<BI",
                cell_type,
                value
            )
        )


# ============================================================
# WRITE TILES.BIN
# ============================================================

print(
    "Writing tiles.bin..."
)


with open(
    TILES_FILE,
    "wb"
) as f:

    # Magic
    f.write(
        b"TZTL"
    )

    # Version
    f.write(
        struct.pack(
            "<H",
            1
        )
    )

    # Tile dimensions
    f.write(
        struct.pack(
            "<HH",
            FINE_PER_COARSE,
            FINE_PER_COARSE
        )
    )

    # Number of tiles
    f.write(
        struct.pack(
            "<I",
            len(fine_tiles)
        )
    )


    for tile in fine_tiles:

        f.write(
            tile.tobytes(
                order="C"
            )
        )


# ============================================================
# VERSION FILE
# ============================================================

with open(
    VERSION_FILE,
    "w",
    encoding="utf-8"
) as f:

    f.write(
        "CGPS Clock Offline Timezone Database\n"
    )

    f.write(
        "Format version: 1\n"
    )

    f.write(
        "Source: Timezone Boundary Builder combined-now\n"
    )

    f.write(
        f"Coarse resolution: {COARSE_DEG} degree\n"
    )

    f.write(
        f"Boundary resolution: {FINE_DEG} degree\n"
    )

    f.write(
        f"Timezone count: {len(unique_zones)}\n"
    )

    f.write(
        f"Fine tiles: {len(fine_tiles)}\n"
    )


# ============================================================
# SUMMARY
# ============================================================

print()
print("========================================")
print("DATABASE BUILD COMPLETE")
print("========================================")
print()

print(
    "Direct coarse cells:",
    direct_cells
)

print(
    "Boundary fine cells:",
    boundary_cells
)

print(
    "Unknown cells:",
    unknown_cells
)

print(
    "Fine tiles:",
    len(fine_tiles)
)

print()

print(
    "zones.bin:",
    ZONES_FILE.stat().st_size,
    "bytes"
)

print(
    "index.bin:",
    INDEX_FILE.stat().st_size,
    "bytes"
)

print(
    "tiles.bin:",
    TILES_FILE.stat().st_size,
    "bytes"
)

print()

print(
    "Output directory:"
)

print(
    OUTPUT
)

print()

input(
    "Press ENTER to exit..."
)

