// ============================================================
// CLOCK SERVICE
// ============================================================

void serviceClockDisplay() {

  if (
    alarmActive
  ) {

    return;
  }

  if (
    !systemTimeValid
  ) {

    return;
  }

  time_t now =
    time(nullptr);

  if (
    now ==
    lastDisplayedSecond
  ) {

    return;
  }

  lastDisplayedSecond =
    now;

  updateNightMode();

  if (
    currentPage ==
    PAGE_CLOCK
  ) {

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
  }
}

// ============================================================
// NIGHT MODE
// ============================================================

void updateNightMode() {

  if (
    !systemTimeValid
  ) {

    return;
  }

  bool newRedMode =
    redMode;

  // ----------------------------------------------------------
  // NIGHT MODE: FORCED ON
  // ----------------------------------------------------------

  if (
    nightSetting == 1
  ) {

    newRedMode =
      true;

    ldrDarkConfirmCount =
      0;

    ldrLightConfirmCount =
      0;
  }

  // ----------------------------------------------------------
  // NIGHT MODE: FORCED OFF
  // ----------------------------------------------------------

  else if (
    nightSetting == 2
  ) {

    newRedMode =
      false;

    ldrDarkConfirmCount =
      0;

    ldrLightConfirmCount =
      0;
  }

  // ----------------------------------------------------------
  // NIGHT MODE: AUTO
  //
  // R21 is the primary control.
  // The configured START/END schedule is retained as fallback
  // if the light-sensor reading is clearly invalid.
  // ----------------------------------------------------------

  else {

    uint32_t mvTotal =
      0;

    for (
      int i = 0;
      i < LDR_SAMPLE_COUNT;
      i++
    ) {

      mvTotal +=
        analogReadMilliVolts(
          LDR_PIN
        );
    }

    int lightMv =
      (int)(
        mvTotal /
        LDR_SAMPLE_COUNT
      );

    ldrLastMilliVolts =
      lightMv;

    ldrSensorValid =
      (
        lightMv >=
          LDR_VALID_MIN_MV
        &&
        lightMv <=
          LDR_VALID_MAX_MV
      );

    if (
      ldrSensorValid
    ) {

      // DARK: require the condition to remain above the
      // dark threshold for several consecutive seconds.
      if (
        lightMv >=
        LDR_DARK_THRESHOLD_MV
      ) {

        ldrLightConfirmCount =
          0;

        if (
          ldrDarkConfirmCount <
          LDR_CONFIRM_SECONDS
        ) {

          ldrDarkConfirmCount++;
        }

        if (
          ldrDarkConfirmCount >=
          LDR_CONFIRM_SECONDS
        ) {

          newRedMode =
            true;
        }
      }

      // LIGHT: require the condition to remain below the
      // light threshold for several consecutive seconds.
      else if (
        lightMv <=
        LDR_LIGHT_THRESHOLD_MV
      ) {

        ldrDarkConfirmCount =
          0;

        if (
          ldrLightConfirmCount <
          LDR_CONFIRM_SECONDS
        ) {

          ldrLightConfirmCount++;
        }

        if (
          ldrLightConfirmCount >=
          LDR_CONFIRM_SECONDS
        ) {

          newRedMode =
            false;
        }
      }

      // HYSTERESIS BAND:
      // 180-220 mV retains the existing display state.
      else {

        ldrDarkConfirmCount =
          0;

        ldrLightConfirmCount =
          0;
      }
    }

    // --------------------------------------------------------
    // SENSOR FALLBACK: SAVED TIME SCHEDULE
    // --------------------------------------------------------

    else {

      ldrDarkConfirmCount =
        0;

      ldrLightConfirmCount =
        0;

      time_t now =
        time(nullptr);

      struct tm localTime;

      getLocalTimeForEpoch(
        now,
        localTime
      );

      int nowMinutes =
        localTime.tm_hour * 60 +
        localTime.tm_min;

      int startMinutes =
        nightStartHour * 60 +
        nightStartMinute;

      int endMinutes =
        nightEndHour * 60 +
        nightEndMinute;

      if (
        startMinutes ==
        endMinutes
      ) {

        newRedMode =
          false;
      }

      else if (
        startMinutes <
        endMinutes
      ) {

        newRedMode =
          nowMinutes >= startMinutes &&
          nowMinutes < endMinutes;
      }

      else {

        newRedMode =
          nowMinutes >= startMinutes ||
          nowMinutes < endMinutes;
      }
    }
  }

  if (
    newRedMode ==
    redMode
  ) {

    return;
  }

  redMode =
    newRedMode;

  lastGpsDisplay[0] =
    '\0';

  lastGridDisplay[0] =
    '\0';

  if (
    currentPage ==
    PAGE_CLOCK
  ) {

    drawClockScreen();
  }

  else if (
    currentPage ==
    PAGE_OPTIONS
  ) {

    drawOptionsScreen();
  }

  else if (
    currentPage ==
    PAGE_ALARM
  ) {

    drawAlarmScreen();
  }

  else if (
    currentPage ==
    PAGE_SET_TIME
  ) {

    drawSetTimeScreen();
  }

  else if (
    currentPage ==
    PAGE_CALLSIGN
  ) {

    drawCallsignScreen();
  }

  else if (
    currentPage ==
    PAGE_NIGHT_SETTINGS
  ) {

    drawNightSettingsScreen();
  }
}

