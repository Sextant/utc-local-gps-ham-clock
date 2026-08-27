// ============================================================
// MAIDENHEAD
// ============================================================

void drawHeaderGrid(
  bool force
) {

  char newGrid[7];

  if (
    storedPositionValid
  ) {

    strncpy(
      newGrid,
      storedGrid,
      sizeof(newGrid)
    );
  }

  else {

    strcpy(
      newGrid,
      "------"
    );
  }

  if (
    !force &&
    strcmp(
      newGrid,
      lastGridDisplay
    ) == 0
  ) {

    return;
  }

  strncpy(
    lastGridDisplay,
    newGrid,
    sizeof(lastGridDisplay) - 1
  );

  lastGridDisplay[
    sizeof(lastGridDisplay) - 1
  ] =
    '\0';

  tft.fillRect(
    250,
    0,
    70,
    21,
    backgroundColor()
  );

  tft.setTextDatum(
    TR_DATUM
  );

  tft.setTextColor(
    primaryColor(),
    backgroundColor()
  );

  tft.setFreeFont(
    &FreeSansBold9pt7b
  );

  tft.drawString(
    newGrid,
    315,
    2
  );

  tft.setFreeFont(
    NULL
  );
}

// ============================================================
// LEAP YEAR
// ============================================================

bool isLeapYear(
  int year
) {

  if (
    year % 400 ==
    0
  ) {

    return true;
  }

  if (
    year % 100 ==
    0
  ) {

    return false;
  }

  return
    (
      year % 4 ==
      0
    );
}

// ============================================================
// DAYS IN MONTH
// ============================================================

int daysInMonth(
  int year,
  int month
) {

  static const int days[] = {
    31,
    28,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31
  };

  if (
    month == 2 &&
    isLeapYear(
      year
    )
  ) {

    return 29;
  }

  return
    days[
      month - 1
    ];
}

// ============================================================
// MAIDENHEAD
// ============================================================

void maidenhead(
  double latitude,
  double longitude,
  char* grid
) {

  longitude +=
    180.0;

  latitude +=
    90.0;

  int A =
    longitude /
    20.0;

  int B =
    latitude /
    10.0;

  longitude -=
    A *
    20.0;

  latitude -=
    B *
    10.0;

  int C =
    longitude /
    2.0;

  int D =
    latitude;

  longitude -=
    C *
    2.0;

  latitude -=
    D;

  int E =
    longitude *
    12.0;

  int F =
    latitude *
    24.0;

  grid[0] =
    'A' + A;

  grid[1] =
    'A' + B;

  grid[2] =
    '0' + C;

  grid[3] =
    '0' + D;

  grid[4] =
    'a' + E;

  grid[5] =
    'a' + F;

  grid[6] =
    '\0';
}

// ============================================================
// FULL MONTH NAME
// ============================================================

const char* monthFullName(
  int month
) {

  static const char* months[] = {

    "---",
    "JANUARY",
    "FEBRUARY",
    "MARCH",
    "APRIL",
    "MAY",
    "JUNE",
    "JULY",
    "AUGUST",
    "SEPTEMBER",
    "OCTOBER",
    "NOVEMBER",
    "DECEMBER"
  };

  if (
    month >= 1 &&
    month <= 12
  ) {

    return
      months[
        month
      ];
  }

  return "---";
}
