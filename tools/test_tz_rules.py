from datetime import datetime, timezone
import os
from pathlib import Path
import struct
import sys


BASE = Path(__file__).resolve().parent.parent
OUTPUT = Path(os.environ.get("GPSCLOCK_OUTPUT_DIR", BASE / "output"))

ZONES_FILE = OUTPUT / "zones.bin"
RULES_FILE = OUTPUT / "rules.bin"


def load_zones():

    zones = []

    with open(ZONES_FILE, "rb") as f:

        if f.read(4) != b"TZON":
            raise RuntimeError("Invalid zones.bin")

        version = struct.unpack("<H", f.read(2))[0]
        zone_count = struct.unpack("<H", f.read(2))[0]

        for _ in range(zone_count):

            length = struct.unpack("<B", f.read(1))[0]
            name = f.read(length).decode("utf-8")

            zones.append(name)

    return zones


def load_rules():

    with open(RULES_FILE, "rb") as f:

        if f.read(4) != b"TZRL":
            raise RuntimeError("Invalid rules.bin")

        version = struct.unpack("<H", f.read(2))[0]

        start_year, end_year = struct.unpack(
            "<HH",
            f.read(4)
        )

        zone_count = struct.unpack(
            "<H",
            f.read(2)
        )[0]

        abbreviation_count = struct.unpack(
            "<B",
            f.read(1)
        )[0]

        abbreviations = []

        for _ in range(abbreviation_count):

            length = struct.unpack(
                "<B",
                f.read(1)
            )[0]

            abbreviation = f.read(
                length
            ).decode("ascii")

            abbreviations.append(
                abbreviation
            )

        zone_rules = []

        for _ in range(zone_count):

            transition_count = struct.unpack(
                "<H",
                f.read(2)
            )[0]

            transitions = []

            for _ in range(transition_count):

                epoch, offset_seconds, abbreviation_id = struct.unpack(
                    "<qiB",
                    f.read(13)
                )

                transitions.append(
                    (
                        epoch,
                        offset_seconds,
                        abbreviations[
                            abbreviation_id
                        ]
                    )
                )

            zone_rules.append(
                transitions
            )

    return (
        start_year,
        end_year,
        zone_rules
    )


def state_for_epoch(
    epoch,
    transitions
):

    current = transitions[0]

    for transition in transitions:

        if transition[0] <= epoch:
            current = transition

        else:
            break

    return current


def format_offset(
    seconds
):

    sign = "+" if seconds >= 0 else "-"

    seconds = abs(seconds)

    hours = seconds // 3600

    minutes = (
        seconds % 3600
    ) // 60

    if minutes == 0:
        return f"UTC{sign}{hours}"

    return f"UTC{sign}{hours}:{minutes:02d}"


def test_zone(
    zone_name,
    utc_datetime,
    zones,
    zone_rules
):

    zone_id = zones.index(
        zone_name
    )

    epoch = int(
        utc_datetime.timestamp()
    )

    transition_epoch, offset_seconds, abbreviation = state_for_epoch(
        epoch,
        zone_rules[zone_id]
    )

    local_epoch = (
        epoch +
        offset_seconds
    )

    local_datetime = datetime.fromtimestamp(
        local_epoch,
        tz=timezone.utc
    )

    print(zone_name)

    print(
        "  UTC:",
        utc_datetime.strftime(
            "%Y-%m-%d %H:%M:%S"
        )
    )

    print(
        "  Local:",
        local_datetime.strftime(
            "%Y-%m-%d %H:%M:%S"
        )
    )

    print(
        "  Zone:",
        abbreviation
    )

    print(
        "  Offset:",
        format_offset(
            offset_seconds
        )
    )

    print()


zones = load_zones()

start_year, end_year, zone_rules = load_rules()


print()
print("========================================")
print("CGPS CLOCK TIMEZONE RULE TEST")
print("========================================")
print()

print(
    "Rule range:",
    start_year,
    "through",
    end_year
)

print()


tests = [

    (
        "America/Los_Angeles",
        datetime(
            2026, 8, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "America/Los_Angeles",
        datetime(
            2026, 1, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "America/Denver",
        datetime(
            2026, 8, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "America/Phoenix",
        datetime(
            2026, 8, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "Europe/London",
        datetime(
            2026, 8, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "Asia/Kathmandu",
        datetime(
            2026, 8, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "Australia/Sydney",
        datetime(
            2026, 8, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),

    (
        "Australia/Sydney",
        datetime(
            2026, 1, 23,
            21, 38, 0,
            tzinfo=timezone.utc
        )
    ),
]


for zone_name, utc_datetime in tests:

    test_zone(
        zone_name,
        utc_datetime,
        zones,
        zone_rules
    )


print("========================================")
print("TEST COMPLETE")
print("========================================")
print()

if sys.stdin.isatty():
    input("Press ENTER to exit...")

