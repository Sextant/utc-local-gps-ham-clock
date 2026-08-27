from pathlib import Path
import math
import os
import struct
import sys


# ============================================================
# PATHS
# ============================================================

BASE = Path(__file__).resolve().parent.parent
OUTPUT = Path(os.environ.get("GPSCLOCK_OUTPUT_DIR", BASE / "output"))

PLACES_FILE = OUTPUT / "places.bin"
INDEX_FILE = OUTPUT / "places_index.bin"


# ============================================================
# CONSTANTS
# ============================================================

EARTH_RADIUS_KM = 6371.0088

PLACES_HEADER_SIZE = 10
INDEX_HEADER_SIZE = 18


# ============================================================
# HAVERSINE DISTANCE
# ============================================================

def distance_km(
    lat1,
    lon1,
    lat2,
    lon2
):

    lat1 = math.radians(lat1)
    lon1 = math.radians(lon1)

    lat2 = math.radians(lat2)
    lon2 = math.radians(lon2)

    dlat = lat2 - lat1
    dlon = lon2 - lon1

    a = (
        math.sin(dlat / 2.0) ** 2
        +
        math.cos(lat1)
        *
        math.cos(lat2)
        *
        math.sin(dlon / 2.0) ** 2
    )

    c = 2.0 * math.atan2(
        math.sqrt(a),
        math.sqrt(1.0 - a)
    )

    return (
        EARTH_RADIUS_KM * c
    )


# ============================================================
# LOAD INDEX HEADER
# ============================================================

def load_index_header():

    with open(
        INDEX_FILE,
        "rb"
    ) as f:

        magic = f.read(4)

        if magic != b"PIDX":
            raise RuntimeError(
                "Invalid places_index.bin"
            )

        version = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        cell_degrees = struct.unpack(
            "<f",
            f.read(4)
        )[0]

        grid_cols, grid_rows = struct.unpack(
            "<HH",
            f.read(4)
        )

        cell_count = struct.unpack(
            "<I",
            f.read(4)
        )[0]

    return {
        "version": version,
        "cell_degrees": cell_degrees,
        "grid_cols": grid_cols,
        "grid_rows": grid_rows,
        "cell_count": cell_count
    }


# ============================================================
# VALIDATE PLACES FILE
# ============================================================

def validate_places_file():

    with open(
        PLACES_FILE,
        "rb"
    ) as f:

        magic = f.read(4)

        if magic != b"PLAC":
            raise RuntimeError(
                "Invalid places.bin"
            )

        version = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        record_count = struct.unpack(
            "<I",
            f.read(4)
        )[0]

    return (
        version,
        record_count
    )


# ============================================================
# GET CELL NUMBER
# ============================================================

def get_cell(
    latitude,
    longitude,
    info
):

    cell_degrees = info[
        "cell_degrees"
    ]

    col = int(
        (longitude + 180.0)
        / cell_degrees
    )

    row = int(
        (latitude + 90.0)
        / cell_degrees
    )

    col = max(
        0,
        min(
            info["grid_cols"] - 1,
            col
        )
    )

    row = max(
        0,
        min(
            info["grid_rows"] - 1,
            row
        )
    )

    return row, col


# ============================================================
# READ INDEX CELL
# ============================================================

def read_index_cell(
    index_file,
    row,
    col,
    info
):

    if (
        row < 0
        or row >= info["grid_rows"]
        or col < 0
        or col >= info["grid_cols"]
    ):

        return 0, 0

    cell_number = (
        row
        * info["grid_cols"]
        + col
    )

    offset = (
        INDEX_HEADER_SIZE
        + cell_number * 8
    )

    index_file.seek(
        offset
    )

    raw = index_file.read(
        8
    )

    if len(raw) != 8:

        raise RuntimeError(
            "Truncated places index."
        )

    place_offset, count = struct.unpack(
        "<II",
        raw
    )

    return (
        place_offset,
        count
    )


# ============================================================
# READ ONE VARIABLE-LENGTH PLACE RECORD
# ============================================================

def read_place_record(
    places_file
):

    fixed = places_file.read(
        16
    )

    if len(fixed) != 16:
        return None

    (
        lat_scaled,
        lon_scaled,
        population,
        geoname_id
    ) = struct.unpack(
        "<iiII",
        fixed
    )


    country_length_raw = (
        places_file.read(1)
    )

    if not country_length_raw:
        return None

    country_length = (
        country_length_raw[0]
    )

    country = places_file.read(
        country_length
    ).decode(
        "ascii",
        errors="replace"
    )


    feature_length_raw = (
        places_file.read(1)
    )

    if not feature_length_raw:
        return None

    feature_length = (
        feature_length_raw[0]
    )

    feature = places_file.read(
        feature_length
    ).decode(
        "ascii",
        errors="replace"
    )


    name_length_raw = (
        places_file.read(2)
    )

    if len(name_length_raw) != 2:
        return None

    name_length = struct.unpack(
        "<H",
        name_length_raw
    )[0]

    name = places_file.read(
        name_length
    ).decode(
        "utf-8",
        errors="replace"
    )


    return {
        "latitude":
            lat_scaled / 1_000_000.0,

        "longitude":
            lon_scaled / 1_000_000.0,

        "population":
            population,

        "geoname_id":
            geoname_id,

        "country":
            country,

        "feature":
            feature,

        "name":
            name
    }


# ============================================================
# SEARCH ONE CELL
# ============================================================

def search_cell(
    target_lat,
    target_lon,
    row,
    col,
    info,
    index_file,
    places_file,
    current_best,
    current_best_distance
):

    place_offset, count = (
        read_index_cell(
            index_file,
            row,
            col,
            info
        )
    )

    if count == 0:

        return (
            current_best,
            current_best_distance,
            0
        )


    places_file.seek(
        place_offset
    )


    checked = 0


    for _ in range(
        count
    ):

        place = read_place_record(
            places_file
        )

        if place is None:

            raise RuntimeError(
                "Truncated place record."
            )


        checked += 1


        distance = distance_km(
            target_lat,
            target_lon,
            place["latitude"],
            place["longitude"]
        )


        if (
            current_best is None
            or distance
            < current_best_distance
        ):

            current_best = place

            current_best_distance = (
                distance
            )


        # If essentially identical coordinates,
        # population becomes a useful tie breaker.

        elif (
            abs(
                distance
                - current_best_distance
            )
            < 0.001
            and
            place["population"]
            >
            current_best["population"]
        ):

            current_best = place

            current_best_distance = (
                distance
            )


    return (
        current_best,
        current_best_distance,
        checked
    )


# ============================================================
# NEAREST PLACE LOOKUP
# ============================================================

def find_nearest_place(
    latitude,
    longitude,
    info,
    maximum_rings=20
):

    center_row, center_col = (
        get_cell(
            latitude,
            longitude,
            info
        )
    )


    best_place = None
    best_distance = float(
        "inf"
    )

    total_checked = 0
    cells_checked = 0


    with open(
        INDEX_FILE,
        "rb"
    ) as index_file, open(
        PLACES_FILE,
        "rb"
    ) as places_file:


        for ring in range(
            maximum_rings + 1
        ):


            # -----------------------------------------------
            # BUILD THIS SEARCH RING
            # -----------------------------------------------

            cells = []


            if ring == 0:

                cells.append(
                    (
                        center_row,
                        center_col
                    )
                )

            else:

                row_min = (
                    center_row
                    - ring
                )

                row_max = (
                    center_row
                    + ring
                )

                col_min = (
                    center_col
                    - ring
                )

                col_max = (
                    center_col
                    + ring
                )


                # Top and bottom
                for col in range(
                    col_min,
                    col_max + 1
                ):

                    cells.append(
                        (
                            row_min,
                            col
                        )
                    )

                    cells.append(
                        (
                            row_max,
                            col
                        )
                    )


                # Left and right sides,
                # excluding corners already added.
                for row in range(
                    row_min + 1,
                    row_max
                ):

                    cells.append(
                        (
                            row,
                            col_min
                        )
                    )

                    cells.append(
                        (
                            row,
                            col_max
                        )
                    )


            # -----------------------------------------------
            # SEARCH RING
            # -----------------------------------------------

            for row, col in cells:

                if (
                    row < 0
                    or row >= info[
                        "grid_rows"
                    ]
                ):
                    continue


                # Wrap longitude around the globe.
                col = (
                    col
                    % info["grid_cols"]
                )


                (
                    best_place,
                    best_distance,
                    checked
                ) = search_cell(
                    latitude,
                    longitude,
                    row,
                    col,
                    info,
                    index_file,
                    places_file,
                    best_place,
                    best_distance
                )


                total_checked += checked
                cells_checked += 1


            # -----------------------------------------------
            # DETERMINE WHETHER WE CAN STOP
            # -----------------------------------------------

            if best_place is not None:

                #
                # Each additional ring is approximately
                # CELL_DEGREES farther away.
                #
                # Use a conservative north/south distance
                # to determine when an unsearched ring
                # cannot possibly contain a closer place.
                #

                minimum_unsearched_km = (
                    ring
                    * info[
                        "cell_degrees"
                    ]
                    * 111.0
                )


                if (
                    ring >= 2
                    and
                    best_distance
                    <
                    minimum_unsearched_km
                ):

                    return {
                        "place":
                            best_place,

                        "distance_km":
                            best_distance,

                        "rings":
                            ring,

                        "cells_checked":
                            cells_checked,

                        "places_checked":
                            total_checked
                    }


    return {
        "place":
            best_place,

        "distance_km":
            best_distance,

        "rings":
            maximum_rings,

        "cells_checked":
            cells_checked,

        "places_checked":
            total_checked
    }


# ============================================================
# TEST LOCATIONS
# ============================================================

tests = [

    (
        "Actual GPS position",
        36.471944,
        -121.734197
    ),

    (
        "Carmel-by-the-Sea",
        36.5552,
        -121.9233
    ),

    (
        "Monterey",
        36.6002,
        -121.8947
    ),

    (
        "Boise",
        43.6150,
        -116.2023
    ),

    (
        "New York City",
        40.7128,
        -74.0060
    ),

    (
        "London",
        51.5074,
        -0.1278
    ),

    (
        "Tokyo",
        35.6762,
        139.6503
    ),

    (
        "Kathmandu",
        27.7172,
        85.3240
    ),

    (
        "Sydney",
        -33.8688,
        151.2093
    ),

    # Offshore tests

    (
        "Monterey Bay offshore",
        36.70,
        -122.10
    ),

    (
        "Pacific Ocean west of Monterey",
        36.50,
        -125.00
    ),

    (
        "Central Pacific",
        20.00,
        -150.00
    ),
]


# ============================================================
# MAIN
# ============================================================

print()
print(
    "========================================"
)

print(
    "CGPS CLOCK PLACE DATABASE TEST"
)

print(
    "========================================"
)

print()


info = load_index_header()

places_version, record_count = (
    validate_places_file()
)


print(
    "Places database version:",
    places_version
)

print(
    "Records:",
    f"{record_count:,}"
)

print(
    "Index cell size:",
    info["cell_degrees"],
    "degrees"
)

print(
    "Grid:",
    info["grid_cols"],
    "x",
    info["grid_rows"]
)

print()


print(
    "TEST RESULTS"
)

print(
    "----------------------------------------"
)

print()


for (
    label,
    latitude,
    longitude
) in tests:


    result = find_nearest_place(
        latitude,
        longitude,
        info
    )


    print(label)

    print(
        "  GPS:",
        f"{latitude:.6f},",
        f"{longitude:.6f}"
    )


    place = result[
        "place"
    ]


    if place is None:

        print(
            "  Result: NO PLACE FOUND"
        )

        print()

        continue


    print(
        "  Nearest:",
        place["name"]
    )

    print(
        "  Country:",
        place["country"]
    )

    print(
        "  Feature:",
        place["feature"]
    )

    print(
        "  Population:",
        f"{place['population']:,}"
    )

    print(
        "  GeoNames ID:",
        place["geoname_id"]
    )

    print(
        "  Place coordinates:",
        f"{place['latitude']:.6f},",
        f"{place['longitude']:.6f}"
    )

    print(
        "  Distance:",
        f"{result['distance_km']:.3f}",
        "km"
    )

    print(
        "  Search rings:",
        result["rings"]
    )

    print(
        "  Cells checked:",
        result["cells_checked"]
    )

    print(
        "  Place records checked:",
        result["places_checked"]
    )

    print()


print(
    "========================================"
)

print(
    "TEST COMPLETE"
)

print(
    "========================================"
)

print()

if sys.stdin.isatty():
    input(
        "Press ENTER to exit..."
    )

