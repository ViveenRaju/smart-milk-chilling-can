/*
  Smart Two-Tier Milk Chilling — Tier 2 Can Firmware (ESP32)
  -----------------------------------------------------------
  Reads a DS18B20 temperature probe and an analog pH sensor and pushes both
  to Firebase Realtime Database for the collection-centre dashboard.

  Tier 2 cooling model (per the current project abstract):
    - The Peltier probe is a REMOVABLE unit used only during the ~1 hour
      loading window, engaging as soon as the first milk is poured.
    - Once loading finishes, the Peltier probe is physically swapped out
      for the sealed OP05 PCM insert through the same central opening.
      From that point PCM alone holds 4-8 degC for the rest of transport —
      this firmware does NOT drive any cooling after the loading window.
    - This is a one-shot event per can, started locally by a push button
      (there is no "auto/manual" mode and no website override any more —
      the previous relay-based always-on threshold control has been
      removed entirely).

  Peltier efficiency idea — tiered PWM duty, not just on/off:
    Rather than switching the Peltier fully on whenever milk is above the
    safe ceiling, the MOSFET is driven with PWM at a duty cycle that scales
    with how far out of range the reading is, so the module isn't drawing
    peak current for a reading that's only slightly warm:
      > 20.0 degC  -> 100% duty  (far out of range, push hard)
      > 12.0 degC  ->  75% duty
      >  8.0 degC  ->  50% duty  (just above the ceiling)
      <= 8.0 degC  ->   0% duty  (already in range — let it coast)
    This is a normal way to run a TEC/Peltier module off a MOSFET (it's
    the same principle a bench TEC controller uses) so the idea is sound.
    The exact bands and duty percentages below are a starting point —
    they are easy to retune once you have real thermal test data from the
    can, and are only in effect during the one-hour loading window.

  Firebase data model:
    /milkCan
      /canId          string   e.g. "can-01"
      /temperature    float
      /phLevel        float
      /lastUpdated    unix timestamp (seconds)
      /loading
        /active         bool   <- true only during the ~1 hour loading window
        /elapsedSec     int    <- seconds since the loading button was pressed
        /peltierDuty    int    <- 0/50/75/100, current commanded duty (telemetry only)
    The website no longer reads /control or /peltierOn — it computes its own
    SAFE / CAUTION / DANGER verdict on the client from /temperature and
    /phLevel, so this firmware's only job is to report those two values
    honestly and often.

  Wiring:
    DS18B20 data pin        -> GPIO 4    (with a 4.7k pull-up resistor to 3.3V)
    Buzzer/LED failsafe     -> GPIO 5
    Peltier MOSFET gate     -> GPIO 25   (PWM — drives the MOSFET gate, which
                                          switches the Peltier's supply. Do
                                          NOT wire the Peltier directly to a
                                          GPIO, it draws far more current
                                          than the pin can source. Add a
                                          flyback diode across the Peltier
                                          leads as a precaution even though
                                          it's a resistive-ish load, since
                                          wiring inductance can still spike
                                          the MOSFET on switch-off.)
    Loading start button    -> GPIO 27   (momentary push button to GND,
                                          internal pull-up enabled — press
                                          once when the first milk is poured
                                          to start the one-hour window)
    pH sensor analog out    -> GPIO 34   (ADC1, input-only pin — safe choice on ESP32)

  Libraries required (install via Arduino Library Manager):
    - "Firebase ESP32 Client" by Mobizt
    - "OneWire" by Jim Studt / Paul Stoffregen
    - "DallasTemperature" by Miles Burton

  pH calibration (IMPORTANT — do this before trusting any pH reading):
    1. Upload this firmware once and open Serial Monitor.
    2. Dip the probe in pH 7.0 buffer solution, let it settle ~30s, note the
       printed raw voltage, and set PH7_VOLTAGE to that value below.
    3. Dip the probe in pH 4.0 buffer solution, let it settle ~30s, note the
       printed raw voltage, and set PH4_VOLTAGE to that value below.
    4. Set PH_DEMO_MODE to false and re-upload. Readings are simulated and
       NOT meaningful until this is done — the probe isn't wired in yet.

  OLED: not wired in yet. When you add one (SSD1306 over I2C is the common
  choice), the natural spot to draw it is right after the Serial.printf in
  loop() below — it would show the same temp/pH/verdict already being
  computed there.

  Board: any ESP32 dev board, Arduino core for ESP32 installed via Boards Manager.
*/

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>   // prints token status to Serial for debugging
#include <OneWire.h>
#include <DallasTemperature.h>
#include <time.h>

// ---------------- USER CONFIG — fill these in before uploading ----------------
// Placeholders below — see secrets.h in the repo for actual values.
// Kept out of this file (and off this slide) since it's shared publicly.
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

#define API_KEY         "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL    "https://your-project-default-rtdb.region.firebasedatabase.app"
// --------------------------------------------------------------------------------

#define CAN_ID          "can-01"
#define SAFE_MAX_C      8.0f          // degC — target ceiling, must match the website

#define ONE_WIRE_PIN      4           // DS18B20 data pin
#define BUZZER_PIN        5           // local failsafe buzzer/LED
#define PELTIER_PWM_PIN   25          // MOSFET gate — PWM, tiered duty during loading
#define LOADING_BTN_PIN   27          // momentary push button, starts the loading window
#define PH_SENSOR_PIN     34          // pH sensor analog output (ADC1 pin)

// ---- Loading-window Peltier control ----
#define LOADING_DURATION_MS   (60UL * 60UL * 1000UL)  // 1 hour, per the design
#define PELTIER_PWM_FREQ_HZ   1000
#define PELTIER_PWM_CHANNEL   0
#define PELTIER_PWM_RES_BITS  8        // duty expressed 0-255

// Tiered duty bands — how far above SAFE_MAX_C before each duty kicks in.
// Tune these against real thermal logs once you have them.
#define BAND_HIGH_OFFSET_C    12.0f    // temp > SAFE_MAX_C + this -> 100%
#define BAND_MID_OFFSET_C      4.0f    // temp > SAFE_MAX_C + this -> 75%
// temp any amount above SAFE_MAX_C (but below BAND_MID) -> 50%
// temp at/below SAFE_MAX_C -> 0% (coast, save battery)

// ---- pH calibration — fill these in using the two-point calibration
// process in the header comment above. These placeholder values are NOT
// accurate and will give a wrong reading until calibrated. ----
#define PH7_VOLTAGE     2.50f         // raw voltage reading in pH 7.0 buffer
#define PH4_VOLTAGE     2.03f         // raw voltage reading in pH 4.0 buffer

// ---- Demo mode — no pH sensor wired in yet ----
// While true, readPH() ignores GPIO 34 entirely and instead simulates a
// milk sample slowly souring over a ~2 minute repeating cycle, so the
// dashboard visibly moves through fresh -> souring -> spoiling and resets,
// without any real hardware. Flip to false once a real probe is wired in
// and calibrated (see instructions above).
#define PH_DEMO_MODE true
#define PH_FRESH_MIN    6.4f          // kept for reference; verdict logic now lives on the website
#define PH_SOURING_MIN  6.0f

#define READ_INTERVAL_MS   3000       // how often to sample sensors + push telemetry
#define BUTTON_DEBOUNCE_MS  250

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastReadMs = 0;

bool firebaseReady = false;
unsigned long lastAuthAttemptMs = 0;
#define AUTH_RETRY_MS 5000   // how often to retry sign-up if not yet authenticated

// ---- Loading-window state ----
bool loadingActive = false;
unsigned long loadingStartMs = 0;
int lastButtonState = HIGH;
unsigned long lastButtonChangeMs = 0;

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(300);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connect timed out — will keep retrying in background");
  }
}

void setupTime() {
  // Needed so /lastUpdated is a real unix timestamp the website can format.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Syncing time");
  time_t now = time(nullptr);
  unsigned long start = millis();
  while (now < 8 * 3600 * 2 && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" done");
}

// Attempts an anonymous Firebase sign-in. Called from setup(), and retried
// from loop() if it hasn't succeeded yet — covers the case where WiFi wasn't
// up yet, or the first attempt hit a transient network error. Without this
// retry, a single failed sign-up at boot would leave the device permanently
// unauthenticated until physically rebooted.
void attemptFirebaseSignUp() {
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase anonymous sign-up OK");
    firebaseReady = true;
  } else {
    Serial.printf("Firebase sign-up failed: %s\n", config.signer.signupError.message.c_str());
    Serial.println("  -> Will retry. Check that Anonymous auth is enabled in the Firebase console.");
    firebaseReady = false;
  }
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // The Firebase ESP32 library needs a signed-in user — even an anonymous
  // one — before it will send ANY request. This is a library requirement,
  // separate from your database rules: open rules (.read/.write: true) only
  // control who's ALLOWED to access data once a request is actually sent.
  //
  // Requires: Firebase console -> Authentication -> Sign-in method ->
  // Anonymous -> enabled.
  config.token_status_callback = tokenStatusCallback; // logs token refresh events to Serial
  attemptFirebaseSignUp();

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// Reads the loading-start button with simple debounce. Returns true exactly
// once, on the press that starts (or would start) the window.
bool loadingButtonPressed() {
  int reading = digitalRead(LOADING_BTN_PIN);
  bool pressed = false;
  if (reading != lastButtonState && millis() - lastButtonChangeMs > BUTTON_DEBOUNCE_MS) {
    lastButtonChangeMs = millis();
    if (reading == LOW) pressed = true;   // active-low, internal pull-up
    lastButtonState = reading;
  }
  return pressed;
}

// Maps the current temperature to a tiered Peltier duty percent. Only
// called while loadingActive is true — see the header comment for the
// reasoning behind the bands.
int peltierDutyForTemp(float tempC) {
  if (tempC > SAFE_MAX_C + BAND_HIGH_OFFSET_C) return 100;
  if (tempC > SAFE_MAX_C + BAND_MID_OFFSET_C)  return 75;
  if (tempC > SAFE_MAX_C)                      return 50;
  return 0;   // already at/below target — coast, save battery
}

void applyPeltierDuty(int dutyPercent) {
  int duty255 = (int)((dutyPercent / 100.0f) * 255.0f);
  ledcWrite(PELTIER_PWM_CHANNEL, duty255);
}

// Reads the pH sensor, averaging several samples to reduce noise, then
// converts the raw voltage to a pH value using the two-point calibration
// constants above. Prints the raw voltage too, since that's what you need
// to read off during calibration.
float readPH() {
#if PH_DEMO_MODE
  // Simulated reading — see PH_DEMO_MODE comment above. Cycles from a fresh
  // 6.8 down to a spoiling 5.3 over 2 minutes, then repeats, with a little
  // random jitter so it doesn't look like an obviously fake straight line.
  float cyclePosition = fmodf(millis() / 1000.0f, 120.0f);   // 0-120s, repeating
  float simulatedPh = 6.8f - (cyclePosition / 120.0f) * 1.5f; // 6.8 -> 5.3
  simulatedPh += random(-5, 6) / 100.0f;                      // ±0.05 jitter
  Serial.printf("pH DEMO MODE (no sensor) — simulated value: %.2f\n", simulatedPh);
  return simulatedPh;
#else
  const int samples = 20;
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(PH_SENSOR_PIN);
    delay(5);
  }
  float avgReading = total / (float)samples;
  float voltage = avgReading / 4095.0f * 3.3f;   // ESP32 ADC: 12-bit, 3.3V reference

  float slope = (PH4_VOLTAGE - PH7_VOLTAGE) / (4.0f - 7.0f);
  float ph = 7.0f + (voltage - PH7_VOLTAGE) / slope;

  Serial.printf("pH raw voltage: %.3f V (use this to calibrate PH7_VOLTAGE/PH4_VOLTAGE)\n", voltage);
  return ph;
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PH_SENSOR_PIN, INPUT);
  pinMode(LOADING_BTN_PIN, INPUT_PULLUP);
  digitalWrite(BUZZER_PIN, LOW);

  ledcSetup(PELTIER_PWM_CHANNEL, PELTIER_PWM_FREQ_HZ, PELTIER_PWM_RES_BITS);
  ledcAttachPin(PELTIER_PWM_PIN, PELTIER_PWM_CHANNEL);
  ledcWrite(PELTIER_PWM_CHANNEL, 0);   // Peltier off until loading starts

  sensors.begin();

  connectWiFi();
  setupTime();
  setupFirebase();

  Serial.println("Ready. Press the loading button once the first milk is poured to start the 1-hour cooling window.");
}

void loop() {
  unsigned long nowMs = millis();

  if (!firebaseReady && WiFi.status() == WL_CONNECTED && nowMs - lastAuthAttemptMs >= AUTH_RETRY_MS) {
    lastAuthAttemptMs = nowMs;
    attemptFirebaseSignUp();
  }

  // Start the loading window on a button press, but only if one isn't
  // already running — a stray extra press mid-window doesn't restart the
  // clock.
  if (loadingButtonPressed() && !loadingActive) {
    loadingActive = true;
    loadingStartMs = nowMs;
    Serial.println("Loading window started — Peltier armed for 1 hour.");
  }

  // End the window automatically once the hour is up. From here on, PCM
  // alone is expected to hold the can in range — this firmware drives no
  // further cooling until the button is pressed again for the next can.
  if (loadingActive && nowMs - loadingStartMs >= LOADING_DURATION_MS) {
    loadingActive = false;
    applyPeltierDuty(0);
    Serial.println("Loading window ended — Peltier off, handing over to the PCM insert.");
  }

  if (nowMs - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = nowMs;

    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);

    if (tempC == DEVICE_DISCONNECTED_C) {
      Serial.println("Sensor read error — check DS18B20 wiring, skipping this cycle");
      return;
    }

    float ph = readPH();
    bool overThreshold = tempC > SAFE_MAX_C;

    // Local failsafe alert — fires independent of WiFi/Firebase.
    digitalWrite(BUZZER_PIN, overThreshold ? HIGH : LOW);

    int peltierDuty = 0;
    if (loadingActive) {
      peltierDuty = peltierDutyForTemp(tempC);
      applyPeltierDuty(peltierDuty);
    }

    unsigned long elapsedSec = loadingActive ? (nowMs - loadingStartMs) / 1000UL : 0;

    Serial.printf("Temp: %.2f C | pH: %.2f | loading: %s (%lus) | peltier duty: %d%%\n",
                  tempC, ph, loadingActive ? "yes" : "no", elapsedSec, peltierDuty);

    if (WiFi.status() == WL_CONNECTED && firebaseReady) {
      time_t nowSec = time(nullptr);

      if (!Firebase.setFloat(fbdo, "/milkCan/temperature", tempC)) {
        Serial.println("setFloat(temperature) failed: " + fbdo.errorReason());
      }
      if (!Firebase.setFloat(fbdo, "/milkCan/phLevel", ph)) {
        Serial.println("setFloat(phLevel) failed: " + fbdo.errorReason());
      }
      if (!Firebase.setString(fbdo, "/milkCan/canId", CAN_ID)) {
        Serial.println("setString(canId) failed: " + fbdo.errorReason());
      }
      if (!Firebase.setInt(fbdo, "/milkCan/lastUpdated", (int)nowSec)) {
        Serial.println("setInt(lastUpdated) failed: " + fbdo.errorReason());
      }
      // Loading/Peltier telemetry — useful for your own logs and testing;
      // the website dashboard does not currently read these.
      if (!Firebase.setBool(fbdo, "/milkCan/loading/active", loadingActive)) {
        Serial.println("setBool(loading/active) failed: " + fbdo.errorReason());
      }
      if (!Firebase.setInt(fbdo, "/milkCan/loading/elapsedSec", (int)elapsedSec)) {
        Serial.println("setInt(loading/elapsedSec) failed: " + fbdo.errorReason());
      }
      if (!Firebase.setInt(fbdo, "/milkCan/loading/peltierDuty", peltierDuty)) {
        Serial.println("setInt(loading/peltierDuty) failed: " + fbdo.errorReason());
      }
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected — loading logic still runs locally, but the website won't update");
    } else {
      Serial.println("Firebase not authenticated yet — loading logic still runs locally, retrying sign-up in background");
    }
  }
}
