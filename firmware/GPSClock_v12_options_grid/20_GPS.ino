// ============================================================
// GPS FRESHNESS
// ============================================================

bool gpsHasFreshFix() {

  return
    gps.location.isValid() &&
    gps.location.age() <
      GPS_FIX_MAX_AGE_MS;
}

bool gpsHasFreshTime() {

  return
    gps.date.isValid() &&
    gps.time.isValid() &&
    gps.date.age() <
      GPS_TIME_MAX_AGE_MS &&
    gps.time.age() <
      GPS_TIME_MAX_AGE_MS;
}

// ============================================================
// GPS SERIAL SERVICE
// ============================================================

void serviceGPS() {

  if (
    gpsState ==
    GPS_SLEEP
  ) {

    return;
  }

  while (
    GPS.available()
  ) {

    gps.encode(
      GPS.read()
    );
  }

  if (
    gps.location.isUpdated() &&
    gpsHasFreshFix()
  ) {

    processStableGPSFixSample();
  }

  if (
    gps.time.isUpdated() &&
    gpsHasFreshTime()
  ) {

    pendingGpsEpoch =
      gpsDateTimeToEpoch();

    pendingGpsSync =
      true;
  }
}

// ============================================================
// PPS SERVICE
// ============================================================

void servicePPS() {

  if (
    !ppsFlag
  ) {

    return;
  }

  ppsFlag =
    false;

  if (
    gpsState !=
    GPS_AWAKE
  ) {

    return;
  }

  if (
    pendingGpsSync
  ) {

    setSystemEpoch(
      pendingGpsEpoch + 1
    );

    pendingGpsSync =
      false;

    gpsTimeSyncedThisCycle =
      true;
  }
}

// ============================================================
// GPS DUTY CYCLE
// ============================================================

void serviceGPSDutyCycle() {

  if (
    gpsState ==
    GPS_SLEEP
  ) {

    if (
      millis() -
      gpsSleepMillis >=
      gpsSleepIntervalMs
    ) {

      wakeGPS();
    }

    return;
  }

  if (
    gpsTimeSyncedThisCycle &&
    gpsPositionReady
  ) {

    finishGPSCycle();

    return;
  }

  if (
    millis() -
    gpsWakeMillis >=
    GPS_ACQUIRE_TIMEOUT_MS
  ) {

    // Only cache a position that has passed the stability test.
    if (stableGpsFixReady) {

      cacheCurrentGPSPosition();
      lastGpsCyclePositionSynced = true;
    }
    else {
      lastGpsCyclePositionSynced = false;
    }

    // A failed or marginal acquisition retries after two minutes.
    // A successful stable position uses the normal 30-minute interval.
    if (stableGpsFixReady) {
      sleepGPSFor(GPS_SLEEP_INTERVAL_MS);
    }
    else {
      sleepGPSFor(GPS_NO_FIX_RETRY_INTERVAL_MS);
    }
  }
}

// ============================================================
// WAKE GPS
// ============================================================

void wakeGPS() {

  digitalWrite(
    GPS_ENABLE_PIN,
    HIGH
  );

  gpsState =
    GPS_AWAKE;

  gpsWakeMillis =
    millis();

  gpsTimeSyncedThisCycle =
    false;

  gpsPositionReady =
    false;

  pendingGpsSync =
    false;

  resetStableGPSFixAccumulator();

  Serial.println(
    "GPS WAKE"
  );

  lastGpsDisplay[0] =
    '\0';

  if (
    currentPage ==
      PAGE_CLOCK &&
    !alarmActive
  ) {

    drawGPSLine(
      true
    );
  }
}

// ============================================================
// SLEEP GPS
// ============================================================

void sleepGPS() {

  sleepGPSFor(
    GPS_SLEEP_INTERVAL_MS
  );
}

void sleepGPSFor(
  unsigned long intervalMs
) {

  digitalWrite(
    GPS_ENABLE_PIN,
    LOW
  );

  gpsState =
    GPS_SLEEP;

  gpsSleepMillis =
    millis();

  gpsSleepIntervalMs =
    intervalMs;

  gpsTimeSyncedThisCycle =
    false;

  gpsPositionReady =
    false;

  pendingGpsSync =
    false;

  Serial.print(
    "GPS STANDBY - next retry in "
  );

  Serial.print(
    gpsSleepIntervalMs / 60000UL
  );

  Serial.println(
    " min"
  );

  lastGpsDisplay[0] =
    '\0';

  if (
    currentPage ==
      PAGE_CLOCK &&
    !alarmActive
  ) {

    drawGPSLine(
      true
    );
  }
}

// ============================================================
// FINISH GPS CYCLE
// ============================================================

void finishGPSCycle() {

  if (stableGpsFixReady) {

    cacheCurrentGPSPosition();
    lastGpsCyclePositionSynced = true;
  }

  sleepGPSFor(
    GPS_SLEEP_INTERVAL_MS
  );
}

// ============================================================
// STABLE GPS FIX
// ============================================================

void resetStableGPSFixAccumulator() {

  gpsStableFixCount = 0;
  gpsStableLastLatitude = 0.0;
  gpsStableLastLongitude = 0.0;
  gpsStableLatitudeSum = 0.0;
  gpsStableLongitudeSum = 0.0;
  gpsStableAltitudeSum = 0.0;
  gpsStableAltitudeSamples = 0;
  gpsStableSatellites = 0;

  stableGpsFixReady = false;
}

void processStableGPSFixSample() {

  if (!gpsHasFreshFix()) {
    return;
  }

  int satellites =
    gps.satellites.isValid()
      ? gps.satellites.value()
      : 0;

  if (satellites < GPS_STABLE_FIX_MIN_SATELLITES) {
    resetStableGPSFixAccumulator();
    return;
  }

  double latitude = gps.location.lat();
  double longitude = gps.location.lng();

  if (gpsStableFixCount > 0) {

    double stepKm = distanceKm(
      gpsStableLastLatitude,
      gpsStableLastLongitude,
      latitude,
      longitude
    );

    if (stepKm > GPS_STABLE_FIX_MAX_STEP_KM) {

      Serial.print("GPS fix unstable, step ");
      Serial.print(stepKm, 3);
      Serial.println(" km - restarting stability count");

      resetStableGPSFixAccumulator();
    }
  }

  gpsStableLastLatitude = latitude;
  gpsStableLastLongitude = longitude;
  gpsStableLatitudeSum += latitude;
  gpsStableLongitudeSum += longitude;
  gpsStableFixCount++;

  if (gps.altitude.isValid() && gps.altitude.age() < GPS_FIX_MAX_AGE_MS) {
    gpsStableAltitudeSum += gps.altitude.feet();
    gpsStableAltitudeSamples++;
  }

  gpsStableSatellites = satellites;

  Serial.print("GPS stable fix sample ");
  Serial.print(gpsStableFixCount);
  Serial.print("/");
  Serial.println(GPS_STABLE_FIX_COUNT_REQUIRED);

  if (gpsStableFixCount >= GPS_STABLE_FIX_COUNT_REQUIRED) {

    stableGpsLatitude =
      gpsStableLatitudeSum / gpsStableFixCount;

    stableGpsLongitude =
      gpsStableLongitudeSum / gpsStableFixCount;

    stableGpsAltitude =
      gpsStableAltitudeSamples > 0
        ? gpsStableAltitudeSum / gpsStableAltitudeSamples
        : storedAltitude;

    stableGpsSatellites =
      gpsStableSatellites;

    stableGpsFixReady = true;
    gpsPositionReady = true;

    Serial.print("GPS STABLE FIX: ");
    Serial.print(stableGpsLatitude, 6);
    Serial.print(", ");
    Serial.print(stableGpsLongitude, 6);
    Serial.print(" SAT:");
    Serial.println(stableGpsSatellites);
  }
}

// ============================================================
// GPS UTC -> EPOCH
// ============================================================

time_t gpsDateTimeToEpoch() {

  struct tm utc = {};

  utc.tm_year =
    gps.date.year() - 1900;

  utc.tm_mon =
    gps.date.month() - 1;

  utc.tm_mday =
    gps.date.day();

  utc.tm_hour =
    gps.time.hour();

  utc.tm_min =
    gps.time.minute();

  utc.tm_sec =
    gps.time.second();

  setenv(
    "TZ",
    "UTC0",
    1
  );

  tzset();

  time_t epoch =
    mktime(
      &utc
    );

  return epoch;
}


