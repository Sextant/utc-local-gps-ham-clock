from datetime import datetime, timedelta, timezone
from pathlib import Path
import struct
from zoneinfo import ZoneInfo


# ============================================================
# PATHS
# ============================================================

BASE = Path(__file__).resolve().parent.parent
OUTPUT = BASE / "output"

ZONES_FILE = OUTPUT / "zones.bin"
RULES_FILE = OUTPUT / "rules.bin"
RULES_TEXT = OUTPUT / "rules.txt"


# ============================================================
# SETTINGS
# ============================================================

START_YEAR = 2025
END_YEAR   = 2045

STEP_HOURS = 6


# ============================================================
# LOAD ZONE NAMES
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
# GET STATE AT UTC TIME
# ============================================================

def get_state(zone, utc_dt):

    local = utc_dt.astimezone(zone)

    offset_seconds = int(
        local.utcoffset().total_seconds()
    )

    abbreviation = local.tzname() or ""

    return (
        offset_seconds,
        abbreviation
    )


# ============================================================
# FIND EXACT TRANSITION
# ============================================================

def refine_transition(
    zone,
    low_utc,
    high_utc,
    old_state
):

    """
    Binary search down to 1-second resolution.
    """

    while (
        high_utc - low_utc
    ).total_seconds() > 1:

        mid = low_utc + (
            high_utc - low_utc
        ) / 2

        mid = mid.replace(
            microsecond=0
        )

        state = get_state(
            zone,
            mid
        )

        if state == old_state:

            low_utc = mid

        else:

            high_utc = mid

    return high_utc


# ============================================================
# BUILD TRANSITIONS FOR ONE ZONE
# ============================================================

def build_zone_transitions(
    zone_name
):

    zone = ZoneInfo(
        zone_name
    )

    start = datetime(
        START_YEAR,
        1,
        1,
        tzinfo=timezone.utc
    )

    end = datetime(
        END_YEAR + 1,
        1,
        1,
        tzinfo=timezone.utc
    )

    transitions = []

    current_state = get_state(
        zone,
        start
    )

    # Initial state
    transitions.append(
        (
            int(start.timestamp()),
            current_state[0],
            current_state[1]
        )
    )

    previous_time = start
    previous_state = current_state

    current_time = (
        start +
        timedelta(hours=STEP_HOURS)
    )

    while current_time <= end:

        state = get_state(
            zone,
            current_time
        )

        if state != previous_state:

            transition_time = refine_transition(
                zone,
                previous_time,
                current_time,
                previous_state
            )

            new_state = get_state(
                zone,
                transition_time
            )

            transitions.append(
                (
                    int(
                        transition_time.timestamp()
                    ),
                    new_state[0],
                    new_state[1]
                )
            )

            previous_state = new_state

        previous_time = current_time

        current_time += timedelta(
            hours=STEP_HOURS
        )

    return transitions


# ============================================================
# MAIN
# ============================================================

print()
print("========================================")
print("CGPS CLOCK TIMEZONE RULE BUILDER")
print("========================================")
print()

zones = load_zones()

print(
    "Zones loaded:",
    len(zones)
)

print(
    "Rule range:",
    START_YEAR,
    "through",
    END_YEAR
)

print()


all_zone_transitions = []


for i, zone_name in enumerate(zones):

    print(
        f"[{i + 1:02d}/{len(zones):02d}]",
        zone_name
    )

    transitions = build_zone_transitions(
        zone_name
    )

    all_zone_transitions.append(
        transitions
    )


# ============================================================
# WRITE BINARY RULE DATABASE
# ============================================================

print()
print("Writing rules.bin...")


with open(
    RULES_FILE,
    "wb"
) as f:

    # Magic
    f.write(
        b"TZRL"
    )

    # Format version
    f.write(
        struct.pack(
            "<H",
            1
        )
    )

    # Start/end years
    f.write(
        struct.pack(
            "<HH",
            START_YEAR,
            END_YEAR
        )
    )

    # Zone count
    f.write(
        struct.pack(
            "<H",
            len(zones)
        )
    )


    # --------------------------------------------------------
    # Build abbreviation table
    # --------------------------------------------------------

    abbreviations = sorted(
        {
            transition[2]

            for zone_transitions
            in all_zone_transitions

            for transition
            in zone_transitions
        }
    )


    if len(abbreviations) > 255:

        raise RuntimeError(
            "Too many timezone abbreviations."
        )


    abbreviation_ids = {
        abbreviation: i

        for i, abbreviation
        in enumerate(abbreviations)
    }


    # Write abbreviation table
    f.write(
        struct.pack(
            "<B",
            len(abbreviations)
        )
    )


    for abbreviation in abbreviations:

        encoded = abbreviation.encode(
            "ascii",
            errors="replace"
        )

        if len(encoded) > 15:

            encoded = encoded[:15]

        f.write(
            struct.pack(
                "<B",
                len(encoded)
            )
        )

        f.write(
            encoded
        )


    # --------------------------------------------------------
    # Write zone transition data
    # --------------------------------------------------------

    for zone_transitions in all_zone_transitions:

        f.write(
            struct.pack(
                "<H",
                len(zone_transitions)
            )
        )

        for (
            epoch,
            offset_seconds,
            abbreviation
        ) in zone_transitions:

            f.write(
                struct.pack(
                    "<qiB",
                    epoch,
                    offset_seconds,
                    abbreviation_ids[
                        abbreviation
                    ]
                )
            )


# ============================================================
# WRITE HUMAN-READABLE TEST FILE
# ============================================================

print(
    "Writing rules.txt..."
)


with open(
    RULES_TEXT,
    "w",
    encoding="utf-8"
) as f:

    for zone_name, transitions in zip(
        zones,
        all_zone_transitions
    ):

        f.write(
            zone_name + "\n"
        )

        for (
            epoch,
            offset_seconds,
            abbreviation
        ) in transitions:

            utc_dt = datetime.fromtimestamp(
                epoch,
                tz=timezone.utc
            )

            hours = offset_seconds / 3600.0

            f.write(
                f"  "
                f"{utc_dt.isoformat()}  "
                f"{abbreviation:6s}  "
                f"UTC{hours:+g}\n"
            )

        f.write("\n")


# ============================================================
# SUMMARY
# ============================================================

print()
print("========================================")
print("RULE DATABASE BUILD COMPLETE")
print("========================================")
print()

print(
    "rules.bin:",
    RULES_FILE.stat().st_size,
    "bytes"
)

print(
    "rules.txt:",
    RULES_TEXT.stat().st_size,
    "bytes"
)

print()

print(
    "Abbreviations:"
)

for abbreviation in abbreviations:

    print(
        " ",
        abbreviation
    )

print()

input(
    "Press ENTER to exit..."
)

