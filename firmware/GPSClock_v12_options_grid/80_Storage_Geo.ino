// ============================================================
// CACHE CURRENT POSITION
// ============================================================

void cacheCurrentGPSPosition() {

  if (!stableGpsFixReady) {
    return;
  }

  bool hadStoredPosition =
    storedPositionValid;

  double previousStoredLatitude =
    storedLatitude;

  double previousStoredLongitude =
    storedLongitude;

  storedLatitude =
    stableGpsLatitude;

  storedLongitude =
    stableGpsLongitude;

  storedAltitude =
    stableGpsAltitude;

  storedSatellites =
    stableGpsSatellites;

  maidenhead(
    storedLatitude,
    storedLongitude,
    storedGrid
  );

  storedPositionValid =
    true;

  // Explicitly force a geographic re-resolution when the newly
  // accepted GPS fix is materially different from either the previous
  // stored fix or the coordinates used for the current label. This
  // prevents the bottom GPS line from updating while a stale locality
  // remains in the LOCAL header.
  bool forceGeoResolution =
    !geoContextValid;

  if (hadStoredPosition) {

    double movedFromPreviousStored = distanceKm(
      previousStoredLatitude,
      previousStoredLongitude,
      storedLatitude,
      storedLongitude
    );

    if (movedFromPreviousStored >= LOCATION_RELOOKUP_DISTANCE_KM) {
      forceGeoResolution = true;
    }
  }
  else {
    forceGeoResolution = true;
  }

  if (geoContextValid) {

    double movedFromResolved = distanceKm(
      resolvedLocationLat,
      resolvedLocationLon,
      storedLatitude,
      storedLongitude
    );

    if (movedFromResolved >= LOCATION_RELOOKUP_DISTANCE_KM) {
      forceGeoResolution = true;
    }
  }

  Serial.print("Accepted GPS position: ");
  Serial.print(storedLatitude, 6);
  Serial.print(", ");
  Serial.print(storedLongitude, 6);
  Serial.print(" ALT:");
  Serial.print(storedAltitude, 0);
  Serial.print("ft  geo force:");
  Serial.println(forceGeoResolution ? "YES" : "NO");

  resolveGeographicContextIfNeeded(
    forceGeoResolution
  );

  lastGpsDisplay[0] =
    '\0';

  lastGridDisplay[0] =
    '\0';

  if (
    currentPage ==
      PAGE_CLOCK &&
    !alarmActive
  ) {

    drawGPSLine(
      true
    );

    drawHeaderGrid(
      true
    );
  }
}

// ============================================================
// SET ESP32 CLOCK
// ============================================================

void setSystemEpoch(
  time_t epoch
) {

  struct timeval tv;

  tv.tv_sec =
    epoch;

  tv.tv_usec =
    0;

  settimeofday(
    &tv,
    NULL
  );

  systemTimeValid =
    true;

  lastDisplayedSecond =
    -1;
}

// ============================================================
// DECIMAL -> DMS
// ============================================================

void decimalToDMS(
  double coordinate,
  bool latitude,
  int &degrees,
  int &minutes,
  double &seconds,
  char &hemisphere
) {

  if (
    latitude
  ) {

    hemisphere =
      coordinate >= 0.0
      ? 'N'
      : 'S';
  }

  else {

    hemisphere =
      coordinate >= 0.0
      ? 'E'
      : 'W';
  }

  double absolute =
    fabs(
      coordinate
    );

  degrees =
    (int)absolute;

  double minuteValue =
    (
      absolute -
      degrees
    ) *
    60.0;

  minutes =
    (int)minuteValue;

  seconds =
    (
      minuteValue -
      minutes
    ) *
    60.0;

  if (
    seconds >=
    59.995
  ) {

    seconds =
      0.0;

    minutes++;

    if (
      minutes >=
      60
    ) {

      minutes =
        0;

      degrees++;
    }
  }
}

// ============================================================
// GPS LINE
//
// Decimal:
// 36.47180   -121.73480
//
// DMS:
// 36°28'18.48"N   121°44'5.28"W
// ============================================================

void drawGPSLine(
  bool force
) {

  char logicalLine[160];

  snprintf(
    logicalLine,
    sizeof(logicalLine),

    "%d|%d|%d|%.6f|%.6f|%.1f",

    (int)gpsState,
    coordinateDMS ? 1 : 0,
    storedSatellites,
    storedLatitude,
    storedLongitude,
    storedAltitude
  );

  if (
    !force &&
    strcmp(
      logicalLine,
      lastGpsDisplay
    ) == 0
  ) {

    return;
  }

  strncpy(
    lastGpsDisplay,
    logicalLine,
    sizeof(lastGpsDisplay) - 1
  );

  lastGpsDisplay[
    sizeof(lastGpsDisplay) - 1
  ] =
    '\0';

  gpsSprite.fillSprite(
    backgroundColor()
  );

  gpsSprite.setFreeFont(
    NULL
  );

  gpsSprite.setTextDatum(
    TL_DATUM
  );

  uint16_t c;

  if (
    storedPositionValid
  ) {

    c =
      statusColor();
  }

  else if (
    redMode
  ) {

    c =
      TFT_BLACK;
  }

  else if (
    lightTheme
  ) {

    c =
      TFT_RED;
  }

  else {

    c =
      TFT_ORANGE;
  }

  gpsSprite.setTextColor(
    c
  );

  // ----------------------------------------------------------
  // HAVE A STORED POSITION
  // ----------------------------------------------------------

  if (
    storedPositionValid
  ) {

    const char* status;

    if (
      gpsState ==
      GPS_SLEEP
    ) {

      status =
        lastGpsCyclePositionSynced
          ? "GPS SYNC"
          : "GPS CACHED";
    }

    else if (
      gpsHasFreshFix()
    ) {

      status =
        "GPS LOCK";
    }

    else {

      status =
        "GPS ACQUIRING";
    }

    int y =
      12;

    int x =
      2;

    // Decimal retains 8 px.
    // DMS reduced slightly so altitude stays fully on-screen.
    const int FIELD_GAP =
      coordinateDMS ? 6 : 8;

    char buf[64];

    // STATUS
    gpsSprite.drawString(
      status,
      x,
      y,
      1
    );

    x +=
      gpsSprite.textWidth(
        status,
        1
      ) +
      FIELD_GAP;

    // SAT
    snprintf(
      buf,
      sizeof(buf),

      "SAT:%d",

      storedSatellites
    );

    gpsSprite.drawString(
      buf,
      x,
      y,
      1
    );

    x +=
      gpsSprite.textWidth(
        buf,
        1
      ) +
      FIELD_GAP;

    // --------------------------------------------------------
    // DECIMAL DEGREES
    // --------------------------------------------------------

    if (
      !coordinateDMS
    ) {

      snprintf(
        buf,
        sizeof(buf),

        "%.5f",

        storedLatitude
      );

      gpsSprite.drawString(
        buf,
        x,
        y,
        1
      );

      x +=
        gpsSprite.textWidth(
          buf,
          1
        ) +
        FIELD_GAP;

      snprintf(
        buf,
        sizeof(buf),

        "%.5f",

        storedLongitude
      );

      gpsSprite.drawString(
        buf,
        x,
        y,
        1
      );

      x +=
        gpsSprite.textWidth(
          buf,
          1
        ) +
        FIELD_GAP;
    }

    // --------------------------------------------------------
    // DMS
    // --------------------------------------------------------

    else {

      int latDeg;
      int latMin;
      double latSec;
      char latHem;

      int lonDeg;
      int lonMin;
      double lonSec;
      char lonHem;

      decimalToDMS(
        storedLatitude,
        true,
        latDeg,
        latMin,
        latSec,
        latHem
      );

      decimalToDMS(
        storedLongitude,
        false,
        lonDeg,
        lonMin,
        lonSec,
        lonHem
      );

      char degreeChar =
        (char)247;

      snprintf(
        buf,
        sizeof(buf),

        "%d%c%02d'%05.2f\"%c",

        latDeg,
        degreeChar,
        latMin,
        latSec,
        latHem
      );

      gpsSprite.drawString(
        buf,
        x,
        y,
        1
      );

      x +=
        gpsSprite.textWidth(
          buf,
          1
        ) +
        FIELD_GAP;

      snprintf(
        buf,
        sizeof(buf),

        "%d%c%02d'%05.2f\"%c",

        lonDeg,
        degreeChar,
        lonMin,
        lonSec,
        lonHem
      );

      gpsSprite.drawString(
        buf,
        x,
        y,
        1
      );

      x +=
        gpsSprite.textWidth(
          buf,
          1
        ) +
        FIELD_GAP;
    }

    // ALT
    snprintf(
      buf,
      sizeof(buf),

      "ALT:%.0fft",

      storedAltitude
    );

    gpsSprite.drawString(
      buf,
      x,
      y,
      1
    );
  }

  // ----------------------------------------------------------
  // NO STORED POSITION YET
  // ----------------------------------------------------------

  else {

    int sats =
      gps.satellites.isValid()
      ? gps.satellites.value()
      : 0;

    char buf[48];

    if (
      gpsState ==
      GPS_AWAKE
    ) {

      snprintf(
        buf,
        sizeof(buf),

        "GPS ACQUIRING SAT:%d",

        sats
      );
    }

    else {

      snprintf(
        buf,
        sizeof(buf),

        "GPS NO FIX"
      );
    }

    gpsSprite.setTextDatum(
      MC_DATUM
    );

    gpsSprite.drawString(
      buf,
      160,
      15,
      1
    );
  }

  gpsSprite.pushSprite(
    0,
    207
  );
}

// ============================================================
// POWER
// ============================================================

void drawPowerStatus() {

  tft.fillRect(
    213,
    0,
    34,
    20,
    backgroundColor()
  );

  if (
    !batteryHardwareInstalled
  ) {

    drawHorizontalUSPlugIcon(
      229,
      8,
      secondaryColor()
    );
  }
}

// ============================================================
// SHARED VSPI BUS SWITCHING
// ============================================================

void selectSDBus() {

  sharedSPI.end();

  sharedSPI.begin(
    SD_SCK,
    SD_MISO,
    SD_MOSI,
    SD_CS
  );
}

void selectTouchBus() {

  sharedSPI.end();

  sharedSPI.begin(
    TOUCH_CLK,
    TOUCH_MISO,
    TOUCH_MOSI,
    TOUCH_CS
  );

  ts.begin(
    sharedSPI
  );
}

// ============================================================
// LITTLE-ENDIAN DATABASE READERS
// ============================================================

uint8_t readU8(File &f) {
  return (uint8_t)f.read();
}

uint16_t readU16(File &f) {

  uint8_t b[2];

  if (f.read(b, 2) != 2) {
    return 0;
  }

  return
    (uint16_t)b[0]
    |
    ((uint16_t)b[1] << 8);
}

uint32_t readU32(File &f) {

  uint8_t b[4];

  if (f.read(b, 4) != 4) {
    return 0;
  }

  return
      (uint32_t)b[0]
    | ((uint32_t)b[1] << 8)
    | ((uint32_t)b[2] << 16)
    | ((uint32_t)b[3] << 24);
}

int32_t readI32(File &f) {
  return (int32_t)readU32(f);
}

int64_t readI64(File &f) {

  uint8_t b[8];

  if (f.read(b, 8) != 8) {
    return 0;
  }

  uint64_t value =
      (uint64_t)b[0]
    | ((uint64_t)b[1] << 8)
    | ((uint64_t)b[2] << 16)
    | ((uint64_t)b[3] << 24)
    | ((uint64_t)b[4] << 32)
    | ((uint64_t)b[5] << 40)
    | ((uint64_t)b[6] << 48)
    | ((uint64_t)b[7] << 56);

  return (int64_t)value;
}

float readFloatLE(File &f) {

  union {
    uint32_t u;
    float f;
  } value;

  value.u = readU32(f);

  return value.f;
}

bool checkMagic(
  File &f,
  const char* expected
) {

  char magic[5];

  magic[0] = f.read();
  magic[1] = f.read();
  magic[2] = f.read();
  magic[3] = f.read();
  magic[4] = '\0';

  return strcmp(
    magic,
    expected
  ) == 0;
}

// ============================================================
// SD INITIALIZATION
// ============================================================

bool initializeSDDatabases() {

  selectSDBus();

  Serial.println("Mounting microSD...");

  if (!SD.begin(SD_CS, sharedSPI)) {

    Serial.println("WARNING: microSD mount failed. LOCAL will fall back to UTC.");

    sdAvailable = false;
    timezoneDatabaseReady = false;
    placeDatabaseReady = false;
    marineDatabaseReady = false;

    selectTouchBus();

    return false;
  }

  sdAvailable = true;

  timezoneDatabaseReady =
    loadTimezoneDatabaseHeaders();

  placeDatabaseReady =
    loadPlaceDatabaseHeaders();

  marineDatabaseReady =
    loadMarineDatabaseHeader();

  Serial.print("Timezone database: ");
  Serial.println(timezoneDatabaseReady ? "READY" : "NOT AVAILABLE");

  Serial.print("Place database: ");
  Serial.println(placeDatabaseReady ? "READY" : "NOT AVAILABLE");

  Serial.print("Marine database: ");
  Serial.println(marineDatabaseReady ? "READY" : "NOT AVAILABLE");

  selectTouchBus();

  return timezoneDatabaseReady;
}

// ============================================================
// TIMEZONE DATABASE HEADERS
// ============================================================

bool loadTimezoneDatabaseHeaders() {

  File f = SD.open(TZ_INDEX_FILE, FILE_READ);

  if (!f) {
    return false;
  }

  if (!checkMagic(f, "TZIX")) {
    f.close();
    return false;
  }

  readU16(f); // version

  tzCoarseCols = readU16(f);
  tzCoarseRows = readU16(f);
  tzCoarseDeg = readFloatLE(f);
  tzFineDeg = readFloatLE(f);
  tzFinePerCoarse = readU16(f);
  readU32(f); // coarse record count

  f.close();

  f = SD.open(TZ_TILES_FILE, FILE_READ);

  if (!f) {
    return false;
  }

  if (!checkMagic(f, "TZTL")) {
    f.close();
    return false;
  }

  readU16(f); // version

  tzTileWidth = readU16(f);
  tzTileHeight = readU16(f);
  tzTileCount = readU32(f);

  f.close();

  // Verify zone/rule files also exist.
  f = SD.open(TZ_ZONES_FILE, FILE_READ);
  if (!f) {
    return false;
  }
  f.close();

  f = SD.open(TZ_RULES_FILE, FILE_READ);
  if (!f) {
    return false;
  }
  f.close();

  return true;
}

// ============================================================
// PLACE DATABASE HEADERS
// ============================================================

bool loadPlaceDatabaseHeaders() {

  File f = SD.open(PLACES_INDEX_FILE, FILE_READ);

  if (!f) {
    return false;
  }

  if (!checkMagic(f, "PIDX")) {
    f.close();
    return false;
  }

  readU16(f); // version

  placeCellDegrees = readFloatLE(f);
  placeGridCols = readU16(f);
  placeGridRows = readU16(f);
  placeCellCount = readU32(f);

  f.close();

  f = SD.open(PLACES_FILE, FILE_READ);

  if (!f) {
    return false;
  }

  if (!checkMagic(f, "PLAC")) {
    f.close();
    return false;
  }

  readU16(f); // version
  placeRecordCount = readU32(f);

  f.close();

  return true;
}

// ============================================================
// MARINE DATABASE HEADER
// ============================================================

bool loadMarineDatabaseHeader() {

  File f = SD.open(MARINE_FILE, FILE_READ);

  if (!f) {
    return false;
  }

  bool valid = checkMagic(f, "MRN1");

  if (valid) {
    readU32(f); // version
    uint32_t count = readU32(f);

    Serial.print("Marine areas: ");
    Serial.println(count);
  }

  f.close();

  return valid;
}

// ============================================================
// POINT IN MARINE POLYGON
// ============================================================

bool pointInMarinePolygon(
  File &f,
  uint32_t pointCount,
  double testLon,
  double testLat
) {

  if (pointCount < 3) {

    f.seek(
      f.position()
      + pointCount * 8UL
    );

    return false;
  }

  float firstLon = readFloatLE(f);
  float firstLat = readFloatLE(f);

  float previousLon = firstLon;
  float previousLat = firstLat;

  bool inside = false;

  for (
    uint32_t i = 1;
    i < pointCount;
    i++
  ) {

    float currentLon = readFloatLE(f);
    float currentLat = readFloatLE(f);

    bool crosses =
      (
        (previousLat > testLat)
        !=
        (currentLat > testLat)
      );

    if (crosses) {

      double denominator =
        currentLat - previousLat;

      if (fabs(denominator) < 0.0000001) {
        denominator = 0.0000001;
      }

      double crossingLon =
          (currentLon - previousLon)
          * (testLat - previousLat)
          / denominator
          + previousLon;

      if (testLon < crossingLon) {
        inside = !inside;
      }
    }

    previousLon = currentLon;
    previousLat = currentLat;
  }

  bool crosses =
    (
      (previousLat > testLat)
      !=
      (firstLat > testLat)
    );

  if (crosses) {

    double denominator =
      firstLat - previousLat;

    if (fabs(denominator) < 0.0000001) {
      denominator = 0.0000001;
    }

    double crossingLon =
        (firstLon - previousLon)
        * (testLat - previousLat)
        / denominator
        + previousLon;

    if (testLon < crossingLon) {
      inside = !inside;
    }
  }

  return inside;
}

// ============================================================
// MARINE AREA LOOKUP
// ============================================================

bool lookupMarineArea(
  double latitude,
  double longitude,
  char* result,
  size_t resultSize
) {

  if (!marineDatabaseReady) {
    return false;
  }

  unsigned long startMillis = millis();
  uint32_t recordsChecked = 0;

  selectSDBus();

  File f = SD.open(MARINE_FILE, FILE_READ);

  if (!f) {
    selectTouchBus();
    return false;
  }

  if (!checkMagic(f, "MRN1")) {
    f.close();
    selectTouchBus();
    return false;
  }

  readU32(f); // version
  uint32_t recordCount = readU32(f);

  bool found = false;

  for (
    uint32_t record = 0;
    record < recordCount;
    record++
  ) {

    recordsChecked++;

    float minLon = readFloatLE(f);
    float minLat = readFloatLE(f);
    float maxLon = readFloatLE(f);
    float maxLat = readFloatLE(f);

    uint16_t nameLength = readU16(f);

    char name[96];

    uint16_t copyLength = min(
      nameLength,
      (uint16_t)(sizeof(name) - 1)
    );

    if (
      f.read((uint8_t*)name, copyLength)
      != copyLength
    ) {
      break;
    }

    name[copyLength] = '\0';

    if (nameLength > copyLength) {
      f.seek(
        f.position()
        + nameLength
        - copyLength
      );
    }

    uint16_t polygonCount = readU16(f);

    bool boundingBoxMatch =
      (
        longitude >= minLon
        && longitude <= maxLon
        && latitude >= minLat
        && latitude <= maxLat
      );

    bool areaMatch = false;

    for (
      uint16_t polygon = 0;
      polygon < polygonCount;
      polygon++
    ) {

      uint32_t pointCount = readU32(f);

      if (boundingBoxMatch && !areaMatch) {

        if (
          pointInMarinePolygon(
            f,
            pointCount,
            longitude,
            latitude
          )
        ) {
          areaMatch = true;
        }
      }

      else {
        f.seek(
          f.position()
          + pointCount * 8UL
        );
      }
    }

    if (areaMatch) {

      strncpy(
        result,
        name,
        resultSize - 1
      );

      result[resultSize - 1] = '\0';

      found = true;
      break;
    }
  }

  f.close();

  unsigned long elapsed = millis() - startMillis;

  selectTouchBus();

  Serial.print("Marine lookup: ");
  Serial.print(recordsChecked);
  Serial.print(" records, ");
  Serial.print(elapsed);
  Serial.println(" ms");

  return found;
}

// ============================================================
// NAUTICAL TIMEZONE FALLBACK
// ============================================================

int nauticalOffsetHours(
  double longitude
) {

  int offset =
    (int)round(
      longitude / 15.0
    );

  if (offset < -12) {
    offset = -12;
  }

  if (offset > 12) {
    offset = 12;
  }

  return offset;
}

// ============================================================
// TIMEZONE GEOGRAPHIC LOOKUP
// ============================================================

int lookupZoneID(
  double latitude,
  double longitude,
  bool &nautical,
  int &nauticalOffset
) {

  nautical = false;
  nauticalOffset = 0;

  if (!timezoneDatabaseReady) {
    return -1;
  }

  if (
    latitude < -90.0 ||
    latitude > 90.0 ||
    longitude < -180.0 ||
    longitude > 180.0
  ) {
    return -1;
  }

  if (longitude >= 180.0) {
    longitude = 179.999999;
  }

  if (latitude >= 90.0) {
    latitude = 89.999999;
  }

  int col =
    (int)(
      (longitude + 180.0)
      / tzCoarseDeg
    );

  int row =
    (int)(
      (latitude + 90.0)
      / tzCoarseDeg
    );

  if (
    col < 0 ||
    col >= tzCoarseCols ||
    row < 0 ||
    row >= tzCoarseRows
  ) {
    return -1;
  }

  uint32_t recordNumber =
    (uint32_t)row
    * tzCoarseCols
    + col;

  uint32_t recordOffset =
    24UL
    + recordNumber * 5UL;

  selectSDBus();

  File f = SD.open(TZ_INDEX_FILE, FILE_READ);

  if (!f || !f.seek(recordOffset)) {
    if (f) f.close();
    selectTouchBus();
    return -1;
  }

  uint8_t cellType = readU8(f);
  uint32_t value = readU32(f);

  f.close();

  if (cellType == TZ_CELL_DIRECT) {
    selectTouchBus();
    return (int)value;
  }

  if (cellType == TZ_CELL_UNKNOWN) {

    nautical = true;
    nauticalOffset = nauticalOffsetHours(longitude);

    selectTouchBus();
    return -1;
  }

  if (cellType != TZ_CELL_TILE) {
    selectTouchBus();
    return -1;
  }

  double coarseLon0 =
    -180.0
    + col * tzCoarseDeg;

  double coarseLat0 =
    -90.0
    + row * tzCoarseDeg;

  int fineCol =
    (int)(
      (longitude - coarseLon0)
      / tzFineDeg
    );

  int fineRow =
    (int)(
      (latitude - coarseLat0)
      / tzFineDeg
    );

  fineCol = constrain(
    fineCol,
    0,
    tzFinePerCoarse - 1
  );

  fineRow = constrain(
    fineRow,
    0,
    tzFinePerCoarse - 1
  );

  uint32_t fineIndex =
    (uint32_t)fineRow
    * tzFinePerCoarse
    + fineCol;

  uint32_t tileSize =
    (uint32_t)tzTileWidth
    * tzTileHeight;

  uint32_t tileOffset =
    14UL
    + value * tileSize
    + fineIndex;

  f = SD.open(TZ_TILES_FILE, FILE_READ);

  if (!f || !f.seek(tileOffset)) {
    if (f) f.close();
    selectTouchBus();
    return -1;
  }

  int zoneID = f.read();

  f.close();

  if (zoneID == TZ_UNKNOWN_ZONE) {

    nautical = true;
    nauticalOffset = nauticalOffsetHours(longitude);

    selectTouchBus();
    return -1;
  }

  selectTouchBus();

  return zoneID;
}

// ============================================================
// ZONE NAME LOOKUP
// ============================================================

bool getZoneName(
  uint8_t zoneID,
  char* output,
  size_t outputSize
) {

  if (!timezoneDatabaseReady) {
    return false;
  }

  selectSDBus();

  File f = SD.open(TZ_ZONES_FILE, FILE_READ);

  if (!f) {
    selectTouchBus();
    return false;
  }

  if (!checkMagic(f, "TZON")) {
    f.close();
    selectTouchBus();
    return false;
  }

  readU16(f); // version

  uint16_t zoneCount = readU16(f);

  if (zoneID >= zoneCount) {
    f.close();
    selectTouchBus();
    return false;
  }

  for (uint16_t i = 0; i < zoneCount; i++) {

    uint8_t length = readU8(f);

    if (i == zoneID) {

      size_t copyLength = min(
        (size_t)length,
        outputSize - 1
      );

      f.read(
        (uint8_t*)output,
        copyLength
      );

      output[copyLength] = '\0';

      f.close();
      selectTouchBus();

      return true;
    }

    f.seek(
      f.position() + length
    );
  }

  f.close();
  selectTouchBus();

  return false;
}

// ============================================================
// TIMEZONE RULE LOOKUP
// ============================================================

bool lookupTimeRule(
  uint8_t targetZone,
  time_t utcEpoch,
  int32_t &offsetSeconds,
  char* abbreviation,
  size_t abbreviationSize,
  time_t &nextTransition
) {

  if (!timezoneDatabaseReady) {
    return false;
  }

  selectSDBus();

  File f = SD.open(TZ_RULES_FILE, FILE_READ);

  if (!f) {
    selectTouchBus();
    return false;
  }

  if (!checkMagic(f, "TZRL")) {
    f.close();
    selectTouchBus();
    return false;
  }

  readU16(f); // version
  readU16(f); // start year
  readU16(f); // end year

  uint16_t zoneCount = readU16(f);
  uint8_t abbreviationCount = readU8(f);

  if (targetZone >= zoneCount || abbreviationCount > 80) {
    f.close();
    selectTouchBus();
    return false;
  }

  char abbreviations[80][16];

  for (uint8_t i = 0; i < abbreviationCount; i++) {

    uint8_t length = readU8(f);

    uint8_t copyLength = min(
      length,
      (uint8_t)15
    );

    f.read(
      (uint8_t*)abbreviations[i],
      copyLength
    );

    abbreviations[i][copyLength] = '\0';

    if (length > copyLength) {
      f.seek(
        f.position()
        + length
        - copyLength
      );
    }
  }

  for (uint16_t zone = 0; zone < zoneCount; zone++) {

    uint16_t transitionCount = readU16(f);

    if (zone != targetZone) {

      f.seek(
        f.position()
        + (uint32_t)transitionCount * 13UL
      );

      continue;
    }

    bool found = false;
    int32_t selectedOffset = 0;
    uint8_t selectedAbbreviation = 0;
    nextTransition = 0;

    for (uint16_t i = 0; i < transitionCount; i++) {

      int64_t transitionEpoch = readI64(f);
      int32_t transitionOffset = readI32(f);
      uint8_t abbreviationID = readU8(f);

      if (transitionEpoch <= (int64_t)utcEpoch) {

        selectedOffset = transitionOffset;
        selectedAbbreviation = abbreviationID;
        found = true;
      }

      else if (nextTransition == 0) {

        nextTransition = (time_t)transitionEpoch;
      }
    }

    if (!found) {
      f.close();
      selectTouchBus();
      return false;
    }

    offsetSeconds = selectedOffset;

    if (selectedAbbreviation < abbreviationCount) {

      strncpy(
        abbreviation,
        abbreviations[selectedAbbreviation],
        abbreviationSize - 1
      );

      abbreviation[abbreviationSize - 1] = '\0';
    }

    else {
      strncpy(abbreviation, "?", abbreviationSize - 1);
      abbreviation[abbreviationSize - 1] = '\0';
    }

    f.close();
    selectTouchBus();

    return true;
  }

  f.close();
  selectTouchBus();

  return false;
}

// ============================================================
// DISTANCE / PLACE RANKING
// ============================================================

double distanceKm(
  double lat1,
  double lon1,
  double lat2,
  double lon2
) {

  double lat1r = radians(lat1);
  double lon1r = radians(lon1);
  double lat2r = radians(lat2);
  double lon2r = radians(lon2);

  double dlat = lat2r - lat1r;
  double dlon = lon2r - lon1r;

  double a =
      sin(dlat / 2.0) * sin(dlat / 2.0)
    + cos(lat1r) * cos(lat2r)
      * sin(dlon / 2.0) * sin(dlon / 2.0);

  double c =
    2.0
    * atan2(
        sqrt(a),
        sqrt(1.0 - a)
      );

  return EARTH_RADIUS_KM * c;
}

int featurePriority(
  const char* feature
) {

  if (strcmp(feature, "PPLC") == 0) return 0;
  if (strcmp(feature, "PPLA") == 0) return 1;
  if (strcmp(feature, "PPLA2") == 0) return 2;
  if (strcmp(feature, "PPLA3") == 0) return 3;
  if (strcmp(feature, "PPLA4") == 0) return 4;
  if (strcmp(feature, "PPL") == 0) return 5;
  if (strcmp(feature, "PPLG") == 0) return 6;
  if (strcmp(feature, "PPLS") == 0) return 6;
  if (strcmp(feature, "PPLL") == 0) return 8;
  if (strcmp(feature, "PPLF") == 0) return 8;
  if (strcmp(feature, "PPLX") == 0) return 12;

  return 10;
}

double placeScore(
  const PlaceRecord &place,
  double distance
) {

  int priority = featurePriority(place.feature);

  double population = max(
    (double)place.population,
    1.0
  );

  double populationBonus = log10(population);

  return
      distance
    + priority * 0.75
    - populationBonus * 0.15;
}

// ============================================================
// PLACE INDEX / RECORD READERS
// ============================================================

bool readPlaceIndexCell(
  File &indexFile,
  int row,
  int col,
  uint32_t &placeOffset,
  uint32_t &count
) {

  if (row < 0 || row >= placeGridRows) {
    placeOffset = 0;
    count = 0;
    return true;
  }

  while (col < 0) {
    col += placeGridCols;
  }

  while (col >= placeGridCols) {
    col -= placeGridCols;
  }

  uint32_t cellNumber =
    (uint32_t)row
    * placeGridCols
    + col;

  uint32_t fileOffset =
    PLACE_INDEX_HEADER_SIZE
    + cellNumber * 8UL;

  if (!indexFile.seek(fileOffset)) {
    return false;
  }

  placeOffset = readU32(indexFile);
  count = readU32(indexFile);

  return true;
}

bool readPlaceRecord(
  File &f,
  PlaceRecord &place
) {

  int32_t latScaled = readI32(f);
  int32_t lonScaled = readI32(f);

  place.population = readU32(f);
  place.geonameID = readU32(f);

  if (!f.available()) {
    return false;
  }

  uint8_t countryLength = readU8(f);

  uint8_t countryCopy = min(
    countryLength,
    (uint8_t)(sizeof(place.country) - 1)
  );

  if (
    f.read((uint8_t*)place.country, countryCopy)
    != countryCopy
  ) {
    return false;
  }

  place.country[countryCopy] = '\0';

  if (countryLength > countryCopy) {
    f.seek(
      f.position()
      + countryLength
      - countryCopy
    );
  }

  uint8_t featureLength = readU8(f);

  uint8_t featureCopy = min(
    featureLength,
    (uint8_t)(sizeof(place.feature) - 1)
  );

  if (
    f.read((uint8_t*)place.feature, featureCopy)
    != featureCopy
  ) {
    return false;
  }

  place.feature[featureCopy] = '\0';

  if (featureLength > featureCopy) {
    f.seek(
      f.position()
      + featureLength
      - featureCopy
    );
  }

  uint16_t nameLength = readU16(f);

  uint16_t nameCopy = min(
    nameLength,
    (uint16_t)(sizeof(place.name) - 1)
  );

  if (
    f.read((uint8_t*)place.name, nameCopy)
    != nameCopy
  ) {
    return false;
  }

  place.name[nameCopy] = '\0';

  if (nameLength > nameCopy) {
    f.seek(
      f.position()
      + nameLength
      - nameCopy
    );
  }

  place.latitude = latScaled / 1000000.0;
  place.longitude = lonScaled / 1000000.0;

  return true;
}

void calculatePlaceSearchBounds(
  double latitude,
  double longitude,
  int &rowMin,
  int &rowMax,
  int &colMin,
  int &colMax
) {

  const double KM_PER_DEG_LAT = 111.32;

  double latDegrees =
    MAX_PLACE_DISTANCE_KM
    / KM_PER_DEG_LAT;

  double cosLat = cos(radians(latitude));

  if (fabs(cosLat) < 0.01) {
    cosLat = 0.01;
  }

  double kmPerDegLon =
    111.32 * fabs(cosLat);

  double lonDegrees =
    MAX_PLACE_DISTANCE_KM
    / kmPerDegLon;

  double minLat = latitude - latDegrees;
  double maxLat = latitude + latDegrees;
  double minLon = longitude - lonDegrees;
  double maxLon = longitude + lonDegrees;

  rowMin = floor(
    (minLat + 90.0)
    / placeCellDegrees
  );

  rowMax = floor(
    (maxLat + 90.0)
    / placeCellDegrees
  );

  colMin = floor(
    (minLon + 180.0)
    / placeCellDegrees
  );

  colMax = floor(
    (maxLon + 180.0)
    / placeCellDegrees
  );

  rowMin = constrain(rowMin, 0, placeGridRows - 1);
  rowMax = constrain(rowMax, 0, placeGridRows - 1);
}

bool findBestPlace(
  double latitude,
  double longitude,
  PlaceRecord &bestPlace,
  double &bestDistance,
  double &bestScore
) {

  if (!placeDatabaseReady) {
    return false;
  }

  int rowMin;
  int rowMax;
  int colMin;
  int colMax;

  calculatePlaceSearchBounds(
    latitude,
    longitude,
    rowMin,
    rowMax,
    colMin,
    colMax
  );

  selectSDBus();

  File indexFile = SD.open(PLACES_INDEX_FILE, FILE_READ);
  File placesFile = SD.open(PLACES_FILE, FILE_READ);

  if (!indexFile || !placesFile) {
    if (indexFile) indexFile.close();
    if (placesFile) placesFile.close();
    selectTouchBus();
    return false;
  }

  bool found = false;
  bestDistance = 999999.0;
  bestScore = 999999.0;

  uint32_t cellsChecked = 0;
  uint32_t placesChecked = 0;

  unsigned long startMillis = millis();

  for (int row = rowMin; row <= rowMax; row++) {

    for (int col = colMin; col <= colMax; col++) {

      cellsChecked++;

      uint32_t placeOffset;
      uint32_t count;

      if (!readPlaceIndexCell(
        indexFile,
        row,
        col,
        placeOffset,
        count
      )) {
        continue;
      }

      if (count == 0) {
        continue;
      }

      if (!placesFile.seek(placeOffset)) {
        continue;
      }

      for (uint32_t i = 0; i < count; i++) {

        PlaceRecord place;

        if (!readPlaceRecord(placesFile, place)) {
          break;
        }

        placesChecked++;

        double distance = distanceKm(
          latitude,
          longitude,
          place.latitude,
          place.longitude
        );

        if (distance > MAX_PLACE_DISTANCE_KM) {
          continue;
        }

        double score = placeScore(
          place,
          distance
        );

        if (!found || score < bestScore) {

          found = true;
          bestPlace = place;
          bestDistance = distance;
          bestScore = score;
        }

        else if (
          fabs(score - bestScore) < 0.001
          && place.population > bestPlace.population
        ) {

          bestPlace = place;
          bestDistance = distance;
          bestScore = score;
        }
      }
    }
  }

  indexFile.close();
  placesFile.close();

  unsigned long elapsed = millis() - startMillis;

  selectTouchBus();

  Serial.print("Place lookup: ");
  Serial.print(cellsChecked);
  Serial.print(" cells, ");
  Serial.print(placesChecked);
  Serial.print(" records, ");
  Serial.print(elapsed);
  Serial.println(" ms");

  return found;
}

// ============================================================
// RESOLVE CURRENT GEOGRAPHIC CONTEXT
// ============================================================

void resolveGeographicContextIfNeeded(
  bool force
) {

  if (!storedPositionValid) {
    return;
  }

  if (!timezoneDatabaseReady) {

    geoContextValid = false;
    currentZoneNautical = false;
    currentZoneID = -1;
    currentUtcOffsetSeconds = 0;
    strcpy(currentZoneAbbrev, "UTC");
    strcpy(currentZoneName, "UTC");
    currentTimeRuleValid = true;
    nextTimeRuleTransition = 0;

    return;
  }

  if (!force && geoContextValid) {

    double moved = distanceKm(
      resolvedLocationLat,
      resolvedLocationLon,
      storedLatitude,
      storedLongitude
    );

    if (moved < LOCATION_RELOOKUP_DISTANCE_KM) {

      if (systemTimeValid) {
        updateCurrentTimeRule(time(nullptr));
      }

      return;
    }
  }

  bool nautical = false;
  int nauticalOffset = 0;

  int zoneID = lookupZoneID(
    storedLatitude,
    storedLongitude,
    nautical,
    nauticalOffset
  );

  currentZoneNautical = nautical;
  currentZoneID = zoneID;
  currentPlaceValid = false;
  currentPlaceName[0] = '\0';
  currentPlaceCountry[0] = '\0';
  currentMarineValid = false;
  currentMarineName[0] = '\0';

  if (nautical) {

    currentUtcOffsetSeconds =
      nauticalOffset * 3600;

    formatUTCOffset(
      currentUtcOffsetSeconds,
      currentZoneAbbrev,
      sizeof(currentZoneAbbrev)
    );

    strcpy(currentZoneName, "NAUTICAL");

    currentTimeRuleValid = true;
    nextTimeRuleTransition = 0;
  }

  else if (zoneID >= 0) {

    getZoneName(
      (uint8_t)zoneID,
      currentZoneName,
      sizeof(currentZoneName)
    );

    currentTimeRuleValid = false;

    if (systemTimeValid) {
      updateCurrentTimeRule(
        time(nullptr),
        true
      );
    }
  }

  else {

    // Database lookup failed. UTC is a safe fallback.
    currentZoneNautical = false;
    currentUtcOffsetSeconds = 0;
    strcpy(currentZoneAbbrev, "UTC");
    strcpy(currentZoneName, "UTC");
    currentTimeRuleValid = true;
    nextTimeRuleTransition = 0;
  }

  // Locality lookup. If nothing credible exists within 10 km,
  // maritime locations display AT SEA rather than a distant town.
  if (placeDatabaseReady) {

    PlaceRecord place;
    double distance;
    double score;

    if (findBestPlace(
      storedLatitude,
      storedLongitude,
      place,
      distance,
      score
    )) {

      currentPlaceValid = true;

      strncpy(
        currentPlaceName,
        place.name,
        sizeof(currentPlaceName) - 1
      );

      currentPlaceName[
        sizeof(currentPlaceName) - 1
      ] = '\0';

      strncpy(
        currentPlaceCountry,
        place.country,
        sizeof(currentPlaceCountry) - 1
      );

      currentPlaceCountry[
        sizeof(currentPlaceCountry) - 1
      ] = '\0';

      currentPlaceDistanceKm = distance;
    }
  }

  // If no populated locality is within 10 km, try the named
  // marine polygon database. This covers oceans, seas, bays,
  // gulfs, sounds, straits, and similar named water bodies.
  if (!currentPlaceValid && marineDatabaseReady) {

    char marineName[96];

    if (lookupMarineArea(
      storedLatitude,
      storedLongitude,
      marineName,
      sizeof(marineName)
    )) {

      currentMarineValid = true;

      strncpy(
        currentMarineName,
        marineName,
        sizeof(currentMarineName) - 1
      );

      currentMarineName[
        sizeof(currentMarineName) - 1
      ] = '\0';
    }
  }

  resolvedLocationLat = storedLatitude;
  resolvedLocationLon = storedLongitude;
  geoContextValid = true;

  Serial.println("Geographic context resolved:");
  Serial.print("  Zone: ");
  Serial.println(currentZoneName);
  Serial.print("  Abbrev: ");
  Serial.println(currentZoneAbbrev);
  Serial.print("  Location: ");
  Serial.println(
    currentPlaceValid
      ? currentPlaceName
      : (currentMarineValid
          ? currentMarineName
          : (currentZoneNautical ? "AT SEA" : "NO LOCALITY"))
  );

  if (
    currentPage == PAGE_CLOCK
    && !alarmActive
  ) {
    drawLocalHeader();
  }
}

// ============================================================
// CURRENT UTC OFFSET / DST RULE
// ============================================================

void updateCurrentTimeRule(
  time_t utcEpoch,
  bool force
) {

  if (currentZoneNautical) {
    return;
  }

  if (currentZoneID < 0) {
    return;
  }

  if (
    !force
    && currentTimeRuleValid
    && (
      nextTimeRuleTransition == 0
      || utcEpoch < nextTimeRuleTransition
    )
  ) {
    return;
  }

  int32_t offsetSeconds = 0;
  char abbreviation[16] = "UTC";
  time_t nextTransition = 0;

  if (lookupTimeRule(
    (uint8_t)currentZoneID,
    utcEpoch,
    offsetSeconds,
    abbreviation,
    sizeof(abbreviation),
    nextTransition
  )) {

    bool headerChanged =
      !currentTimeRuleValid
      || offsetSeconds != currentUtcOffsetSeconds
      || strcmp(abbreviation, currentZoneAbbrev) != 0;

    currentUtcOffsetSeconds = offsetSeconds;

    strncpy(
      currentZoneAbbrev,
      abbreviation,
      sizeof(currentZoneAbbrev) - 1
    );

    currentZoneAbbrev[
      sizeof(currentZoneAbbrev) - 1
    ] = '\0';

    currentTimeRuleValid = true;
    nextTimeRuleTransition = nextTransition;

    if (
      headerChanged
      && currentPage == PAGE_CLOCK
      && !alarmActive
    ) {
      drawLocalHeader();
    }
  }
}

// ============================================================
// UTC -> LOCAL CIVIL TIME
// ============================================================

void getLocalTimeForEpoch(
  time_t utcEpoch,
  struct tm &localTime
) {

  if (geoContextValid) {
    updateCurrentTimeRule(utcEpoch);
  }

  int32_t offset =
    currentTimeRuleValid
      ? currentUtcOffsetSeconds
      : 0;

  time_t localEpoch =
    utcEpoch + offset;

  // gmtime_r is intentional: localEpoch has already been shifted
  // by the resolved civil UTC offset.
  gmtime_r(
    &localEpoch,
    &localTime
  );
}

// ============================================================
// UTC OFFSET STRING
// ============================================================

void formatUTCOffset(
  int32_t seconds,
  char* output,
  size_t outputSize
) {

  if (seconds == 0) {
    snprintf(output, outputSize, "UTC");
    return;
  }

  char sign =
    seconds >= 0
      ? '+'
      : '-';

  int32_t absoluteSeconds =
    seconds >= 0
      ? seconds
      : -seconds;

  int hours =
    absoluteSeconds / 3600;

  int minutes =
    (absoluteSeconds % 3600) / 60;

  if (minutes == 0) {

    snprintf(
      output,
      outputSize,
      "UTC%c%d",
      sign,
      hours
    );
  }

  else {

    snprintf(
      output,
      outputSize,
      "UTC%c%d:%02d",
      sign,
      hours,
      minutes
    );
  }
}


