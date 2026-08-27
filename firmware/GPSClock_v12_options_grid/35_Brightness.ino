// ============================================================
// MANUAL TFT BACKLIGHT BRIGHTNESS
// ============================================================

void initializeBacklight() {

  // ESP32 Arduino Core 3.x LEDC API.
  // Attach GPIO21 directly to a 5 kHz, 8-bit PWM generator.
  ledcAttach(
    TFT_BACKLIGHT_PIN,
    BACKLIGHT_PWM_FREQ,
    BACKLIGHT_PWM_RESOLUTION
  );

  applyBrightness();
}

void applyBrightness() {

  brightnessLevel =
    constrain(
      brightnessLevel,
      0,
      3
    );

  ledcWrite(
    TFT_BACKLIGHT_PIN,
    BRIGHTNESS_DUTY[
      brightnessLevel
    ]
  );
}

void cycleBrightness() {

  brightnessLevel =
    (
      brightnessLevel + 1
    ) % 4;

  applyBrightness();

  saveSettings();
}

