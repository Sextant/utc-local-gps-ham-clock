// ============================================================
// CLOCK SCREEN
// ============================================================

void drawClockScreen() {

  tft.fillScreen(
    backgroundColor()
  );

  lastGpsDisplay[0] =
    '\0';

  lastGridDisplay[0] =
    '\0';

  // Force static date/suffix fields to redraw after a full-screen clear.
  lastUTCDateDisplay[0] =
    '\0';

  lastLocalDateDisplay[0] =
    '\0';

  drawCallsign();

  drawPowerStatus();

  drawHeaderGrid(
    true
  );

  drawAlarmIndicator();

  drawBoldSmallLabel(
    "UTC",
    5,
    22
  );

  tft.drawFastHLine(
    5,
    108,
    310,
    secondaryColor()
  );

  drawLocalHeader();

  tft.drawFastHLine(
    5,
    204,
    310,
    secondaryColor()
  );

  if (
    !systemTimeValid
  ) {

    tft.setFreeFont(
      NULL
    );

    tft.setTextDatum(
      MC_DATUM
    );

    tft.setTextColor(
      primaryColor(),
      backgroundColor()
    );

    tft.drawString(
      "GPS ACQUIRING TIME",
      160,
      80,
      2
    );

    drawGPSLine(
      true
    );

    return;
  }

  updateClockScreen();
}

// ============================================================
// CLOCK UPDATE
// ============================================================

void updateClockScreen() {

  if (
    !systemTimeValid
  ) {

    return;
  }

  time_t now =
    time(nullptr);

  struct tm utcTime;
  struct tm localTime;

  gmtime_r(
    &now,
    &utcTime
  );

  getLocalTimeForEpoch(
    now,
    localTime
  );

  drawUTCTime(
    utcTime
  );

  drawUTCDate(
    utcTime
  );

  drawLocalTime(
    localTime
  );

  drawLocalDate(
    localTime
  );

  drawGPSLine();

  drawHeaderGrid();

  drawAlarmIndicator();
}

// ============================================================
// UTC TIME
// ============================================================

void drawUTCTime(
  struct tm &t
) {

  char buf[16];

  snprintf(
    buf,
    sizeof(buf),

    "%02d:%02d:%02d",

    t.tm_hour,
    t.tm_min,
    t.tm_sec
  );

  utcTimeSprite.fillSprite(
    backgroundColor()
  );

  utcTimeSprite.setTextColor(
    primaryColor()
  );

  drawTightSevenSegTime(
    utcTimeSprite,
    buf
  );

  utcTimeSprite.pushSprite(
    5,
    37
  );
}

// ============================================================
// UTC DATE
// ============================================================

void drawUTCDate(
  struct tm &t
) {

  char buf[28];

  snprintf(
    buf,
    sizeof(buf),

    "%02d %s",

    t.tm_mday,

    monthFullName(
      t.tm_mon + 1
    )
  );

  // UTC date normally changes only once per day. Do not erase and
  // redraw this field every second.
  if (
    strcmp(
      buf,
      lastUTCDateDisplay
    ) == 0
  ) {
    return;
  }

  strncpy(
    lastUTCDateDisplay,
    buf,
    sizeof(lastUTCDateDisplay) - 1
  );

  lastUTCDateDisplay[
    sizeof(lastUTCDateDisplay) - 1
  ] = '\0';

  tft.fillRect(
    30,
    90,
    260,
    17,
    backgroundColor()
  );

  tft.setFreeFont(
    NULL
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    dateColor(),
    backgroundColor()
  );

  tft.drawString(
    buf,
    160,
    98,
    2
  );
}

// ============================================================
// LOCAL TIME
// ============================================================

void drawLocalTime(
  struct tm &t
) {

  char timeBuf[16];

  if (
    local24Hour
  ) {

    snprintf(
      timeBuf,
      sizeof(timeBuf),

      "%02d:%02d:%02d",

      t.tm_hour,
      t.tm_min,
      t.tm_sec
    );
  }

  else {

    int displayHour =
      t.tm_hour %
      12;

    if (
      displayHour == 0
    ) {

      displayHour = 12;
    }

    snprintf(
      timeBuf,
      sizeof(timeBuf),

      "%d:%02d:%02d",

      displayHour,
      t.tm_min,
      t.tm_sec
    );
  }

  localTimeSprite.fillSprite(
    backgroundColor()
  );

  localTimeSprite.setTextColor(
    primaryColor()
  );

  drawTightSevenSegTime(
    localTimeSprite,
    timeBuf
  );

  localTimeSprite.pushSprite(
    5,
    129
  );
}

// ============================================================
// FONT 7 KERNING
//
// Special handling prevents 11: from touching.
// ============================================================

int sevenSegPairGap(
  char a,
  char b
) {
  return 2;
}

// ============================================================
// TIGHT SEVEN SEGMENT TIME
// ============================================================

void drawTightSevenSegTime(
  TFT_eSprite &sprite,
  const char* text
) {

  int length = strlen(text);

  // Fixed-width digit slots prevent the time from shifting
  // when a narrow glyph such as 1 appears or disappears.
  int digitSlot =
    sprite.textWidth(
      "8",
      7
    );

  int colonSlot =
    sprite.textWidth(
      ":",
      7
    ) + 2;

  int totalWidth = 0;

  for (
    int i = 0;
    i < length;
    i++
  ) {

    if (
      text[i] == ':'
    ) {
      totalWidth += colonSlot;
    }
    else {
      totalWidth += digitSlot;
    }
  }

  int x =
    (
      sprite.width() -
      totalWidth
    ) / 2;

  int y = 26;

  sprite.setTextDatum(
    MC_DATUM
  );

  for (
    int i = 0;
    i < length;
    i++
  ) {

    char oneChar[2];

    oneChar[0] = text[i];
    oneChar[1] = '\0';

    if (
      text[i] == ':'
    ) {

      sprite.drawString(
        oneChar,
        x + colonSlot / 2,
        y,
        7
      );

      x += colonSlot;
    }
    else {

      sprite.drawString(
        oneChar,
        x + digitSlot / 2,
        y,
        7
      );

      x += digitSlot;
    }
  }
}


// ============================================================
// LOCAL DATE
// ============================================================

void drawLocalDate(
  struct tm &t
) {

  char dateBuf[28];

  snprintf(
    dateBuf,
    sizeof(dateBuf),

    "%02d %s",

    t.tm_mday,

    monthFullName(
      t.tm_mon + 1
    )
  );

  const char* suffix =
    t.tm_hour >= 12
    ? "PM"
    : "AM";

  // Include 12/24-hour mode and AM/PM in the cache key so switching
  // modes or crossing noon/midnight forces a redraw. Otherwise this
  // entire static field remains untouched while seconds tick.
  char logicalDisplay[40];

  snprintf(
    logicalDisplay,
    sizeof(logicalDisplay),
    "%s|%s|%d",
    dateBuf,
    local24Hour ? "24" : suffix,
    local24Hour ? 1 : 0
  );

  if (
    strcmp(
      logicalDisplay,
      lastLocalDateDisplay
    ) == 0
  ) {
    return;
  }

  strncpy(
    lastLocalDateDisplay,
    logicalDisplay,
    sizeof(lastLocalDateDisplay) - 1
  );

  lastLocalDateDisplay[
    sizeof(lastLocalDateDisplay) - 1
  ] = '\0';

  tft.fillRect(
    0,
    183,
    320,
    20,
    backgroundColor()
  );

  tft.setFreeFont(
    NULL
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    dateColor(),
    backgroundColor()
  );

  tft.drawString(
    dateBuf,
    160,
    192,
    2
  );

  if (
    !local24Hour
  ) {

    tft.setTextDatum(
      MR_DATUM
    );

    tft.setTextColor(
      primaryColor(),
      backgroundColor()
    );

    tft.drawString(
      suffix,
      312,
      192,
      2
    );
  }
}

// ============================================================
// CALLSIGN
// ============================================================

void drawCallsign() {

  tft.fillRect(
    0,
    0,
    90,
    21,
    backgroundColor()
  );

  tft.setTextDatum(
    TL_DATUM
  );

  tft.setTextColor(
    primaryColor(),
    backgroundColor()
  );

  tft.setFreeFont(
    &FreeSansBold9pt7b
  );

  tft.drawString(
    callSign,
    5,
    2
  );

  tft.setFreeFont(
    NULL
  );
}

// ============================================================
// PLUG ICON
// ============================================================

void drawHorizontalUSPlugIcon(
  int x,
  int y,
  uint16_t color
) {

  tft.drawLine(
    x - 10,
    y,
    x - 6,
    y,
    color
  );

  tft.fillRect(
    x - 5,
    y - 3,
    7,
    7,
    color
  );

  tft.drawFastHLine(
    x + 2,
    y - 2,
    5,
    color
  );

  tft.drawFastHLine(
    x + 2,
    y + 2,
    5,
    color
  );
}

// ============================================================
// BELL ICON
// ============================================================

void drawBellIcon(
  int x,
  int y,
  uint16_t color
) {

  tft.drawPixel(
    x,
    y - 6,
    color
  );

  tft.drawFastHLine(
    x - 2,
    y - 5,
    5,
    color
  );

  tft.drawFastHLine(
    x - 4,
    y - 3,
    9,
    color
  );

  tft.drawLine(
    x - 4,
    y - 3,
    x - 4,
    y + 2,
    color
  );

  tft.drawLine(
    x + 4,
    y - 3,
    x + 4,
    y + 2,
    color
  );

  tft.drawLine(
    x - 4,
    y + 2,
    x - 6,
    y + 4,
    color
  );

  tft.drawLine(
    x + 4,
    y + 2,
    x + 6,
    y + 4,
    color
  );

  tft.drawFastHLine(
    x - 6,
    y + 4,
    13,
    color
  );

  tft.drawFastHLine(
    x - 1,
    y + 6,
    3,
    color
  );
}

// ============================================================
// MANUAL LOCAL DATE/TIME -> UTC
// ============================================================

time_t localFieldsToUtcEpoch(
  struct tm localFields
) {

  // Interpret the entered wall-clock fields as if they were UTC
  // to get a neutral numeric epoch for those calendar fields.
  setenv("TZ", "UTC0", 1);
  tzset();

  time_t naiveLocalEpoch =
    mktime(&localFields);

  int32_t offset =
    currentTimeRuleValid
      ? currentUtcOffsetSeconds
      : 0;

  if (
    geoContextValid
    && !currentZoneNautical
    && currentZoneID >= 0
    && timezoneDatabaseReady
  ) {

    // First estimate. The timezone lookup is keyed by UTC, so subtract
    // the currently known offset and then ask the rules database what
    // offset applies on the entered date. Repeat once for DST boundaries.
    time_t guessUtc =
      naiveLocalEpoch - offset;

    int32_t testOffset;
    char testAbbrev[16];
    time_t testNext;

    if (lookupTimeRule(
      (uint8_t)currentZoneID,
      guessUtc,
      testOffset,
      testAbbrev,
      sizeof(testAbbrev),
      testNext
    )) {

      offset = testOffset;
      guessUtc = naiveLocalEpoch - offset;

      if (lookupTimeRule(
        (uint8_t)currentZoneID,
        guessUtc,
        testOffset,
        testAbbrev,
        sizeof(testAbbrev),
        testNext
      )) {
        offset = testOffset;
      }
    }
  }

  return naiveLocalEpoch - offset;
}

// ============================================================
// DYNAMIC LOCAL HEADER
// ============================================================

void drawLocalHeader() {

  // Clear only the LOCAL header band.
  tft.fillRect(
    0,
    110,
    320,
    18,
    backgroundColor()
  );

  drawBoldSmallLabel(
    "LOCAL",
    5,
    112
  );

  tft.setFreeFont(NULL);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(accentColor());

  int x =
    5
    + tft.textWidth("LOCAL", 2)
    + 14;

  char offsetBuf[16];

  formatUTCOffset(
    currentTimeRuleValid
      ? currentUtcOffsetSeconds
      : 0,
    offsetBuf,
    sizeof(offsetBuf)
  );

  char suffix[160];

  if (!geoContextValid) {

    snprintf(
      suffix,
      sizeof(suffix),
      "UTC  GPS LOCATION PENDING"
    );
  }

  else if (currentPlaceValid) {

    snprintf(
      suffix,
      sizeof(suffix),
      "%s (%s) %s",
      currentZoneAbbrev,
      offsetBuf,
      currentPlaceName
    );
  }

  else if (currentMarineValid) {

    if (currentZoneNautical) {

      snprintf(
        suffix,
        sizeof(suffix),
        "%s  %s",
        offsetBuf,
        currentMarineName
      );
    }

    else {

      snprintf(
        suffix,
        sizeof(suffix),
        "%s (%s) %s",
        currentZoneAbbrev,
        offsetBuf,
        currentMarineName
      );
    }
  }

  else if (currentZoneNautical) {

    snprintf(
      suffix,
      sizeof(suffix),
      "%s  AT SEA",
      offsetBuf
    );
  }

  else {

    snprintf(
      suffix,
      sizeof(suffix),
      "%s (%s) LOCATION UNKNOWN",
      currentZoneAbbrev,
      offsetBuf
    );
  }

  // Use the compact built-in font for the dynamic suffix so long
  // worldwide place names have the best chance of fitting.
  tft.drawString(
    suffix,
    x,
    116,
    1
  );
}



