// TFT_eSPI setup for ESP32-2432S028 / "Cheap Yellow Display" (CYD)
// Used by Offline GPS Ham Clock.
//
// Copy this file into TFT_eSPI/User_Setups/ and select it from
// User_Setup_Select.h, or merge these definitions into your User_Setup.h.

#define USER_SETUP_INFO "ESP32-2432S028 GPS Clock"

#define ILI9341_DRIVER

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// Touch is NOT handled by TFT_eSPI in this project. The CYD touch controller
// is on a different SPI pin set and is handled by XPT2046_Touchscreen in the
// main sketch (SCK 25, MISO 39, MOSI 32, CS 33, IRQ 36).

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT7
#define LOAD_GFXFF

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000

// The CYD display bus is the ESP32 HSPI pin set. This keeps the display bus
// separate from the VSPI host dynamically shared by touch and microSD access.
#define USE_HSPI_PORT

