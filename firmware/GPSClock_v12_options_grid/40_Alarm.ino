// ============================================================
// ALARM OCCURRENCE KEY
// ============================================================

long long makeAlarmOccurrenceKey(
  struct tm &localTime
) {

  return
      (long long)(localTime.tm_year + 1900)
        * 100000000LL

    + (long long)(localTime.tm_mon + 1)
        * 1000000LL

    + (long long)localTime.tm_mday
        * 10000LL

    + (long long)localTime.tm_hour
        * 100LL

    + (long long)localTime.tm_min;
}

void resetAlarmOccurrenceLock() {

  lastAlarmOccurrenceKey =
    -1;
}

// ============================================================
// ALARM SERVICE
// ============================================================

void serviceAlarm() {

  if (
    !systemTimeValid
  ) {

    return;
  }

  time_t now =
    time(nullptr);

  if (
    alarmActive
  ) {

    updateAlarmFlash();

    if (
      millis() -
      alarmStartedMillis >=
      ALARM_TIMEOUT_MS
    ) {

      stopAlarm();
    }

    return;
  }

  if (
    snoozeActive &&
    now >=
      snoozeUntilEpoch
  ) {

    snoozeActive =
      false;

    startAlarm();

    return;
  }

  if (
    !alarmEnabled
  ) {

    return;
  }

  struct tm localTime;

  getLocalTimeForEpoch(
    now,
    localTime
  );

  long long occurrenceKey =
    makeAlarmOccurrenceKey(
      localTime
    );

  if (
    localTime.tm_hour ==
      alarmHour &&

    localTime.tm_min ==
      alarmMinute &&

    occurrenceKey !=
      lastAlarmOccurrenceKey
  ) {

    lastAlarmOccurrenceKey =
      occurrenceKey;

    startAlarm();
  }
}

// ============================================================
// START ALARM
// ============================================================

void startAlarm() {

  alarmActive =
    true;

  alarmStartedMillis =
    millis();

  lastAlarmFlashMillis =
    millis();

  alarmFlashRed =
    true;

  Serial.println(
    "ALARM TRIGGERED"
  );

  drawAlarmActiveScreen();
}

// ============================================================
// STOP ALARM
// ============================================================

void stopAlarm() {

  alarmActive =
    false;

  snoozeActive =
    false;

  currentPage =
    PAGE_CLOCK;

  lastGpsDisplay[0] =
    '\0';

  lastGridDisplay[0] =
    '\0';

  drawClockScreen();

  lastDisplayedSecond =
    -1;
}

// ============================================================
// SNOOZE
// ============================================================

void snoozeAlarm() {

  if (
    !systemTimeValid
  ) {

    stopAlarm();

    return;
  }

  alarmActive =
    false;

  snoozeActive =
    true;

  snoozeUntilEpoch =
    time(nullptr) +
    SNOOZE_SECONDS;

  currentPage =
    PAGE_CLOCK;

  lastGpsDisplay[0] =
    '\0';

  lastGridDisplay[0] =
    '\0';

  drawClockScreen();

  lastDisplayedSecond =
    -1;
}

// ============================================================
// ALARM FLASH
// ============================================================

void updateAlarmFlash() {

  unsigned long nowMillis =
    millis();

  if (
    nowMillis -
    lastAlarmFlashMillis <
    ALARM_FLASH_MS
  ) {

    return;
  }

  lastAlarmFlashMillis =
    nowMillis;

  alarmFlashRed =
    !alarmFlashRed;

  drawAlarmActiveScreen();
}

// ============================================================
// ALARM / SNOOZE INDICATOR
// ============================================================

void drawAlarmIndicator() {

  tft.fillRect(
    168,
    0,
    43,
    20,
    backgroundColor()
  );

  if (
    !alarmEnabled
  ) {

    return;
  }

  uint16_t c =
    primaryColor();

  drawBellIcon(
    179,
    9,
    c
  );

  if (
    snoozeActive
  ) {

    tft.setFreeFont(
      NULL
    );

    tft.setTextDatum(
      TL_DATUM
    );

    tft.setTextColor(
      c
    );

    tft.drawString(
      "Zzz",
      189,
      2,
      1
    );
  }
}


