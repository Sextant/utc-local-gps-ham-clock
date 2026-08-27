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

INDEX_HEADER_SIZE = 18

# If no good populated place is within this distance,
# treat the location as maritime / remote.
MAX_PLACE_DISTANCE_KM = 10.0


# ============================================================
# FEATURE PRIORITY
#
# Lower number = better choice.
#
# Prefer recognized cities/towns/admin seats over tiny
# neighborhood/subdivision records.
# ============================================================

FEATURE_PRIORITY = {

    "PPLC": 0,   # national capital
    "PPLA": 1,   # first-order admin seat
    "PPLA2": 2,
    "PPLA3": 3,
    "PPLA4": 4,

    "PPL": 5,    # populated place

    "PPLG": 6,
    "PPLS": 6,

    "PPLL": 8,
    "PPLF": 8,

    "PPLX": 12,  # section / neighborhood
}


def feature_priority(code):

    return FEATURE_PRIORITY.get(
        code,
        10
    )


# ============================================================
# HAVERSINE
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

    return EARTH_RADIUS_KM * c


# ============================================================
# LOAD INDEX HEADER
# ============================================================

def load_index_header():

    with open(INDEX_FILE, "rb") as f:

        if f.read(4) != b"PIDX":
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
        row * info["grid_cols"]
        + col
    )

    offset = (
        INDEX_HEADER_SIZE
        + cell_number * 8
    )

    index_file.seek(offset)

    raw = index_file.read(8)

    if len(raw) != 8:
        raise RuntimeError(
            "Truncated index."
        )

    return struct.unpack(
        "<II",
        raw
    )


# ============================================================
# READ PLACE RECORD
# ============================================================

def read_place_record(f):

    fixed = f.read(16)

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

    country_len_raw = f.read(1)

    if not country_len_raw:
        return None

    country_len = country_len_raw[0]

    country = f.read(
        country_len
    ).decode(
        "ascii",
        errors="replace"
    )

    feature_len_raw = f.read(1)

    if not feature_len_raw:
        return None

    feature_len = feature_len_raw[0]

    feature = f.read(
        feature_len
    ).decode(
        "ascii",
        errors="replace"
    )

    name_len_raw = f.read(2)

    if len(name_len_raw) != 2:
        return None

    name_len = struct.unpack(
        "<H",
        name_len_raw
    )[0]

    name = f.read(
        name_len
    ).decode(
        "utf-8",
        errors="replace"
    )

    return {
        "latitude": lat_scaled / 1_000_000.0,
        "longitude": lon_scaled / 1_000_000.0,
        "population": population,
        "geoname_id": geoname_id,
        "country": country,
        "feature": feature,
        "name": name
    }


# ============================================================
# CENTER CELL
# ============================================================

def get_cell(
    latitude,
    longitude,
    info
):

    col = int(
        (longitude + 180.0)
        / info["cell_degrees"]
    )

    row = int(
        (latitude + 90.0)
        / info["cell_degrees"]
    )

    return row, col


# ============================================================
# RANKING SCORE
#
# Distance still matters most locally, but feature significance
# and population can prevent tiny neighborhood names from
# beating recognized towns/cities.
# ============================================================

def place_score(
    place,
    distance
):

    priority = feature_priority(
        place["feature"]
    )

    population = max(
        place["population"],
        1
    )

    # Mild population benefit only.
    population_bonus = math.log10(
        population
    )

    return (
        distance
        +
        priority * 0.75
        -
        population_bonus * 0.15
    )


# ============================================================
# RANKED LOOKUP
# ============================================================

def find_ranked_place(
    latitude,
    longitude,
    info,
    max_rings=6
):

    center_row, center_col = get_cell(
        latitude,
        longitude,
        info
    )

    candidates = []

    with open(
        INDEX_FILE,
        "rb"
    ) as index_file, open(
        PLACES_FILE,
        "rb"
    ) as places_file:

        for ring in range(
            max_rings + 1
        ):

            row_min = center_row - ring
            row_max = center_row + ring

            col_min = center_col - ring
            col_max = center_col + ring

            cells = set()

            if ring == 0:

                cells.add(
                    (
                        center_row,
                        center_col
                    )
                )

            else:

                for col in range(
                    col_min,
                    col_max + 1
                ):

                    cells.add(
                        (
                            row_min,
                            col
                        )
                    )

                    cells.add(
                        (
                            row_max,
                            col
                        )
                    )

                for row in range(
                    row_min,
                    row_max + 1
                ):

                    cells.add(
                        (
                            row,
                            col_min
                        )
                    )

                    cells.add(
                        (
                            row,
                            col_max
                        )
                    )

            for row, col in cells:

                if (
                    row < 0
                    or row >= info[
                        "grid_rows"
                    ]
                ):
                    continue

                col = (
                    col
                    % info["grid_cols"]
                )

                offset, count = read_index_cell(
                    index_file,
                    row,
                    col,
                    info
                )

                if count == 0:
                    continue

                places_file.seek(
                    offset
                )

                for _ in range(
                    count
                ):

                    place = read_place_record(
                        places_file
                    )

                    if place is None:
                        break

                    distance = distance_km(
                        latitude,
                        longitude,
                        place["latitude"],
                        place["longitude"]
                    )

                    # We only care about plausible nearby
                    # locality candidates.
                    if (
                        distance
                        <= MAX_PLACE_DISTANCE_KM
                    ):

                        score = place_score(
                            place,
                            distance
                        )

                        candidates.append(
                            (
                                score,
                                distance,
                                place
                            )
                        )

    if not candidates:

        return None

    candidates.sort(
        key=lambda x: (
            x[0],
            x[1],
            -x[2]["population"]
        )
    )

    return candidates[0]


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
]


# ============================================================
# MAIN
# ============================================================

print()
print(
    "========================================"
)

print(
    "CGPS CLOCK RANKED PLACE TEST"
)

print(
    "========================================"
)

print()

info = load_index_header()

print(
    "Maximum locality distance:",
    MAX_PLACE_DISTANCE_KM,
    "km"
)

print()

for (
    label,
    latitude,
    longitude
) in tests:

    result = find_ranked_place(
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

    if result is None:

        print(
            "  Result: NO LOCALITY / MARITIME"
        )

        print()

        continue

    score, distance, place = result

    print(
        "  Selected:",
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
        "  Distance:",
        f"{distance:.3f}",
        "km"
    )

    print(
        "  Score:",
        f"{score:.3f}"
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

