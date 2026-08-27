// ============================================================
// LOAD SETTINGS
// ============================================================

void loadSettings() {

  prefs.begin(
    "hamclock",
    true
  );

  local24Hour =
    prefs.getBool(
      "local24",
      false
    );

  lightTheme =
    prefs.getBool(
      "light",
      false
    );

  brightnessLevel =
    prefs.getUChar(
      "bright",
      3
    );

  nightSetting =
    prefs.getUChar(
      "night",
      0
    );

  nightStartHour =
    prefs.getInt(
      "nightSH",
      20
    );

  nightStartMinute =
    prefs.getInt(
      "nightSM",
      0
    );

  nightEndHour =
    prefs.getInt(
      "nightEH",
      7
    );

  nightEndMinute =
    prefs.getInt(
      "nightEM",
      0
    );

  coordinateDMS =
    prefs.getBool(
      "coordDMS",
      false
    );

  alarmEnabled =
    prefs.getBool(
      "alarmEn",
      false
    );

  alarmHour =
    prefs.getInt(
      "alarmHr",
      7
    );

  alarmMinute =
    prefs.getInt(
      "alarmMin",
      0
    );

  String storedCall =
    prefs.getString(
      "callsign",
      DEFAULT_CALLSIGN
    );

  storedCall.trim();
  storedCall.toUpperCase();

  if (storedCall.length() == 0) {
    storedCall = DEFAULT_CALLSIGN;
  }

  storedCall.toCharArray(
    callSign,
    sizeof(callSign)
  );

  prefs.end();

  alarmHour =
    constrain(
      alarmHour,
      0,
      23
    );

  alarmMinute =
    constrain(
      alarmMinute,
      0,
      59
    );

  if (
    nightSetting > 2
  ) {

    nightSetting = 0;
  }

  if (
    brightnessLevel > 3
  ) {

    brightnessLevel = 3;
  }

  nightStartHour = constrain(nightStartHour, 0, 23);
  nightStartMinute = constrain(nightStartMinute, 0, 59);
  nightEndHour = constrain(nightEndHour, 0, 23);
  nightEndMinute = constrain(nightEndMinute, 0, 59);
}

// ============================================================
// SAVE SETTINGS
// ============================================================

void saveSettings() {

  prefs.begin(
    "hamclock",
    false
  );

  prefs.putBool(
    "local24",
    local24Hour
  );

  prefs.putBool(
    "light",
    lightTheme
  );

  prefs.putUChar(
    "bright",
    brightnessLevel
  );

  prefs.putUChar(
    "night",
    nightSetting
  );

  prefs.putInt(
    "nightSH",
    nightStartHour
  );

  prefs.putInt(
    "nightSM",
    nightStartMinute
  );

  prefs.putInt(
    "nightEH",
    nightEndHour
  );

  prefs.putInt(
    "nightEM",
    nightEndMinute
  );

  prefs.putBool(
    "coordDMS",
    coordinateDMS
  );

  prefs.putBool(
    "alarmEn",
    alarmEnabled
  );

  prefs.putInt(
    "alarmHr",
    alarmHour
  );

  prefs.putInt(
    "alarmMin",
    alarmMinute
  );

  prefs.putString(
    "callsign",
    callSign
  );

  prefs.end();
}

// ============================================================
// ALARM SETTINGS SCREEN
// ============================================================

void drawAlarmScreen() {

  tft.fillScreen(
    backgroundColor()
  );

  tft.setFreeFont(
    NULL
  );

  uint16_t c =
    primaryColor();

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    accentColor(),
    backgroundColor()
  );

  tft.drawString(
    "ALARM",
    160,
    15,
    4
  );

  char timeBuf[24];

  if (
    local24Hour
  ) {

    snprintf(
      timeBuf,
      sizeof(timeBuf),

      "%02d:%02d",

      alarmHour,
      alarmMinute
    );
  }

  else {

    int h =
      alarmHour %
      12;

    if (
      h == 0
    ) {

      h = 12;
    }

    snprintf(
      timeBuf,
      sizeof(timeBuf),

      "%d:%02d %s",

      h,
      alarmMinute,

      alarmHour >= 12
        ? "PM"
        : "AM"
    );
  }

  tft.setTextColor(
    c,
    backgroundColor()
  );

  tft.drawString(
    timeBuf,
    160,
    49,
    4
  );

  drawOptionButton(
    55,
    69,
    210,
    27,

    alarmEnabled
      ? "ALARM ENABLED"
      : "ALARM DISABLED"
  );

  tft.drawString(
    "HOUR",
    160,
    116,
    2
  );

  drawSmallButton(
    20,
    101,
    55,
    31,
    "-"
  );

  drawSmallButton(
    245,
    101,
    55,
    31,
    "+"
  );

  tft.drawString(
    "MINUTE",
    160,
    153,
    2
  );

  drawSmallButton(
    20,
    138,
    55,
    31,
    "-"
  );

  drawSmallButton(
    245,
    138,
    55,
    31,
    "+"
  );

  if (
    !local24Hour
  ) {

    drawSmallButton(
      115,
      176,
      90,
      27,

      alarmHour >= 12
        ? "PM"
        : "AM"
    );
  }

  drawSmallButton(
    15,
    210,
    95,
    25,
    "BACK"
  );

  drawSmallButton(
    210,
    210,
    95,
    25,
    "SAVE"
  );
}


