import os
import struct
import sys
from pathlib import Path


BASE = Path(__file__).resolve().parent.parent
OUTPUT = Path(os.environ.get("GPSCLOCK_OUTPUT_DIR", BASE / "output"))
MARINE_FILE = OUTPUT / "marine.bin"


def point_in_polygon(
    lon,
    lat,
    points
):

    inside = False

    j = len(points) - 1

    for i in range(len(points)):

        xi, yi = points[i]
        xj, yj = points[j]

        intersects = (
            (
                (yi > lat)
                !=
                (yj > lat)
            )
            and
            (
                lon
                <
                (
                    (xj - xi)
                    * (lat - yi)
                    / (
                        (yj - yi)
                        if (yj - yi) != 0
                        else 1e-12
                    )
                    + xi
                )
            )
        )

        if intersects:
            inside = not inside

        j = i

    return inside


def read_database():

    records = []

    with open(
        MARINE_FILE,
        "rb"
    ) as f:

        magic = f.read(4)

        if magic != b"MRN1":
            raise RuntimeError(
                "Invalid marine.bin"
            )

        version = struct.unpack(
            "<I",
            f.read(4)
        )[0]

        record_count = struct.unpack(
            "<I",
            f.read(4)
        )[0]

        for _ in range(
            record_count
        ):

            (
                min_lon,
                min_lat,
                max_lon,
                max_lat
            ) = struct.unpack(
                "<ffff",
                f.read(16)
            )

            name_len = struct.unpack(
                "<H",
                f.read(2)
            )[0]

            name = f.read(
                name_len
            ).decode(
                "utf-8",
                errors="replace"
            )

            polygon_count = struct.unpack(
                "<H",
                f.read(2)
            )[0]

            polygons = []

            for _ in range(
                polygon_count
            ):

                point_count = struct.unpack(
                    "<I",
                    f.read(4)
                )[0]

                points = []

                for _ in range(
                    point_count
                ):

                    lon, lat = struct.unpack(
                        "<ff",
                        f.read(8)
                    )

                    points.append(
                        (
                            lon,
                            lat
                        )
                    )

                polygons.append(
                    points
                )

            records.append(
                {
                    "name": name,
                    "min_lon": min_lon,
                    "min_lat": min_lat,
                    "max_lon": max_lon,
                    "max_lat": max_lat,
                    "polygons": polygons
                }
            )

    return version, records


def lookup_marine(
    latitude,
    longitude,
    records
):

    for record in records:

        if (
            longitude
            <
            record["min_lon"]
            or
            longitude
            >
            record["max_lon"]
            or
            latitude
            <
            record["min_lat"]
            or
            latitude
            >
            record["max_lat"]
        ):
            continue

        for polygon in record[
            "polygons"
        ]:

            if point_in_polygon(
                longitude,
                latitude,
                polygon
            ):

                return record[
                    "name"
                ]

    return None


print()
print(
    "========================================"
)
print(
    "CGPS CLOCK MARINE DATABASE TEST"
)
print(
    "========================================"
)
print()

version, records = read_database()

print(
    "Database version:",
    version
)

print(
    "Marine areas:",
    len(records)
)

print()


tests = [

    (
        "Actual GPS position",
        36.471944,
        -121.734197
    ),

    (
        "Monterey Bay",
        36.70,
        -122.10
    ),

    (
        "Monterey Bay - inner bay",
        36.7000,
        -121.9000
    ),

    (
        "Monterey Bay - center",
        36.8000,
        -121.9500
    ),

    (
        "Monterey Bay - outer",
        36.7500,
        -122.0500
    ),

    (
        "Monterey Bay - north",
        36.9000,
        -121.9500
    ),

    (
        "Just offshore Monterey",
        36.60,
        -121.95
    ),

    (
        "Pacific west of Monterey",
        36.50,
        -125.00
    ),

    (
        "Central North Pacific",
        30.00,
        -150.00
    ),

    (
        "Central South Pacific",
        -20.00,
        -150.00
    ),

    (
        "North Atlantic",
        35.00,
        -40.00
    ),

    (
        "Mediterranean",
        36.00,
        18.00
    ),

    (
        "Gulf of Mexico",
        25.00,
        -90.00
    ),

    (
        "San Francisco Bay",
        37.80,
        -122.35
    ),
]


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

    marine_name = lookup_marine(
        latitude,
        longitude,
        records
    )

    print(label)

    print(
        "  GPS:",
        f"{latitude:.6f},",
        f"{longitude:.6f}"
    )

    print(
        "  Marine:",
        marine_name
        if marine_name
        else "NONE"
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

