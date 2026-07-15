/*
 * Digital Level
 * Arduino Nano ESP32 + MPU-6050 + Buzzer + Green LED + Cal Button + Cal LED
 * BLE Nordic UART + Standalone mode + Persistent calibration
 *
 * Version: 2.5
 * Date:    2026-07-10
 *
 * New in v2.5 (changes from v2.4):
 *   - Fixed: standalone buzzer no longer sounds during the calibration
 *     sequence (CAL_POS1 / CAL_WAIT2 / CAL_POS2 / CAL_DONE). Previously the
 *     buzzer was driven unconditionally by the standalone level check on
 *     every loop() iteration, with no awareness of calibration state, so it
 *     rapidly buzzed on/off as pitch crossed the level threshold while the
 *     unit was being rotated through calibration positions. The buzzer is
 *     now suppressed for the full duration of any active calibration or
 *     tare-confirmation state (calState != CAL_IDLE); the standalone green
 *     LED_PIN indicator and the CAL_LED_PIN calibration feedback are
 *     unchanged.
 *
 * New in v2.4 (changes from v2.3):
 *   - Auto-detect orientation mode (horizontal vs vertical) each loop iteration
 *   - Horizontal mode (|az| dominant): pitch = atan2(ay, az), roll = atan2(-ax, az)
 *   - Vertical mode   (|ay| dominant): pitch = atan2(az, ay), roll = atan2(-ax, ay)
 *   - Hysteresis band (MODE_HYST = 0.3g) prevents flickering at 45° crossover
 *   - BLE packet extended: "P:x.xx,R:x.xx,M:H" or "M:V" so app can display mode
 *
 * New in v2.3 (changes from v2.2):
 *   - pitchAccel formula changed from atan2(ay, sqrt(ax²+az²)) to atan2(ay, az)
 *
 * New in v1.9 (changes from v1.8):
 *   - Standalone two-position calibration via D8 button + D9 cal LED
 *   - Non-blocking calibration state machine (millis-based, no delay())
 *   - Button calibration disabled while BLE is connected
 *   - Calibration offsets saved to flash (Preferences) -- restored on every boot
 *   - BLE CAL: command also saves offsets to flash
 *
 * New hardware for v1.9:
 *   Cal button: one leg -> D8, other -> GND  (INPUT_PULLUP, active LOW)
 *   Cal LED:    Anode -> D9 via 150 Ohm, Cathode -> GND
 *
 * Standalone calibration flow (BLE disconnected only):
 *   1. Press button -> cal LED fast blinks 5s (sampling position 1)
 *   2. Cal LED solid ON -> rotate device 180 deg end-for-end
 *   3. Press button -> cal LED fast blinks 5s (sampling position 2)
 *   4. Cal LED 3x slow flash -> offsets applied and saved to flash
 *
 * Short press  = instant Tare (saved to flash)
 * Long press (3s) = start two-position calibration
 * BLE TAR: command = remote tare from app
 *
 * Required libraries:
 *   - Adafruit MPU6050, Adafruit Unified Sensor, Adafruit BusIO
 *   - Preferences (built into Arduino ESP32 Boards -- no install needed)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

#define BUZZER_PIN           D7
#define LED_PIN              D6
#define CAL_BTN_PIN          D8   // momentary button (INPUT_PULLUP, active LOW)
#define CAL_LED_PIN          D9   // calibration status LED (via 150 Ohm)
#define LEVEL_THRESHOLD      1.0f
#define STANDALONE_THRESHOLD 0.3f
#define ALPHA                0.96f
#define CAL_DURATION_MS      5000UL
#define MODE_HYST            0.3f   // hysteresis band (g) to avoid flickering at 45°

// Orientation mode
enum OrientMode { MODE_HORIZONTAL, MODE_VERTICAL };
OrientMode orientMode = MODE_HORIZONTAL;

Adafruit_MPU6050 mpu;
Preferences prefs;

BLEServer*         pServer           = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;
bool deviceConnected = false;

float pitch = 0.0f, roll = 0.0f;
unsigned long lastTime = 0, lastSendTime = 0;
float pitchOffset = 0.0f, rollOffset = 0.0f;
float gyroBiasX = 0.0f, gyroBiasY = 0.0f;  // measured at boot, subtracted in filter
String inputBuffer = "";

// -- Calibration state machine
enum CalState { CAL_IDLE, CAL_POS1, CAL_WAIT2, CAL_POS2, CAL_DONE };
CalState calState = CAL_IDLE;

double calPitchSum = 0.0, calRollSum = 0.0;
int    calCount    = 0;
float  pos1Pitch   = 0.0f, pos1Roll = 0.0f;
unsigned long calStartMs    = 0;
unsigned long calFlashMs    = 0;
int           calFlashCount = 0;

// -- Persistent storage
void saveOffsets() {
  prefs.begin("cal", false);
  prefs.putFloat("pitch", pitchOffset);
  prefs.putFloat("roll",  rollOffset);
  prefs.end();
  Serial.print("Offsets saved -- P:"); Serial.print(pitchOffset, 3);
  Serial.print("  R:"); Serial.println(rollOffset, 3);
}

void loadOffsets() {
  prefs.begin("cal", true);
  pitchOffset = prefs.getFloat("pitch", 0.0f);
  rollOffset  = prefs.getFloat("roll",  0.0f);
  prefs.end();
  Serial.print("Offsets loaded -- P:"); Serial.print(pitchOffset, 3);
  Serial.print("  R:"); Serial.println(rollOffset, 3);
}

void seedFromAccel() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float axf = a.acceleration.x / 9.81f;
  float ayf = a.acceleration.y / 9.81f;
  float azf = a.acceleration.z / 9.81f;
  pitch = atan2(ayf, azf) * 180.0f / PI;
  roll  = atan2(-axf, azf) * 180.0f / PI;
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) {
    deviceConnected = true;
    calState = CAL_IDLE;  // abort any in-progress standalone cal
    digitalWrite(CAL_LED_PIN, LOW);
    digitalWrite(LED_PIN,    LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("BLE Connected -- hardware muted, button cal disabled");
  }
  void onDisconnect(BLEServer* s) {
    deviceConnected = false;
    digitalWrite(LED_PIN,    LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("BLE Disconnected -- standalone resumed");
    BLEDevice::startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    std::string rxValue = pChar->getValue();
    for (size_t i = 0; i < rxValue.length(); i++) {
      char c = rxValue[i];
      if (c == '\n' || c == '\r') {
        if (inputBuffer.length() > 0) { processCommand(inputBuffer); inputBuffer = ""; }
      } else { inputBuffer += c; }
    }
  }
  void processCommand(String cmd) {
    cmd.trim();
    if (cmd.startsWith("CAL:")) {
      int comma = cmd.indexOf(',', 4);
      if (comma > 0) {
        pitchOffset = cmd.substring(4, comma).toFloat();
        rollOffset  = cmd.substring(comma + 1).toFloat();
        saveOffsets();
      }
    } else if (cmd.startsWith("TAR:")) {
      int comma = cmd.indexOf(',', 4);
      if (comma > 0) {
        pitchOffset += cmd.substring(4, comma).toFloat();
        rollOffset  += cmd.substring(comma + 1).toFloat();
        saveOffsets();
      }
    } else if (cmd == "LED:1") {
      digitalWrite(LED_PIN, HIGH); digitalWrite(BUZZER_PIN, HIGH);
    } else if (cmd == "LED:0") {
      digitalWrite(LED_PIN, LOW);  digitalWrite(BUZZER_PIN, LOW);
    }
  }
};

// -- Non-blocking calibration state machine
void updateCal() {
  if (deviceConnected) { digitalWrite(CAL_LED_PIN, LOW); return; }

  unsigned long now = millis();

  // Debounce + short/long press detect
  static bool btnStable  = HIGH;
  static bool btnLast    = HIGH;
  static unsigned long debounceMs = 0;
  static unsigned long btnDownMs  = 0;
  static bool longFired  = false;
  bool btnRaw = digitalRead(CAL_BTN_PIN);
  bool btnShortPress = false;
  bool btnLongPress  = false;
  if (btnRaw != btnLast) debounceMs = now;
  btnLast = btnRaw;
  if ((now - debounceMs) > 30 && btnRaw != btnStable) {
    btnStable = btnRaw;
    if (btnStable == LOW) { btnDownMs = now; longFired = false; }
    else if (!longFired)  { btnShortPress = true; }
  }
  if (btnStable == LOW && !longFired && (now - btnDownMs) >= 3000) {
    btnLongPress = true;
    longFired    = true;
  }

  switch (calState) {

    case CAL_IDLE:
      digitalWrite(CAL_LED_PIN, LOW);
      if (btnShortPress) {
        // Short press: instant tare to current orientation -- saved to flash
        pitchOffset = pitch;
        rollOffset  = roll;
        saveOffsets();
        calFlashCount = 0; calFlashMs = now; calState = CAL_DONE;
        Serial.println("Tare applied");
      }
      if (btnLongPress) {
        // Long press (3s): start two-position calibration sequence
        calPitchSum = 0; calRollSum = 0; calCount = 0;
        calStartMs = now;
        calState   = CAL_POS1;
        Serial.println("Cal: position 1 -- hold still...");
      }
      break;

    case CAL_POS1:
      digitalWrite(CAL_LED_PIN, (now / 150) % 2);
      calPitchSum += pitch - pitchOffset;
      calRollSum  += roll  - rollOffset;
      calCount++;
      if (now - calStartMs >= CAL_DURATION_MS) {
        pos1Pitch = (float)(calPitchSum / calCount);
        pos1Roll  = (float)(calRollSum  / calCount);
        calState  = CAL_WAIT2;
        Serial.print("Cal pos1 avg -- P:"); Serial.print(pos1Pitch, 3);
        Serial.print("  R:"); Serial.println(pos1Roll, 3);
        Serial.println("Cal: rotate 180 deg and press button...");
      }
      break;

    case CAL_WAIT2:
      digitalWrite(CAL_LED_PIN, HIGH);
      if (btnShortPress) {
        calPitchSum = 0; calRollSum = 0; calCount = 0;
        calStartMs = now;
        calState   = CAL_POS2;
        Serial.println("Cal: position 2 -- hold still...");
      }
      break;

    case CAL_POS2:
      digitalWrite(CAL_LED_PIN, (now / 150) % 2);
      calPitchSum += pitch - pitchOffset;
      calRollSum  += roll  - rollOffset;
      calCount++;
      if (now - calStartMs >= CAL_DURATION_MS) {
        float pos2Pitch = (float)(calPitchSum / calCount);
        float pos2Roll  = (float)(calRollSum  / calCount);
        pitchOffset += (pos1Pitch + pos2Pitch) / 2.0f;
        rollOffset  += (pos1Roll  + pos2Roll)  / 2.0f;
        saveOffsets();
        calFlashCount = 0;
        calFlashMs    = now;
        calState      = CAL_DONE;
        Serial.print("Cal done -- P:"); Serial.print(pitchOffset, 3);
        Serial.print("  R:"); Serial.println(rollOffset, 3);
      }
      break;

    case CAL_DONE:
      if (calFlashCount < 6) {
        digitalWrite(CAL_LED_PIN, (calFlashCount % 2 == 0) ? HIGH : LOW);
        if (now - calFlashMs >= 300) { calFlashCount++; calFlashMs = now; }
      } else {
        digitalWrite(CAL_LED_PIN, LOW);
        calState = CAL_IDLE;
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN,  OUTPUT);
  pinMode(LED_PIN,     OUTPUT);
  pinMode(CAL_LED_PIN, OUTPUT);
  pinMode(CAL_BTN_PIN, INPUT_PULLUP);
  digitalWrite(BUZZER_PIN,  LOW);
  digitalWrite(LED_PIN,     LOW);
  digitalWrite(CAL_LED_PIN, LOW);

  digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN,    HIGH); delay(200); digitalWrite(LED_PIN,    LOW);

  if (!mpu.begin()) {
    Serial.println("MPU-6050 not found -- check wiring!");
    while (1) { digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); }
  }
  Serial.println("MPU-6050 OK");
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  loadOffsets();
  delay(100);
  seedFromAccel();

  // Gyro bias calibration -- board must be still for ~1s after boot
  Serial.println("Calibrating gyro bias -- keep still...");
  double bx = 0.0, by = 0.0;
  const int BIAS_SAMPLES = 100;
  for (int i = 0; i < BIAS_SAMPLES; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    bx += g.gyro.x;
    by += g.gyro.y;
    delay(10);
  }
  gyroBiasX = (float)(bx / BIAS_SAMPLES);
  gyroBiasY = (float)(by / BIAS_SAMPLES);
  Serial.print("Gyro bias -- X:"); Serial.print(gyroBiasX, 4);
  Serial.print("  Y:"); Serial.println(gyroBiasY, 4);

  lastTime = micros();

  BLEDevice::init("Level");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService* pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic* pRxChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxChar->setCallbacks(new MyCallbacks());
  pService->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("Ready -- v2.5 -- auto H/V mode + gyro bias cal + short press=tare, long press=2-pos cal, BLE TAR: (buzzer muted during cal)");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long now = micros();
  float dt = (now - lastTime) / 1000000.0f;
  lastTime = now;
  if (dt <= 0 || dt > 0.5f) dt = 0.01f;

  float axf = a.acceleration.x / 9.81f;
  float ayf = a.acceleration.y / 9.81f;
  float azf = a.acceleration.z / 9.81f;

  // -- Auto orientation detection with hysteresis
  float absAy = fabs(ayf);
  float absAz = fabs(azf);
  if (orientMode == MODE_HORIZONTAL && absAy > absAz + MODE_HYST) {
    orientMode = MODE_VERTICAL;
    Serial.println("Mode -> VERTICAL");
  } else if (orientMode == MODE_VERTICAL && absAz > absAy + MODE_HYST) {
    orientMode = MODE_HORIZONTAL;
    Serial.println("Mode -> HORIZONTAL");
  }

  float pitchAccel, rollAccel;
  if (orientMode == MODE_HORIZONTAL) {
    pitchAccel = atan2(ayf,  azf) * 180.0f / PI;   // tool flat on surface
    rollAccel  = atan2(-axf, azf) * 180.0f / PI;
  } else {
    pitchAccel = atan2(azf,  ayf) * 180.0f / PI;   // tool standing on edge
    rollAccel  = atan2(-axf, ayf) * 180.0f / PI;
  }

  pitch = ALPHA * (pitch + (g.gyro.y - gyroBiasY) * (180.0f / PI) * dt) + (1.0f - ALPHA) * pitchAccel;
  roll  = ALPHA * (roll  + (g.gyro.x - gyroBiasX) * (180.0f / PI) * dt) + (1.0f - ALPHA) * rollAccel;

  float adjPitch = pitch - pitchOffset;
  float adjRoll  = roll  - rollOffset;

  updateCal();

  if (!deviceConnected) {
    bool isLevel = (fabs(adjPitch) <= STANDALONE_THRESHOLD);
    bool calibrating = (calState != CAL_IDLE);  // v2.5: mute buzzer while cal/tare-confirm is active
    digitalWrite(LED_PIN,    isLevel ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, (isLevel && !calibrating) ? HIGH : LOW);
  }

  if (deviceConnected && (now / 1000 - lastSendTime) >= 100) {
    lastSendTime = now / 1000;
    char buf[40];
    snprintf(buf, sizeof(buf), "P:%.2f,R:%.2f,M:%c", adjPitch, adjRoll, orientMode == MODE_HORIZONTAL ? 'H' : 'V');
    pTxCharacteristic->setValue((uint8_t*)buf, strlen(buf));
    pTxCharacteristic->notify();
  }

  delay(10);
}
