struct PlaceRecord;

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// ============================================================
// DISPLAY
// ============================================================

TFT_eSPI tft = TFT_eSPI();

TFT_eSprite utcTimeSprite   = TFT_eSprite(&tft);
TFT_eSprite localTimeSprite = TFT_eSprite(&tft);
TFT_eSprite gpsSprite       = TFT_eSprite(&tft);

// ============================================================
// GPS HARDWARE
// ============================================================

TinyGPSPlus gps;
HardwareSerial GPS(2);

const int GPS_RX_PIN     = 22;
const int PPS_PIN        = 35;
const int GPS_ENABLE_PIN = 27;   // CN1 GPIO27 -> GP-02 N/F

volatile bool ppsFlag = false;

void IRAM_ATTR ppsISR() {
  ppsFlag = true;
}

// ============================================================
// GPS DUTY CYCLE
// ============================================================

enum GPSState {
  GPS_AWAKE,
  GPS_SLEEP
};

GPSState gpsState = GPS_AWAKE;

// GPS wakes every 30 minutes
const unsigned long GPS_SLEEP_INTERVAL_MS =
  30UL * 60UL * 1000UL;

// Maximum acquisition period for each GPS wake cycle.
const unsigned long GPS_ACQUIRE_TIMEOUT_MS =
  120UL * 1000UL;

// If a wake cycle ends without a stable position, retry much sooner
// than the normal 30-minute synchronization interval. This prevents
// an indoor startup from leaving the clock geographically stale for
// 30 minutes after it is later moved to a window.
const unsigned long GPS_NO_FIX_RETRY_INTERVAL_MS =
  2UL * 60UL * 1000UL;

// Require several mutually consistent location updates before the
// position is accepted. This avoids caching the first marginal fix
// after a long period without satellite reception.
const int GPS_STABLE_FIX_COUNT_REQUIRED = 3;
const int GPS_STABLE_FIX_MIN_SATELLITES = 4;
const double GPS_STABLE_FIX_MAX_STEP_KM = 0.10;  // 100 m

unsigned long gpsWakeMillis  = 0;
unsigned long gpsSleepMillis = 0;
unsigned long gpsSleepIntervalMs = GPS_SLEEP_INTERVAL_MS;

bool gpsTimeSyncedThisCycle = false;
bool gpsPositionReady       = false;

// True only when the most recently completed GPS wake cycle accepted
// a stable position. If a cycle times out without a stable new fix,
// the display says GPS CACHED rather than incorrectly saying GPS SYNC.
bool lastGpsCyclePositionSynced = false;

bool pendingGpsSync = false;
time_t pendingGpsEpoch = 0;

// Stable-fix accumulator and accepted snapshot.
int gpsStableFixCount = 0;
double gpsStableLastLatitude = 0.0;
double gpsStableLastLongitude = 0.0;
double gpsStableLatitudeSum = 0.0;
double gpsStableLongitudeSum = 0.0;
double gpsStableAltitudeSum = 0.0;
int gpsStableAltitudeSamples = 0;
int gpsStableSatellites = 0;

bool stableGpsFixReady = false;
double stableGpsLatitude = 0.0;
double stableGpsLongitude = 0.0;
double stableGpsAltitude = 0.0;
int stableGpsSatellites = 0;

// ============================================================
// GPS FRESHNESS
// ============================================================

const uint32_t GPS_FIX_MAX_AGE_MS  = 3000;
const uint32_t GPS_TIME_MAX_AGE_MS = 3000;

// ============================================================
// STORED GPS DATA
// ============================================================

bool storedPositionValid = false;

double storedLatitude  = 0.0;
double storedLongitude = 0.0;
double storedAltitude  = 0.0;

int storedSatellites = 0;

char storedGrid[7] = "------";

char lastGpsDisplay[160] = "";
char lastGridDisplay[7]  = "";

// Static clock fields are cached so they are only redrawn when
// their displayed value actually changes. This prevents the UTC/local
// dates and AM/PM indicator from flickering in sync with the seconds.
char lastUTCDateDisplay[32]   = "";
char lastLocalDateDisplay[40] = "";

// ============================================================
// COORDINATE FORMAT
//
// false = signed decimal degrees
// true  = degrees / minutes / seconds
// ============================================================

bool coordinateDMS = false;

// ============================================================
// SYSTEM TIME
// ============================================================

bool systemTimeValid = false;

time_t lastDisplayedSecond = -1;

// ============================================================
// TOUCHSCREEN
// ============================================================

#define TOUCH_CS    33
#define TOUCH_IRQ   36
#define TOUCH_CLK   25
#define TOUCH_MISO  39
#define TOUCH_MOSI  32

#define TOUCH_X_MIN 500
#define TOUCH_X_MAX 3550
#define TOUCH_Y_MIN 650
#define TOUCH_Y_MAX 3470

// The touchscreen and microSD slot both use the ESP32 VSPI host,
// but they are wired to different physical pins on this board.
// We therefore share one SPIClass instance and switch the pin routing
// only when SD access is required. SD database lookups are infrequent.
SPIClass sharedSPI = SPIClass(VSPI);

XPT2046_Touchscreen ts(
  TOUCH_CS,
  TOUCH_IRQ
);

// ============================================================
// MICROSD / OFFLINE GEOGRAPHIC DATABASES
// ============================================================

#define SD_CS    5
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCK   18

const char* TZ_ZONES_FILE = "/timezone/zones.bin";
const char* TZ_INDEX_FILE = "/timezone/index.bin";
const char* TZ_TILES_FILE = "/timezone/tiles.bin";
const char* TZ_RULES_FILE = "/timezone/rules.bin";

const char* PLACES_FILE = "/places/places.bin";
const char* PLACES_INDEX_FILE = "/places/places_index.bin";
const char* MARINE_FILE = "/marine/marine.bin";

bool sdAvailable = false;
bool timezoneDatabaseReady = false;
bool placeDatabaseReady = false;
bool marineDatabaseReady = false;

// Timezone geographic index headers
uint16_t tzCoarseCols = 0;
uint16_t tzCoarseRows = 0;
float tzCoarseDeg = 0.0f;
float tzFineDeg = 0.0f;
uint16_t tzFinePerCoarse = 0;
uint16_t tzTileWidth = 0;
uint16_t tzTileHeight = 0;
uint32_t tzTileCount = 0;

// Place geographic index headers
float placeCellDegrees = 0.0f;
uint16_t placeGridCols = 0;
uint16_t placeGridRows = 0;
uint32_t placeCellCount = 0;
uint32_t placeRecordCount = 0;

const uint8_t TZ_CELL_DIRECT = 0;
const uint8_t TZ_CELL_TILE = 1;
const uint8_t TZ_CELL_UNKNOWN = 2;
const uint8_t TZ_UNKNOWN_ZONE = 255;

const uint32_t PLACE_INDEX_HEADER_SIZE = 18;
const double EARTH_RADIUS_KM = 6371.0088;
const double MAX_PLACE_DISTANCE_KM = 10.0;
const double LOCATION_RELOOKUP_DISTANCE_KM = 1.0;

// Current resolved geographic context
bool geoContextValid = false;
bool currentZoneNautical = false;
int currentZoneID = -1;
char currentZoneName[48] = "";
char currentZoneAbbrev[16] = "UTC";
int32_t currentUtcOffsetSeconds = 0;
bool currentTimeRuleValid = false;
time_t nextTimeRuleTransition = 0;

bool currentPlaceValid = false;
char currentPlaceName[96] = "";
char currentPlaceCountry[4] = "";
double currentPlaceDistanceKm = 0.0;

bool currentMarineValid = false;
char currentMarineName[96] = "";

double resolvedLocationLat = 999.0;
double resolvedLocationLon = 999.0;

struct PlaceRecord {
  double latitude;
  double longitude;
  uint32_t population;
  uint32_t geonameID;
  char country[4];
  char feature[12];
  char name[96];
};

// ============================================================
// USER
// ============================================================

// Callsign is editable from OPTIONS and stored in Preferences.
// New installations default to NOCALL until the user sets one.
const char* DEFAULT_CALLSIGN = "NOCALL";
const size_t CALLSIGN_MAX_LEN = 15;
char callSign[CALLSIGN_MAX_LEN + 1] = "NOCALL";
char editCallSign[CALLSIGN_MAX_LEN + 1] = "";

// ============================================================
// PREFERENCES
// ============================================================

Preferences prefs;

// ============================================================
// DISPLAY SETTINGS
// ============================================================

// false = 12 hour
// true  = 24 hour
bool local24Hour = false;

// false = dark
// true  = light
bool lightTheme = false;

// Night mode:
// 0 = AUTO
// 1 = ON
// 2 = OFF
uint8_t nightSetting = 0;

bool redMode = false;

// ============================================================
// MANUAL SCREEN BRIGHTNESS
// ============================================================
//
// CYD TFT backlight is controlled by GPIO21.
// Four user-selectable levels are provided on the Options page.
// The selected level is stored in ESP32 Preferences.
//
// 0 = 25%
// 1 = 50%
// 2 = 75%
// 3 = 100%

const int TFT_BACKLIGHT_PIN = 21;
const int BACKLIGHT_PWM_FREQ = 5000;
const int BACKLIGHT_PWM_RESOLUTION = 8;

uint8_t brightnessLevel = 3;

const uint8_t BRIGHTNESS_DUTY[4] = {
  64,   // 25%
  128,  // 50%
  191,  // 75%
  255   // 100%
};

const char* BRIGHTNESS_LABEL[4] = {
  "25%",
  "50%",
  "75%",
  "100%"
};

// AUTO night schedule. These are user-configurable and saved
// in ESP32 Preferences. Defaults preserve the original behavior.
int nightStartHour   = 20;
int nightStartMinute = 0;
int nightEndHour     = 7;
int nightEndMinute   = 0;

// Temporary values used by the Night Settings editor.
uint8_t editNightSetting = 0;
int editNightStartHour   = 20;
int editNightStartMinute = 0;
int editNightEndHour     = 7;
int editNightEndMinute   = 0;

// ============================================================
// ONBOARD R21 AMBIENT LIGHT SENSOR
// ============================================================
//
// CYD ESP32-2432S028 onboard photoresistor.
// Measured on this hardware:
//   normal room / bright light: ~142 mV
//   dark / covered:            ~225-395 mV
//
// AUTO Night Mode uses hysteresis:
//   >= 220 mV for 5 consecutive checks -> enter red Night Mode
//   <= 180 mV for 5 consecutive checks -> leave red Night Mode
//   180-220 mV -> retain current state
//
// The saved START/END schedule remains available as a fallback
// if the sensor reading is outside a plausible operating range.

const int LDR_PIN = 34;

const int LDR_DARK_THRESHOLD_MV  = 220;
const int LDR_LIGHT_THRESHOLD_MV = 180;

const int LDR_CONFIRM_SECONDS = 5;
const int LDR_SAMPLE_COUNT    = 16;

// Broad validity range. The actual R21 circuit operates far
// inside this range; these limits primarily detect an open,
// failed, or obviously invalid ADC condition.
const int LDR_VALID_MIN_MV = 20;
const int LDR_VALID_MAX_MV = 2000;

int ldrLastMilliVolts = -1;
int ldrDarkConfirmCount = 0;
int ldrLightConfirmCount = 0;
bool ldrSensorValid = false;

// ============================================================
// ALARM
// ============================================================

bool alarmEnabled = false;

int alarmHour   = 7;
int alarmMinute = 0;

bool alarmActive = false;

const unsigned long ALARM_TIMEOUT_MS = 120000UL;
const unsigned long ALARM_FLASH_MS   = 500UL;

const time_t SNOOZE_SECONDS =
  9 * 60;

unsigned long alarmStartedMillis   = 0;
unsigned long lastAlarmFlashMillis = 0;

bool alarmFlashRed = false;

bool snoozeActive = false;

time_t snoozeUntilEpoch = 0;

long long lastAlarmOccurrenceKey = -1;

// ============================================================
// MANUAL DATE / TIME
// ============================================================

int editYear   = 2026;
int editMonth  = 1;
int editDay    = 1;
int editHour   = 12;
int editMinute = 0;

// ============================================================
// POWER DISPLAY
// ============================================================

bool batteryHardwareInstalled = false;

// ============================================================
// SCREEN STATE
// ============================================================

enum ScreenPage {
  PAGE_CLOCK,
  PAGE_OPTIONS,
  PAGE_ALARM,
  PAGE_SET_TIME,
  PAGE_CALLSIGN,
  PAGE_NIGHT_SETTINGS
};

ScreenPage currentPage =
  PAGE_CLOCK;

// ============================================================
// LONG PRESS
// ============================================================

const unsigned long HOLD_START_MS = 500;
const unsigned long HOLD_SLOW_MS  = 150;
const unsigned long HOLD_FAST_MS  = 65;
const unsigned long HOLD_ACCEL_MS = 2000;

enum AdjustTarget {
  ADJ_ALARM_HOUR,
  ADJ_ALARM_MINUTE,
  ADJ_EDIT_HOUR,
  ADJ_EDIT_MINUTE,
  ADJ_EDIT_MONTH,
  ADJ_EDIT_DAY,
  ADJ_EDIT_YEAR
};

// ============================================================
// COLORS
// ============================================================

uint16_t backgroundColor() {

  if (redMode) {
    return TFT_RED;
  }

  if (lightTheme) {
    return TFT_WHITE;
  }

  return TFT_BLACK;
}

uint16_t primaryColor() {

  if (redMode) {
    return TFT_BLACK;
  }

  if (lightTheme) {
    return TFT_NAVY;
  }

  return TFT_WHITE;
}

uint16_t secondaryColor() {

  if (redMode) {
    return TFT_BLACK;
  }

  if (lightTheme) {
    return TFT_DARKGREY;
  }

  return TFT_LIGHTGREY;
}

uint16_t accentColor() {

  if (redMode) {
    return TFT_BLACK;
  }

  if (lightTheme) {
    return TFT_BLUE;
  }

  return TFT_CYAN;
}

uint16_t statusColor() {

  if (redMode) {
    return TFT_BLACK;
  }

  if (lightTheme) {
    return TFT_DARKGREEN;
  }

  return TFT_GREEN;
}

uint16_t dateColor() {

  if (redMode) {
    return TFT_BLACK;
  }

  if (lightTheme) {
    return TFT_MAROON;
  }

  return TFT_YELLOW;
}

// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void loadSettings();
void saveSettings();

bool gpsHasFreshFix();
bool gpsHasFreshTime();

void serviceGPS();
void servicePPS();
void serviceGPSDutyCycle();

void wakeGPS();
void sleepGPS();
void sleepGPSFor(unsigned long intervalMs);
void finishGPSCycle();

void resetStableGPSFixAccumulator();
void processStableGPSFixSample();
void cacheCurrentGPSPosition();

time_t gpsDateTimeToEpoch();

void setSystemEpoch(
  time_t epoch
);

void serviceClockDisplay();

void updateNightMode();

void serviceAlarm();

void startAlarm();
void stopAlarm();
void snoozeAlarm();

void updateAlarmFlash();
void drawAlarmActiveScreen();

long long makeAlarmOccurrenceKey(
  struct tm &localTime
);

void resetAlarmOccurrenceLock();

void drawClockScreen();
void updateClockScreen();

void drawUTCTime(
  struct tm &t
);

void drawUTCDate(
  struct tm &t
);

void drawLocalTime(
  struct tm &t
);

void drawLocalDate(
  struct tm &t
);

void drawTightSevenSegTime(
  TFT_eSprite &sprite,
  const char* text
);

int sevenSegPairGap(
  char a,
  char b
);

void drawBoldSmallLabel(
  const char* text,
  int x,
  int y
);

void drawGPSLine(
  bool force = false
);

void drawHeaderGrid(
  bool force = false
);

void drawCallsign();

void drawPowerStatus();

void drawHorizontalUSPlugIcon(
  int x,
  int y,
  uint16_t color
);

void drawAlarmIndicator();

void drawBellIcon(
  int x,
  int y,
  uint16_t color
);

void drawOptionsScreen();
void drawAlarmScreen();
void drawSetTimeScreen();
void drawCallsignScreen();
void drawNightSettingsScreen();

void drawKeyboardKey(
  int x,
  int y,
  int w,
  int h,
  const char* text
);

void drawOptionButton(
  int x,
  int y,
  int w,
  int h,
  const char* text
);

void drawSmallButton(
  int x,
  int y,
  int w,
  int h,
  const char* text
);

void handleTouch();

void waitForTouchRelease();

void handleClockTouch(
  int x,
  int y
);

void handleOptionsTouch(
  int x,
  int y
);

void handleAlarmTouch(
  int x,
  int y
);

void handleSetTimeTouch(
  int x,
  int y
);

void handleCallsignTouch(
  int x,
  int y
);

void handleNightSettingsTouch(
  int x,
  int y
);

void prepareNightSettingsEditor();
void saveNightSettingsEditor();

void beginCallsignEditor();
void appendCallsignCharacter(char ch);
void deleteCallsignCharacter();
void saveCallsignEditor();

void handleActiveAlarmTouch(
  int x,
  int y
);

void performAdjustment(
  AdjustTarget target,
  int direction
);

void holdRepeat(
  AdjustTarget target,
  int direction
);

void redrawAdjustmentPage(
  AdjustTarget target
);

void prepareManualTimeEditor();

void saveManualDateTime();

bool isLeapYear(
  int year
);

int daysInMonth(
  int year,
  int month
);

void maidenhead(
  double latitude,
  double longitude,
  char* grid
);

const char* monthFullName(
  int month
);

void decimalToDMS(
  double coordinate,
  bool latitude,
  int &degrees,
  int &minutes,
  double &seconds,
  char &hemisphere
);

void initializeBacklight();
void applyBrightness();
void cycleBrightness();

// ============================================================
// OFFLINE LOCATION / TIMEZONE FORWARD DECLARATIONS
// ============================================================

void selectSDBus();
void selectTouchBus();
bool initializeSDDatabases();

uint8_t readU8(File &f);
uint16_t readU16(File &f);
uint32_t readU32(File &f);
int32_t readI32(File &f);
int64_t readI64(File &f);
float readFloatLE(File &f);
bool checkMagic(File &f, const char* expected);

bool loadTimezoneDatabaseHeaders();
bool loadPlaceDatabaseHeaders();
bool loadMarineDatabaseHeader();
bool lookupMarineArea(double latitude, double longitude, char* result, size_t resultSize);
bool pointInMarinePolygon(File &f, uint32_t pointCount, double testLon, double testLat);
int lookupZoneID(double latitude, double longitude, bool &nautical, int &nauticalOffset);
bool getZoneName(uint8_t zoneID, char* output, size_t outputSize);
bool lookupTimeRule(uint8_t targetZone, time_t utcEpoch, int32_t &offsetSeconds, char* abbreviation, size_t abbreviationSize, time_t &nextTransition);
int nauticalOffsetHours(double longitude);

void resolveGeographicContextIfNeeded(bool force = false);
void updateCurrentTimeRule(time_t utcEpoch, bool force = false);
void getLocalTimeForEpoch(time_t utcEpoch, struct tm &localTime);
time_t localFieldsToUtcEpoch(struct tm localFields);
void formatUTCOffset(int32_t seconds, char* output, size_t outputSize);
void drawLocalHeader();

double distanceKm(double lat1, double lon1, double lat2, double lon2);
int featurePriority(const char* feature);
double placeScore(const PlaceRecord &place, double distance);
bool readPlaceIndexCell(File &indexFile, int row, int col, uint32_t &placeOffset, uint32_t &count);
bool readPlaceRecord(File &f, PlaceRecord &place);
void calculatePlaceSearchBounds(double latitude, double longitude, int &rowMin, int &rowMax, int &colMin, int &colMax);
bool findBestPlace(double latitude, double longitude, PlaceRecord &bestPlace, double &bestDistance, double &bestScore);

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(300);

  loadSettings();

  // GPS N/F
  pinMode(
    GPS_ENABLE_PIN,
    OUTPUT
  );

  digitalWrite(
    GPS_ENABLE_PIN,
    HIGH
  );

  gpsState =
    GPS_AWAKE;

  gpsWakeMillis =
    millis();

  resetStableGPSFixAccumulator();

  // GPS UART
  GPS.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    -1
  );

  // PPS
  pinMode(
    PPS_PIN,
    INPUT
  );

  attachInterrupt(
    digitalPinToInterrupt(PPS_PIN),
    ppsISR,
    RISING
  );

  // Onboard R21 ambient-light sensor
  pinMode(
    LDR_PIN,
    INPUT
  );

  analogReadResolution(
    12
  );

  analogSetPinAttenuation(
    LDR_PIN,
    ADC_11db
  );

  // Keep the ESP32 system clock itself in UTC.
  // Local civil time is calculated from the offline SD timezone database.
  setenv(
    "TZ",
    "UTC0",
    1
  );

  tzset();

  // Display
  tft.init();

  tft.setRotation(1);

  // Configure the CYD backlight for manual PWM brightness control
  // after TFT initialization, then restore the saved brightness.
  initializeBacklight();

  tft.fillScreen(
    backgroundColor()
  );

  utcTimeSprite.createSprite(
    310,
    53
  );

  localTimeSprite.createSprite(
    310,
    53
  );

  gpsSprite.createSprite(
    320,
    33
  );

  // Mount the SD card and validate the offline databases first.
  // The SPI bus is then returned to the touchscreen pins.
  initializeSDDatabases();

  selectTouchBus();

  drawClockScreen();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  serviceGPS();

  servicePPS();

  serviceGPSDutyCycle();

  serviceAlarm();

  serviceClockDisplay();

  handleTouch();
}


