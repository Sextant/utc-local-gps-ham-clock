import os
import sys
import struct
import math
import shapefile
from shapely.geometry import shape
from shapely.ops import unary_union

# ============================================================
# PATHS
# ============================================================

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SOURCE_DIR = os.path.join(
    BASE_DIR,
    "source",
    "marine"
)

OUTPUT_DIR = os.path.join(
    BASE_DIR,
    "output"
)

SHP_FILE = os.path.join(
    SOURCE_DIR,
    "ne_10m_geography_marine_polys.shp"
)

MARINE_BIN = os.path.join(
    OUTPUT_DIR,
    "marine.bin"
)

MARINE_INFO = os.path.join(
    OUTPUT_DIR,
    "marine_info.txt"
)

# ============================================================
# DATABASE FORMAT
# ============================================================
#
# marine.bin
#
# Header:
#   4 bytes   magic = b"MRN1"
#   uint32    version
#   uint32    record count
#
# Each record:
#   float     min longitude
#   float     min latitude
#   float     max longitude
#   float     max latitude
#   uint16    UTF-8 name length
#   bytes     UTF-8 name
#
#   uint16    polygon count
#
# For each polygon:
#   uint32    point count
#
#   Repeated:
#       float longitude
#       float latitude
#
# This intentionally keeps the first marine implementation
# straightforward and reliable. The Natural Earth marine
# dataset is small enough that we do not need the huge
# geographic index used by the place database.
#
# ============================================================


def clean_name(value):
    if value is None:
        return ""

    value = str(value).strip()

    if value.lower() in (
        "",
        "none",
        "null",
        "-99"
    ):
        return ""

    return value


def find_name(record_dict):

    # Natural Earth field names can vary slightly between
    # releases. Prefer English/name fields in this order.

    candidates = [
        "name",
        "NAME",
        "name_en",
        "NAME_EN",
        "name_long",
        "NAME_LONG",
        "label",
        "LABEL"
    ]

    for field in candidates:

        if field in record_dict:

            value = clean_name(
                record_dict[field]
            )

            if value:
                return value

    return ""


def geometry_to_polygons(geom):

    polygons = []

    if geom.is_empty:
        return polygons

    if geom.geom_type == "Polygon":

        polygons.append(
            list(geom.exterior.coords)
        )

    elif geom.geom_type == "MultiPolygon":

        for polygon in geom.geoms:

            if not polygon.is_empty:

                polygons.append(
                    list(
                        polygon.exterior.coords
                    )
                )

    elif geom.geom_type == "GeometryCollection":

        for part in geom.geoms:

            polygons.extend(
                geometry_to_polygons(part)
            )

    return polygons


def simplify_polygon(points):

    # Natural Earth 1:10m contains far more coastline detail
    # than the clock requires for naming a body of water.
    #
    # Convert the ring to a Shapely polygon and simplify it
    # slightly. 0.01 degree is roughly 1 km north/south and
    # retains substantially more geographic precision than
    # the clock needs for an ocean/sea/bay label.

    from shapely.geometry import Polygon

    if len(points) < 4:
        return []

    try:

        polygon = Polygon(points)

        if not polygon.is_valid:
            polygon = polygon.buffer(0)

        if polygon.is_empty:
            return []

        simplified = polygon.simplify(
            0.01,
            preserve_topology=True
        )

        if simplified.is_empty:
            return []

        if simplified.geom_type == "Polygon":

            return [
                list(
                    simplified.exterior.coords
                )
            ]

        if simplified.geom_type == "MultiPolygon":

            result = []

            for p in simplified.geoms:

                result.append(
                    list(
                        p.exterior.coords
                    )
                )

            return result

    except Exception:
        pass

    return [points]


def main():

    print()
    print(
        "========================================"
    )
    print(
        "CGPS CLOCK MARINE DATABASE BUILDER"
    )
    print(
        "========================================"
    )
    print()

    print("Looking for:")
    print(SHP_FILE)
    print()

    if not os.path.exists(SHP_FILE):

        print("ERROR:")
        print("Marine shapefile not found.")
        print()
        input("Press ENTER to exit...")
        sys.exit(1)

    os.makedirs(
        OUTPUT_DIR,
        exist_ok=True
    )

    print(
        "Loading Natural Earth marine areas..."
    )
    print()

    reader = shapefile.Reader(
        SHP_FILE,
        encoding="utf-8"
    )

    field_names = [
        field[0]
        for field in reader.fields[1:]
    ]

    print("Attribute fields:")
    print(", ".join(field_names))
    print()

    records = []

    unnamed = 0

    total_shapes = len(reader)

    print(
        f"Marine features found: {total_shapes}"
    )
    print()

    for index, shape_record in enumerate(
        reader.iterShapeRecords(),
        start=1
    ):

        record_dict = dict(
            zip(
                field_names,
                shape_record.record
            )
        )

        name = find_name(
            record_dict
        )

        if not name:

            unnamed += 1
            continue

        try:

            geom = shape(
                shape_record.shape.__geo_interface__
            )

        except Exception as exc:

            print(
                f"Skipping {name}: "
                f"geometry error: {exc}"
            )

            continue

        raw_polygons = geometry_to_polygons(
            geom
        )

        polygons = []

        for raw_polygon in raw_polygons:

            simplified_parts = simplify_polygon(
                raw_polygon
            )

            for part in simplified_parts:

                if len(part) >= 4:

                    polygons.append(
                        part
                    )

        if not polygons:
            continue

        min_lon = 180.0
        min_lat = 90.0
        max_lon = -180.0
        max_lat = -90.0

        for polygon in polygons:

            for lon, lat in polygon:

                min_lon = min(
                    min_lon,
                    lon
                )

                min_lat = min(
                    min_lat,
                    lat
                )

                max_lon = max(
                    max_lon,
                    lon
                )

                max_lat = max(
                    max_lat,
                    lat
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

        print(
            f"[{index:03d}/{total_shapes:03d}] "
            f"{name}"
        )

    print()
    print(
        f"Usable named marine areas: "
        f"{len(records)}"
    )

    if unnamed:

        print(
            f"Unnamed features skipped: {unnamed}"
        )

    # --------------------------------------------------------
    # Sort smaller geographic areas before larger ones.
    #
    # This is important because Monterey Bay can overlap a
    # much larger Pacific Ocean polygon. We want the more
    # specific body of water to win.
    # --------------------------------------------------------

    def bbox_area(record):

        width = (
            record["max_lon"] -
            record["min_lon"]
        )

        height = (
            record["max_lat"] -
            record["min_lat"]
        )

        return abs(
            width * height
        )

    records.sort(
        key=bbox_area
    )

    print()
    print("Writing marine.bin...")

    total_points = 0

    with open(
        MARINE_BIN,
        "wb"
    ) as output:

        output.write(
            b"MRN1"
        )

        output.write(
            struct.pack(
                "<I",
                1
            )
        )

        output.write(
            struct.pack(
                "<I",
                len(records)
            )
        )

        for record in records:

            output.write(
                struct.pack(
                    "<ffff",
                    float(
                        record["min_lon"]
                    ),
                    float(
                        record["min_lat"]
                    ),
                    float(
                        record["max_lon"]
                    ),
                    float(
                        record["max_lat"]
                    )
                )
            )

            name_bytes = (
                record["name"]
                .encode(
                    "utf-8",
                    errors="replace"
                )
            )

            if len(name_bytes) > 65535:

                name_bytes = (
                    name_bytes[:65535]
                )

            output.write(
                struct.pack(
                    "<H",
                    len(name_bytes)
                )
            )

            output.write(
                name_bytes
            )

            polygons = record[
                "polygons"
            ]

            output.write(
                struct.pack(
                    "<H",
                    len(polygons)
                )
            )

            for polygon in polygons:

                output.write(
                    struct.pack(
                        "<I",
                        len(polygon)
                    )
                )

                total_points += len(
                    polygon
                )

                for lon, lat in polygon:

                    output.write(
                        struct.pack(
                            "<ff",
                            float(lon),
                            float(lat)
                        )
                    )

    # --------------------------------------------------------
    # Human-readable information file
    # --------------------------------------------------------

    with open(
        MARINE_INFO,
        "w",
        encoding="utf-8"
    ) as info:

        info.write(
            "CGPS CLOCK MARINE DATABASE\n"
        )

        info.write(
            "==========================\n\n"
        )

        info.write(
            "Source: Natural Earth "
            "1:10m Geography Marine Polygons\n"
        )

        info.write(
            "Database version: 1\n"
        )

        info.write(
            f"Marine areas: {len(records)}\n"
        )

        info.write(
            f"Polygon points: {total_points}\n"
        )

        info.write(
            "Simplification: 0.01 degrees\n\n"
        )

        info.write(
            "Areas in lookup order:\n"
        )

        for record in records:

            info.write(
                record["name"] +
                "\n"
            )

    marine_size = os.path.getsize(
        MARINE_BIN
    )

    print()
    print(
        "========================================"
    )
    print(
        "MARINE DATABASE BUILD COMPLETE"
    )
    print(
        "========================================"
    )
    print()

    print(
        f"Marine areas: {len(records):,}"
    )

    print(
        f"Polygon points: {total_points:,}"
    )

    print(
        f"marine.bin: {marine_size:,} bytes"
    )

    print()
    print("Created:")
    print(MARINE_BIN)
    print(MARINE_INFO)
    print()

    input(
        "Press ENTER to exit..."
    )


if __name__ == "__main__":
    main()

