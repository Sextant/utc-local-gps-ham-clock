// ============================================================
// ACTIVE ALARM SCREEN
// ============================================================

void drawAlarmActiveScreen() {

  uint16_t bg;
  uint16_t fg;
  uint16_t accent;

  if (
    alarmFlashRed
  ) {

    bg =
      TFT_RED;

    fg =
      TFT_BLACK;

    accent =
      TFT_BLACK;
  }

  else {

    bg =
      TFT_BLACK;

    fg =
      TFT_WHITE;

    accent =
      TFT_RED;
  }

  tft.fillScreen(
    bg
  );

  tft.setFreeFont(
    NULL
  );

  drawBellIcon(
    160,
    13,
    accent
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    accent,
    bg
  );

  tft.drawString(
    "ALARM",
    160,
    34,
    4
  );

  char currentTimeBuf[20] =
    "--:--:--";

  const char* suffix =
    "";

  if (
    systemTimeValid
  ) {

    time_t now =
      time(nullptr);

    struct tm localTime;

    getLocalTimeForEpoch(
      now,
      localTime
    );

    if (
      local24Hour
    ) {

      snprintf(
        currentTimeBuf,
        sizeof(currentTimeBuf),

        "%02d:%02d:%02d",

        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec
      );
    }

    else {

      int h =
        localTime.tm_hour %
        12;

      if (
        h == 0
      ) {

        h = 12;
      }

      snprintf(
        currentTimeBuf,
        sizeof(currentTimeBuf),

        "%d:%02d:%02d",

        h,
        localTime.tm_min,
        localTime.tm_sec
      );

      suffix =
        localTime.tm_hour >= 12
        ? "PM"
        : "AM";
    }
  }

  TFT_eSprite alarmTimeSprite =
    TFT_eSprite(&tft);

  alarmTimeSprite.createSprite(
    310,
    53
  );

  alarmTimeSprite.fillSprite(
    bg
  );

  alarmTimeSprite.setTextColor(
    fg
  );

  drawTightSevenSegTime(
    alarmTimeSprite,
    currentTimeBuf
  );

  alarmTimeSprite.pushSprite(
    5,
    51
  );

  alarmTimeSprite.deleteSprite();

  if (
    !local24Hour &&
    systemTimeValid
  ) {

    tft.setTextDatum(
      MR_DATUM
    );

    tft.setTextColor(
      fg,
      bg
    );

    tft.drawString(
      suffix,
      308,
      105,
      2
    );
  }

  char scheduledBuf[32];

  if (
    local24Hour
  ) {

    snprintf(
      scheduledBuf,
      sizeof(scheduledBuf),

      "SET: %02d:%02d",

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
      scheduledBuf,
      sizeof(scheduledBuf),

      "SET: %d:%02d %s",

      h,
      alarmMinute,

      alarmHour >= 12
        ? "PM"
        : "AM"
    );
  }

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    fg,
    bg
  );

  tft.drawString(
    scheduledBuf,
    160,
    111,
    1
  );

  tft.drawRoundRect(
    55,
    126,
    210,
    43,
    7,
    fg
  );

  tft.drawString(
    "SNOOZE 9 MIN",
    160,
    147,
    2
  );

  tft.drawRoundRect(
    55,
    181,
    210,
    43,
    7,
    fg
  );

  tft.drawString(
    "STOP",
    160,
    202,
    2
  );
}

// ============================================================
// BOLD SMALL UTC / LOCAL LABEL
//
// Each character is drawn individually.
// This preserves spacing while retaining the bold effect.
// ============================================================

void drawBoldSmallLabel(
  const char* text,
  int x,
  int y
) {

  tft.setFreeFont(
    NULL
  );

  tft.setTextDatum(
    TL_DATUM
  );

  tft.setTextColor(
    accentColor()
  );

  int cursorX =
    x;

  for (
    int i = 0;
    text[i] != '\0';
    i++
  ) {

    char ch[2];

    ch[0] =
      text[i];

    ch[1] =
      '\0';

    int charWidth =
      tft.textWidth(
        ch,
        2
      );

    // First pass
    tft.drawString(
      ch,
      cursorX,
      y,
      2
    );

    // Second pass shifted 1 pixel for bold effect
    tft.drawString(
      ch,
      cursorX + 1,
      y,
      2
    );

    // Preserve normal visual separation
    cursorX +=
      charWidth + 2;
  }
}

// ============================================================
// TOUCH
// ============================================================

void handleTouch() {

  if (
    !ts.touched()
  ) {

    return;
  }

  TS_Point p =
    ts.getPoint();

  int x =
    map(
      p.x,
      TOUCH_X_MIN,
      TOUCH_X_MAX,
      0,
      319
    );

  int y =
    map(
      p.y,
      TOUCH_Y_MIN,
      TOUCH_Y_MAX,
      0,
      239
    );

  x =
    constrain(
      x,
      0,
      319
    );

  y =
    constrain(
      y,
      0,
      239
    );

  delay(30);

  if (
    alarmActive
  ) {

    handleActiveAlarmTouch(
      x,
      y
    );

    return;
  }

  if (
    currentPage ==
    PAGE_CLOCK
  ) {

    handleClockTouch(
      x,
      y
    );
  }

  else if (
    currentPage ==
    PAGE_OPTIONS
  ) {

    handleOptionsTouch(
      x,
      y
    );
  }

  else if (
    currentPage ==
    PAGE_ALARM
  ) {

    handleAlarmTouch(
      x,
      y
    );
  }

  else if (
    currentPage ==
    PAGE_SET_TIME
  ) {

    handleSetTimeTouch(
      x,
      y
    );
  }

  else if (
    currentPage ==
    PAGE_CALLSIGN
  ) {

    handleCallsignTouch(
      x,
      y
    );
  }

  else if (
    currentPage ==
    PAGE_NIGHT_SETTINGS
  ) {

    handleNightSettingsTouch(
      x,
      y
    );
  }
}

// ============================================================
// ACTIVE ALARM TOUCH
// ============================================================

void handleActiveAlarmTouch(
  int x,
  int y
) {

  if (
    x >= 45 &&
    x <= 275 &&
    y >= 120 &&
    y <= 174
  ) {

    snoozeAlarm();

    waitForTouchRelease();

    return;
  }

  if (
    x >= 45 &&
    x <= 275 &&
    y >= 175 &&
    y <= 232
  ) {

    stopAlarm();

    waitForTouchRelease();

    return;
  }
}

// ============================================================
// CLOCK TOUCH
// ============================================================

void handleClockTouch(
  int x,
  int y
) {

  if (
    y < 30
  ) {

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();

    waitForTouchRelease();

    return;
  }

  if (
    y >= 125 &&
    y <= 183
  ) {

    local24Hour =
      !local24Hour;

    saveSettings();

    if (
      systemTimeValid
    ) {

      time_t now =
        time(nullptr);

      struct tm localTime;

      getLocalTimeForEpoch(
        now,
        localTime
      );

      drawLocalTime(
        localTime
      );

      drawLocalDate(
        localTime
      );
    }

    waitForTouchRelease();

    return;
  }

  // Bottom GPS line toggles Decimal / DMS
  if (
    y >= 204
  ) {

    coordinateDMS =
      !coordinateDMS;

    saveSettings();

    lastGpsDisplay[0] =
      '\0';

    drawGPSLine(
      true
    );

    waitForTouchRelease();

    return;
  }
}

// ============================================================
// OPTIONS SCREEN
// ============================================================

// Keep the drawing and touch layout based on one uniform grid.
// The visible buttons have a small gap between them, while the touch
// handler divides the full screen into generous row/column regions.
const int OPTIONS_LEFT_X  = 10;
const int OPTIONS_RIGHT_X = 165;
const int OPTIONS_BUTTON_W = 145;
const int OPTIONS_BUTTON_H = 43;

const int OPTIONS_ROW_1_Y = 32;
const int OPTIONS_ROW_2_Y = 80;
const int OPTIONS_ROW_3_Y = 128;
const int OPTIONS_ROW_4_Y = 176;

void drawOptionsScreen() {

  tft.fillScreen(
    backgroundColor()
  );

  tft.setFreeFont(
    NULL
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    accentColor(),
    backgroundColor()
  );

  tft.drawString(
    "OPTIONS",
    160,
    13,
    4
  );

  drawOptionButton(
    OPTIONS_LEFT_X,
    OPTIONS_ROW_1_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,

    local24Hour
      ? "LOCAL: 24 HOUR"
      : "LOCAL: 12 HOUR"
  );

  drawOptionButton(
    OPTIONS_RIGHT_X,
    OPTIONS_ROW_1_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,

    lightTheme
      ? "THEME: LIGHT"
      : "THEME: DARK"
  );

  const char* nightText;

  if (
    nightSetting == 0
  ) {

    nightText =
      "NIGHT MODE: AUTO";
  }

  else if (
    nightSetting == 1
  ) {

    nightText =
      "NIGHT MODE: ON";
  }

  else {

    nightText =
      "NIGHT MODE: OFF";
  }

  drawOptionButton(
    OPTIONS_LEFT_X,
    OPTIONS_ROW_2_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,
    nightText
  );

  char alarmText[48];

  if (
    local24Hour
  ) {

    snprintf(
      alarmText,
      sizeof(alarmText),

      "ALARM: %s %02d:%02d",

      alarmEnabled
        ? "ON"
        : "OFF",

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
      alarmText,
      sizeof(alarmText),

      "ALARM: %s %d:%02d %s",

      alarmEnabled
        ? "ON"
        : "OFF",

      h,
      alarmMinute,

      alarmHour >= 12
        ? "PM"
        : "AM"
    );
  }

  drawOptionButton(
    OPTIONS_RIGHT_X,
    OPTIONS_ROW_2_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,
    alarmText
  );

  char callButton[28];

  snprintf(
    callButton,
    sizeof(callButton),
    "CALL: %s",
    callSign
  );

  drawOptionButton(
    OPTIONS_LEFT_X,
    OPTIONS_ROW_3_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,
    callButton
  );

  drawOptionButton(
    OPTIONS_RIGHT_X,
    OPTIONS_ROW_3_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,
    "SET DATE/TIME"
  );

  char brightnessText[32];

  snprintf(
    brightnessText,
    sizeof(brightnessText),
    "BRIGHT: %s",
    BRIGHTNESS_LABEL[
      brightnessLevel
    ]
  );

  drawOptionButton(
    OPTIONS_LEFT_X,
    OPTIONS_ROW_4_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,
    brightnessText
  );

  drawOptionButton(
    OPTIONS_RIGHT_X,
    OPTIONS_ROW_4_Y,
    OPTIONS_BUTTON_W,
    OPTIONS_BUTTON_H,
    "BACK"
  );
}

// ============================================================
// OPTIONS TOUCH
// ============================================================

void handleOptionsTouch(
  int x,
  int y
) {

  // Split the entire usable area at the centers of the visual gaps.
  // This avoids narrow dead zones and gives the bottom row extra room
  // for the small Y-axis error common near the edge of resistive panels.
  bool leftColumn =
    x < 160;

  if (
    y >= 28 &&
    y <= 77
  ) {

    if (
      leftColumn
    ) {

      local24Hour =
        !local24Hour;
    }

    else {

      lightTheme =
        !lightTheme;
    }

    saveSettings();

    drawOptionsScreen();

    waitForTouchRelease();

    return;
  }

  if (
    y >= 78 &&
    y <= 125
  ) {

    if (
      leftColumn
    ) {

      prepareNightSettingsEditor();

      currentPage =
        PAGE_NIGHT_SETTINGS;

      drawNightSettingsScreen();
    }

    else {

      currentPage =
        PAGE_ALARM;

      drawAlarmScreen();
    }

    waitForTouchRelease();

    return;
  }

  if (
    y >= 126 &&
    y <= 173
  ) {

    if (
      leftColumn
    ) {

      beginCallsignEditor();

      currentPage =
        PAGE_CALLSIGN;

      drawCallsignScreen();
    }

    else {

      prepareManualTimeEditor();

      currentPage =
        PAGE_SET_TIME;

      drawSetTimeScreen();
    }

    waitForTouchRelease();

    return;
  }

  if (
    y >= 174
  ) {

    if (
      leftColumn
    ) {

      cycleBrightness();

      drawOptionsScreen();
    }

    else {

      currentPage =
        PAGE_CLOCK;

      drawClockScreen();
    }

    waitForTouchRelease();

    return;
  }
}

// ============================================================
// NIGHT SETTINGS EDITOR
// ============================================================

void prepareNightSettingsEditor() {

  editNightSetting =
    nightSetting;

  editNightStartHour =
    nightStartHour;

  editNightStartMinute =
    nightStartMinute;

  editNightEndHour =
    nightEndHour;

  editNightEndMinute =
    nightEndMinute;
}

void saveNightSettingsEditor() {

  nightSetting =
    editNightSetting;

  nightStartHour =
    editNightStartHour;

  nightStartMinute =
    editNightStartMinute;

  nightEndHour =
    editNightEndHour;

  nightEndMinute =
    editNightEndMinute;

  saveSettings();
  updateNightMode();
}

void drawNightSettingsScreen() {

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
    "NIGHT MODE",
    160,
    14,
    4
  );

  const char* modeText;

  if (editNightSetting == 0) {
    modeText = "MODE: AUTO";
  }
  else if (editNightSetting == 1) {
    modeText = "MODE: ON";
  }
  else {
    modeText = "MODE: OFF";
  }

  drawOptionButton(
    25,
    35,
    270,
    27,
    modeText
  );

  char buf[32];

  snprintf(
    buf,
    sizeof(buf),
    "START  %02d:%02d",
    editNightStartHour,
    editNightStartMinute
  );

  tft.setTextColor(
    c,
    backgroundColor()
  );

  tft.drawString(
    buf,
    160,
    79,
    2
  );

  drawSmallButton(15, 94, 65, 27, "H-");
  drawSmallButton(88, 94, 65, 27, "H+");
  drawSmallButton(167, 94, 65, 27, "M-");
  drawSmallButton(240, 94, 65, 27, "M+");

  snprintf(
    buf,
    sizeof(buf),
    "END    %02d:%02d",
    editNightEndHour,
    editNightEndMinute
  );

  tft.drawString(
    buf,
    160,
    139,
    2
  );

  drawSmallButton(15, 154, 65, 27, "H-");
  drawSmallButton(88, 154, 65, 27, "H+");
  drawSmallButton(167, 154, 65, 27, "M-");
  drawSmallButton(240, 154, 65, 27, "M+");

  tft.setTextColor(
    secondaryColor(),
    backgroundColor()
  );

  tft.drawString(
    "AUTO uses these local times",
    160,
    192,
    1
  );

  drawSmallButton(
    15,
    207,
    110,
    27,
    "BACK"
  );

  drawSmallButton(
    195,
    207,
    110,
    27,
    "SAVE"
  );
}

void handleNightSettingsTouch(
  int x,
  int y
) {

  // Cycle AUTO -> ON -> OFF.
  if (
    y >= 33 &&
    y <= 64
  ) {

    editNightSetting++;

    if (editNightSetting > 2) {
      editNightSetting = 0;
    }

    drawNightSettingsScreen();
    waitForTouchRelease();
    return;
  }

  // Start-time controls.
  if (
    y >= 90 &&
    y <= 125
  ) {

    if (x < 84) {
      editNightStartHour--;
      if (editNightStartHour < 0) editNightStartHour = 23;
    }
    else if (x < 160) {
      editNightStartHour++;
      if (editNightStartHour > 23) editNightStartHour = 0;
    }
    else if (x < 236) {
      editNightStartMinute -= 5;
      if (editNightStartMinute < 0) editNightStartMinute = 55;
    }
    else {
      editNightStartMinute += 5;
      if (editNightStartMinute > 59) editNightStartMinute = 0;
    }

    drawNightSettingsScreen();
    waitForTouchRelease();
    return;
  }

  // End-time controls.
  if (
    y >= 150 &&
    y <= 185
  ) {

    if (x < 84) {
      editNightEndHour--;
      if (editNightEndHour < 0) editNightEndHour = 23;
    }
    else if (x < 160) {
      editNightEndHour++;
      if (editNightEndHour > 23) editNightEndHour = 0;
    }
    else if (x < 236) {
      editNightEndMinute -= 5;
      if (editNightEndMinute < 0) editNightEndMinute = 55;
    }
    else {
      editNightEndMinute += 5;
      if (editNightEndMinute > 59) editNightEndMinute = 0;
    }

    drawNightSettingsScreen();
    waitForTouchRelease();
    return;
  }

  // BACK discards edits.
  if (
    y >= 202 &&
    x < 150
  ) {

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();
    waitForTouchRelease();
    return;
  }

  // SAVE commits to Preferences.
  if (
    y >= 202 &&
    x > 170
  ) {

    saveNightSettingsEditor();

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();
    waitForTouchRelease();
    return;
  }
}

// ============================================================
// CALLSIGN EDITOR
//
// Accepts A-Z, 0-9, and slash. Maximum 15 characters.
// The value is stored in ESP32 Preferences and survives reboot.
// ============================================================

void beginCallsignEditor() {

  strncpy(
    editCallSign,
    callSign,
    sizeof(editCallSign) - 1
  );

  editCallSign[
    sizeof(editCallSign) - 1
  ] = '\0';
}

void appendCallsignCharacter(
  char ch
) {

  size_t len =
    strlen(editCallSign);

  if (
    len >=
    CALLSIGN_MAX_LEN
  ) {
    return;
  }

  editCallSign[len] =
    ch;

  editCallSign[
    len + 1
  ] = '\0';
}

void deleteCallsignCharacter() {

  size_t len =
    strlen(editCallSign);

  if (
    len == 0
  ) {
    return;
  }

  editCallSign[
    len - 1
  ] = '\0';
}

void saveCallsignEditor() {

  if (
    strlen(editCallSign) == 0
  ) {

    strncpy(
      editCallSign,
      DEFAULT_CALLSIGN,
      sizeof(editCallSign) - 1
    );

    editCallSign[
      sizeof(editCallSign) - 1
    ] = '\0';
  }

  strncpy(
    callSign,
    editCallSign,
    sizeof(callSign) - 1
  );

  callSign[
    sizeof(callSign) - 1
  ] = '\0';

  saveSettings();
}

void drawKeyboardKey(
  int x,
  int y,
  int w,
  int h,
  const char* text
) {

  uint16_t c =
    primaryColor();

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    3,
    c
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    c,
    backgroundColor()
  );

  tft.drawString(
    text,
    x + w / 2,
    y + h / 2,
    1
  );
}

void drawCallsignScreen() {

  tft.fillScreen(
    backgroundColor()
  );

  tft.setFreeFont(
    NULL
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    accentColor(),
    backgroundColor()
  );

  tft.drawString(
    "EDIT CALLSIGN",
    160,
    12,
    2
  );

  tft.setTextColor(
    primaryColor(),
    backgroundColor()
  );

  tft.drawRoundRect(
    35,
    27,
    250,
    29,
    4,
    primaryColor()
  );

  tft.drawString(
    strlen(editCallSign)
      ? editCallSign
      : "_",
    160,
    41,
    2
  );

  // Row 1: numbers
  const char* row1 =
    "1234567890";

  for (
    int i = 0;
    i < 10;
    i++
  ) {

    char key[2] = {
      row1[i],
      '\0'
    };

    drawKeyboardKey(
      5 + i * 31,
      61,
      29,
      27,
      key
    );
  }

  // Row 2: QWERTY
  const char* row2 =
    "QWERTYUIOP";

  for (
    int i = 0;
    i < 10;
    i++
  ) {

    char key[2] = {
      row2[i],
      '\0'
    };

    drawKeyboardKey(
      5 + i * 31,
      91,
      29,
      27,
      key
    );
  }

  // Row 3
  const char* row3 =
    "ASDFGHJKL";

  for (
    int i = 0;
    i < 9;
    i++
  ) {

    char key[2] = {
      row3[i],
      '\0'
    };

    drawKeyboardKey(
      20 + i * 31,
      121,
      29,
      27,
      key
    );
  }

  // Row 4 includes slash for portable/compound callsigns.
  const char* row4 =
    "ZXCVBNM/";

  for (
    int i = 0;
    i < 8;
    i++
  ) {

    char key[2] = {
      row4[i],
      '\0'
    };

    drawKeyboardKey(
      36 + i * 31,
      151,
      29,
      27,
      key
    );
  }

  drawSmallButton(
    5,
    190,
    70,
    39,
    "BACK"
  );

  drawSmallButton(
    82,
    190,
    70,
    39,
    "BKSP"
  );

  drawSmallButton(
    159,
    190,
    70,
    39,
    "CLEAR"
  );

  drawSmallButton(
    236,
    190,
    79,
    39,
    "SAVE"
  );
}

void handleCallsignTouch(
  int x,
  int y
) {

  // Keyboard-only X calibration.
  // The left side of the keyboard needs a positive offset, while the
  // error grows progressively toward the right.  A constant offset therefore
  // cannot align the whole keyboard.  Apply a small linear scale correction
  // only on this page; the rest of the UI keeps the proven global calibration.
  // Final empirical fit from hardware testing:
  //     correctedX ~= 0.920 * reportedX + 16
  int keyX = (int)round(
    ((double)x * 0.920) + 16.0
  );

  keyX = constrain(
    keyX,
    0,
    319
  );

  // Row 1
  if (
    y >= 59 &&
    y <= 90
  ) {

    int index =
      (keyX - 5) / 31;

    if (
      index >= 0 &&
      index < 10
    ) {

      const char* row =
        "1234567890";

      appendCallsignCharacter(
        row[index]
      );

      drawCallsignScreen();
    }

    waitForTouchRelease();
    return;
  }

  // Row 2
  if (
    y >= 89 &&
    y <= 120
  ) {

    int index =
      (keyX - 5) / 31;

    if (
      index >= 0 &&
      index < 10
    ) {

      const char* row =
        "QWERTYUIOP";

      appendCallsignCharacter(
        row[index]
      );

      drawCallsignScreen();
    }

    waitForTouchRelease();
    return;
  }

  // Row 3
  if (
    y >= 119 &&
    y <= 150
  ) {

    int index =
      (keyX - 20) / 31;

    if (
      index >= 0 &&
      index < 9
    ) {

      const char* row =
        "ASDFGHJKL";

      appendCallsignCharacter(
        row[index]
      );

      drawCallsignScreen();
    }

    waitForTouchRelease();
    return;
  }

  // Row 4
  if (
    y >= 149 &&
    y <= 181
  ) {

    int index =
      (keyX - 36) / 31;

    if (
      index >= 0 &&
      index < 8
    ) {

      const char* row =
        "ZXCVBNM/";

      appendCallsignCharacter(
        row[index]
      );

      drawCallsignScreen();
    }

    waitForTouchRelease();
    return;
  }

  // Bottom action buttons
  if (
    y >= 185
  ) {

    if (
      x < 78
    ) {

      currentPage =
        PAGE_OPTIONS;

      drawOptionsScreen();
    }

    else if (
      x < 156
    ) {

      deleteCallsignCharacter();
      drawCallsignScreen();
    }

    else if (
      x < 233
    ) {

      editCallSign[0] =
        '\0';

      drawCallsignScreen();
    }

    else {

      saveCallsignEditor();

      currentPage =
        PAGE_OPTIONS;

      drawOptionsScreen();
    }

    waitForTouchRelease();
    return;
  }
}

// ============================================================
// ALARM TOUCH
// ============================================================

void handleAlarmTouch(
  int x,
  int y
) {

  if (
    y >= 67 &&
    y <= 98
  ) {

    bool wasEnabled =
      alarmEnabled;

    alarmEnabled =
      !alarmEnabled;

    if (
      !wasEnabled &&
      alarmEnabled
    ) {

      resetAlarmOccurrenceLock();

      snoozeActive =
        false;
    }

    drawAlarmScreen();

    waitForTouchRelease();

    return;
  }

  if (
    x <= 95 &&
    y >= 99 &&
    y <= 134
  ) {

    performAdjustment(
      ADJ_ALARM_HOUR,
      -1
    );

    holdRepeat(
      ADJ_ALARM_HOUR,
      -1
    );

    waitForTouchRelease();

    return;
  }

  if (
    x >= 225 &&
    y >= 99 &&
    y <= 134
  ) {

    performAdjustment(
      ADJ_ALARM_HOUR,
      1
    );

    holdRepeat(
      ADJ_ALARM_HOUR,
      1
    );

    waitForTouchRelease();

    return;
  }

  if (
    x <= 95 &&
    y >= 136 &&
    y <= 171
  ) {

    performAdjustment(
      ADJ_ALARM_MINUTE,
      -1
    );

    holdRepeat(
      ADJ_ALARM_MINUTE,
      -1
    );

    waitForTouchRelease();

    return;
  }

  if (
    x >= 225 &&
    y >= 136 &&
    y <= 171
  ) {

    performAdjustment(
      ADJ_ALARM_MINUTE,
      1
    );

    holdRepeat(
      ADJ_ALARM_MINUTE,
      1
    );

    waitForTouchRelease();

    return;
  }

  if (
    !local24Hour &&
    x >= 105 &&
    x <= 215 &&
    y >= 173 &&
    y <= 205
  ) {

    alarmHour =
      (
        alarmHour + 12
      ) %
      24;

    resetAlarmOccurrenceLock();

    snoozeActive =
      false;

    drawAlarmScreen();

    waitForTouchRelease();

    return;
  }

  if (
    x <= 120 &&
    y >= 207
  ) {

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();

    waitForTouchRelease();

    return;
  }

  if (
    x >= 200 &&
    y >= 207
  ) {

    saveSettings();

    resetAlarmOccurrenceLock();

    snoozeActive =
      false;

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();

    waitForTouchRelease();

    return;
  }
}

// ============================================================
// ADJUSTMENT
// ============================================================

void performAdjustment(
  AdjustTarget target,
  int direction
) {

  switch (
    target
  ) {

    case ADJ_ALARM_HOUR:

      alarmHour +=
        direction;

      if (
        alarmHour < 0
      ) {

        alarmHour = 23;
      }

      if (
        alarmHour > 23
      ) {

        alarmHour = 0;
      }

      resetAlarmOccurrenceLock();

      snoozeActive =
        false;

      break;

    case ADJ_ALARM_MINUTE:

      alarmMinute +=
        direction;

      if (
        alarmMinute < 0
      ) {

        alarmMinute = 59;
      }

      if (
        alarmMinute > 59
      ) {

        alarmMinute = 0;
      }

      resetAlarmOccurrenceLock();

      snoozeActive =
        false;

      break;

    case ADJ_EDIT_HOUR:

      editHour +=
        direction;

      if (
        editHour < 0
      ) {

        editHour = 23;
      }

      if (
        editHour > 23
      ) {

        editHour = 0;
      }

      break;

    case ADJ_EDIT_MINUTE:

      editMinute +=
        direction;

      if (
        editMinute < 0
      ) {

        editMinute = 59;
      }

      if (
        editMinute > 59
      ) {

        editMinute = 0;
      }

      break;

    case ADJ_EDIT_MONTH:

      editMonth +=
        direction;

      if (
        editMonth < 1
      ) {

        editMonth = 12;
      }

      if (
        editMonth > 12
      ) {

        editMonth = 1;
      }

      if (
        editDay >
        daysInMonth(
          editYear,
          editMonth
        )
      ) {

        editDay =
          daysInMonth(
            editYear,
            editMonth
          );
      }

      break;

    case ADJ_EDIT_DAY: {

      int maxDay =
        daysInMonth(
          editYear,
          editMonth
        );

      editDay +=
        direction;

      if (
        editDay < 1
      ) {

        editDay =
          maxDay;
      }

      if (
        editDay > maxDay
      ) {

        editDay = 1;
      }

      break;
    }

    case ADJ_EDIT_YEAR:

      editYear +=
        direction;

      if (
        editYear < 2020
      ) {

        editYear = 2099;
      }

      if (
        editYear > 2099
      ) {

        editYear = 2020;
      }

      if (
        editDay >
        daysInMonth(
          editYear,
          editMonth
        )
      ) {

        editDay =
          daysInMonth(
            editYear,
            editMonth
          );
      }

      break;
  }

  redrawAdjustmentPage(
    target
  );
}

// ============================================================
// LONG PRESS
// ============================================================

void holdRepeat(
  AdjustTarget target,
  int direction
) {

  unsigned long pressStart =
    millis();

  while (
    ts.touched()
  ) {

    if (
      millis() -
      pressStart >=
      HOLD_START_MS
    ) {

      break;
    }

    delay(10);
  }

  if (
    !ts.touched()
  ) {

    return;
  }

  unsigned long lastRepeat =
    millis();

  while (
    ts.touched()
  ) {

    unsigned long heldFor =
      millis() -
      pressStart;

    unsigned long repeatInterval =
      heldFor >=
      HOLD_ACCEL_MS
      ? HOLD_FAST_MS
      : HOLD_SLOW_MS;

    if (
      millis() -
      lastRepeat >=
      repeatInterval
    ) {

      lastRepeat =
        millis();

      performAdjustment(
        target,
        direction
      );
    }

    delay(5);
  }
}

// ============================================================
// REDRAW ADJUSTMENT
// ============================================================

void redrawAdjustmentPage(
  AdjustTarget target
) {

  if (
    target ==
      ADJ_ALARM_HOUR ||

    target ==
      ADJ_ALARM_MINUTE
  ) {

    drawAlarmScreen();
  }

  else {

    drawSetTimeScreen();
  }
}

// ============================================================
// PREPARE MANUAL TIME
// ============================================================

void prepareManualTimeEditor() {

  if (
    systemTimeValid
  ) {

    time_t now =
      time(nullptr);

    struct tm localTime;

    getLocalTimeForEpoch(
      now,
      localTime
    );

    editYear =
      localTime.tm_year +
      1900;

    editMonth =
      localTime.tm_mon +
      1;

    editDay =
      localTime.tm_mday;

    editHour =
      localTime.tm_hour;

    editMinute =
      localTime.tm_min;
  }

  else {

    editYear   = 2026;
    editMonth  = 1;
    editDay    = 1;
    editHour   = 12;
    editMinute = 0;
  }
}

// ============================================================
// SET DATE / TIME SCREEN
// ============================================================

void drawSetTimeScreen() {

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
    "SET DATE / TIME",
    160,
    14,
    2
  );

  char buf[40];

  if (
    local24Hour
  ) {

    snprintf(
      buf,
      sizeof(buf),

      "HOUR      %02d",

      editHour
    );
  }

  else {

    int h =
      editHour %
      12;

    if (
      h == 0
    ) {

      h = 12;
    }

    snprintf(
      buf,
      sizeof(buf),

      "HOUR    %d %s",

      h,

      editHour >= 12
        ? "PM"
        : "AM"
    );
  }

  tft.setTextColor(
    c,
    backgroundColor()
  );

  tft.drawString(
    buf,
    160,
    43,
    2
  );

  drawSmallButton(
    15,
    29,
    45,
    28,
    "-"
  );

  drawSmallButton(
    260,
    29,
    45,
    28,
    "+"
  );

  snprintf(
    buf,
    sizeof(buf),

    "MINUTE      %02d",

    editMinute
  );

  tft.drawString(
    buf,
    160,
    73,
    2
  );

  drawSmallButton(
    15,
    59,
    45,
    28,
    "-"
  );

  drawSmallButton(
    260,
    59,
    45,
    28,
    "+"
  );

  snprintf(
    buf,
    sizeof(buf),

    "MONTH      %s",

    monthFullName(
      editMonth
    )
  );

  tft.drawString(
    buf,
    160,
    103,
    2
  );

  drawSmallButton(
    15,
    89,
    45,
    28,
    "-"
  );

  drawSmallButton(
    260,
    89,
    45,
    28,
    "+"
  );

  snprintf(
    buf,
    sizeof(buf),

    "DAY        %02d",

    editDay
  );

  tft.drawString(
    buf,
    160,
    133,
    2
  );

  drawSmallButton(
    15,
    119,
    45,
    28,
    "-"
  );

  drawSmallButton(
    260,
    119,
    45,
    28,
    "+"
  );

  snprintf(
    buf,
    sizeof(buf),

    "YEAR       %04d",

    editYear
  );

  tft.drawString(
    buf,
    160,
    163,
    2
  );

  drawSmallButton(
    15,
    149,
    45,
    28,
    "-"
  );

  drawSmallButton(
    260,
    149,
    45,
    28,
    "+"
  );

  if (
    !local24Hour
  ) {

    drawSmallButton(
      115,
      181,
      90,
      25,

      editHour >= 12
        ? "PM"
        : "AM"
    );
  }

  drawSmallButton(
    10,
    211,
    90,
    24,
    "BACK"
  );

  drawSmallButton(
    115,
    211,
    90,
    24,
    "NOW GPS"
  );

  drawSmallButton(
    220,
    211,
    90,
    24,
    "SAVE"
  );
}

// ============================================================
// SET DATE / TIME TOUCH
// ============================================================

void handleSetTimeTouch(
  int x,
  int y
) {

  bool minusSide =
    x < 90;

  bool plusSide =
    x > 230;

  if (
    y >= 27 &&
    y <= 58
  ) {

    if (
      minusSide
    ) {

      performAdjustment(
        ADJ_EDIT_HOUR,
        -1
      );

      holdRepeat(
        ADJ_EDIT_HOUR,
        -1
      );
    }

    else if (
      plusSide
    ) {

      performAdjustment(
        ADJ_EDIT_HOUR,
        1
      );

      holdRepeat(
        ADJ_EDIT_HOUR,
        1
      );
    }

    waitForTouchRelease();

    return;
  }

  if (
    y >= 58 &&
    y <= 88
  ) {

    if (
      minusSide
    ) {

      performAdjustment(
        ADJ_EDIT_MINUTE,
        -1
      );

      holdRepeat(
        ADJ_EDIT_MINUTE,
        -1
      );
    }

    else if (
      plusSide
    ) {

      performAdjustment(
        ADJ_EDIT_MINUTE,
        1
      );

      holdRepeat(
        ADJ_EDIT_MINUTE,
        1
      );
    }

    waitForTouchRelease();

    return;
  }

  if (
    y >= 88 &&
    y <= 118
  ) {

    if (
      minusSide
    ) {

      performAdjustment(
        ADJ_EDIT_MONTH,
        -1
      );

      holdRepeat(
        ADJ_EDIT_MONTH,
        -1
      );
    }

    else if (
      plusSide
    ) {

      performAdjustment(
        ADJ_EDIT_MONTH,
        1
      );

      holdRepeat(
        ADJ_EDIT_MONTH,
        1
      );
    }

    waitForTouchRelease();

    return;
  }

  if (
    y >= 118 &&
    y <= 148
  ) {

    if (
      minusSide
    ) {

      performAdjustment(
        ADJ_EDIT_DAY,
        -1
      );

      holdRepeat(
        ADJ_EDIT_DAY,
        -1
      );
    }

    else if (
      plusSide
    ) {

      performAdjustment(
        ADJ_EDIT_DAY,
        1
      );

      holdRepeat(
        ADJ_EDIT_DAY,
        1
      );
    }

    waitForTouchRelease();

    return;
  }

  if (
    y >= 148 &&
    y <= 179
  ) {

    if (
      minusSide
    ) {

      performAdjustment(
        ADJ_EDIT_YEAR,
        -1
      );

      holdRepeat(
        ADJ_EDIT_YEAR,
        -1
      );
    }

    else if (
      plusSide
    ) {

      performAdjustment(
        ADJ_EDIT_YEAR,
        1
      );

      holdRepeat(
        ADJ_EDIT_YEAR,
        1
      );
    }

    waitForTouchRelease();

    return;
  }

  if (
    !local24Hour &&
    x >= 100 &&
    x <= 220 &&
    y >= 178 &&
    y <= 209
  ) {

    editHour =
      (
        editHour + 12
      ) %
      24;

    drawSetTimeScreen();

    waitForTouchRelease();

    return;
  }

  if (
    x < 105 &&
    y >= 205
  ) {

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();

    waitForTouchRelease();

    return;
  }

  if (
    x >= 105 &&
    x <= 215 &&
    y >= 205
  ) {

    if (
      gpsState ==
      GPS_SLEEP
    ) {

      wakeGPS();
    }

    currentPage =
      PAGE_OPTIONS;

    drawOptionsScreen();

    waitForTouchRelease();

    return;
  }

  if (
    x > 215 &&
    y >= 205
  ) {

    saveManualDateTime();

    currentPage =
      PAGE_CLOCK;

    drawClockScreen();

    waitForTouchRelease();

    return;
  }
}

// ============================================================
// SAVE MANUAL DATE / TIME
// ============================================================

void saveManualDateTime() {

  struct tm local = {};

  local.tm_year =
    editYear - 1900;

  local.tm_mon =
    editMonth - 1;

  local.tm_mday =
    editDay;

  local.tm_hour =
    editHour;

  local.tm_min =
    editMinute;

  local.tm_sec =
    0;

  local.tm_isdst =
    -1;

  // Interpret the entered fields as LOCAL civil time in the currently
  // resolved geographic timezone, then convert them back to UTC.
  time_t epoch =
    localFieldsToUtcEpoch(
      local
    );

  setSystemEpoch(
    epoch
  );

  Serial.println(
    "Manual date/time set"
  );
}

// ============================================================
// OPTION BUTTON
// ============================================================

void drawOptionButton(
  int x,
  int y,
  int w,
  int h,
  const char* text
) {

  uint16_t c =
    primaryColor();

  tft.setFreeFont(
    NULL
  );

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    5,
    c
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    c,
    backgroundColor()
  );

  tft.drawString(
    text,
    x + w / 2,
    y + h / 2,
    2
  );
}

// ============================================================
// SMALL BUTTON
// ============================================================

void drawSmallButton(
  int x,
  int y,
  int w,
  int h,
  const char* text
) {

  uint16_t c =
    primaryColor();

  tft.setFreeFont(
    NULL
  );

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    4,
    c
  );

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    c,
    backgroundColor()
  );

  tft.drawString(
    text,
    x + w / 2,
    y + h / 2,
    2
  );
}

// ============================================================
// TOUCH RELEASE
// ============================================================

void waitForTouchRelease() {

  while (
    ts.touched()
  ) {

    delay(10);
  }

  delay(50);
}

