#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleMouse.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define BATTERY_PIN 39
const float DIV_RATIO = 2.0;
const float ADC_REF = 3.3;
const int ADC_RES = 4095;
const float BATT_MIN = 3.0;
const float BATT_MAX = 4.2;
const int BATT_SAMPLES = 20;
#define BATT_SMOOTH 0.95

Adafruit_MPU6050 mpu;

#define FLEX_INDEX 34
#define FLEX_MIDDLE 35
#define FLEX_THUMB 36
#define FLEX_RING 32
#define FLEX_PINKY 33

int FLEX_INDEX_THRESH = 50;
int FLEX_MIDDLE_THRESH = 160;
int FLEX_THUMB_THRESH = 150;
int FLEX_RING_THRESH = 60;
int FLEX_PINKY_THRESH = 100;

#define GYRO_DEAD 0.10
#define CURSOR_SENSITIVITY 16.0
#define MOTION_THRESHOLD 0.15

#define SCROLL_SENSITIVITY 1.5
#define SCROLL_TILT_DEAD 0.25

float gyroYawOffset = 0;
float gyroPitchOffset = 0;
float gyroRollOffset = 0;

unsigned long lastUpdate = 0;
unsigned long lastDisplayUpdate = 0;

BleMouse bleMouse("Vlink", "VisionLabs", 100);

bool leftPressed = false;
bool rightPressed = false;
bool dragging = false;
bool scrolling = false;
float smoothedBatteryVoltage = 0;
bool isCalibrated = false;
bool mpuInitialized = false;

float readBatteryVoltage() {
  float sum = 0;
  for (int i = 0; i < BATT_SAMPLES; i++) {
    int raw = analogRead(BATTERY_PIN);
    sum += (raw * ADC_REF / ADC_RES) * DIV_RATIO;
    delayMicroseconds(100);
  }
  float voltage = sum / BATT_SAMPLES;
  
  if (smoothedBatteryVoltage == 0) {
    smoothedBatteryVoltage = voltage;
  } else {
    smoothedBatteryVoltage = (smoothedBatteryVoltage * BATT_SMOOTH) + (voltage * (1.0 - BATT_SMOOTH));
  }
  return smoothedBatteryVoltage;
}

int getBatteryPercent() {
  float voltage = readBatteryVoltage();
  int percent = (int)((voltage - BATT_MIN) / (BATT_MAX - BATT_MIN) * 100.0);
  return constrain(percent, 0, 100);
}

void drawBatteryIcon(int x, int y, int percent) {
  display.drawRect(x, y, 12, 7, SSD1306_WHITE);
  display.fillRect(x + 12, y + 2, 2, 3, SSD1306_WHITE);
  int fillWidth = map(percent, 0, 100, 0, 10);
  if (fillWidth > 0) {
    display.fillRect(x + 1, y + 1, fillWidth, 5, SSD1306_WHITE);
  }
  if (percent < 20 && (millis() / 500) % 2 == 0) {
    display.drawRect(x, y, 12, 7, SSD1306_WHITE);
  }
}

void calibrateGyro() {
  const int samples = 100;  
  float sumYaw = 0, sumPitch = 0, sumRoll = 0;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Calibrating...");
  display.setCursor(10, 35);
  display.println("Keep still!");
  display.display();
  
  for (int i = 0; i < samples; i++) {
    if (i % 25 == 0) {
      int progress = map(i, 0, samples, 0, 100);
      display.fillRect(16, 47, 96, 4, SSD1306_BLACK);
      display.drawRect(14, 45, 100, 8, SSD1306_WHITE);
      display.fillRect(16, 47, map(progress, 0, 100, 0, 96), 4, SSD1306_WHITE);
      display.display();
    }
    
    sensors_event_t a, g, t;
    if (!mpu.getEvent(&a, &g, &t)) {
      Serial.println("MPU read failed during calibration!");
      delay(10);
      continue;
    }
    
    sumYaw += g.gyro.z;
    sumPitch += g.gyro.y;
    sumRoll += g.gyro.x;
    delay(10);
  }
  
  gyroYawOffset = sumYaw / samples;
  gyroPitchOffset = sumPitch / samples;
  gyroRollOffset = sumRoll / samples;
  
  isCalibrated = true;
  
  Serial.printf("Gyro Offsets - Yaw:%.3f Pitch:%.3f Roll:%.3f\n", 
                gyroYawOffset, gyroPitchOffset, gyroRollOffset);
  
  display.fillRect(16, 47, 96, 4, SSD1306_WHITE);
  display.display();
  delay(500);
}

void showStatusAnimated(const char* msg) {
  static int dotCount = 0;
  unsigned long now = millis();
  
  if (now - lastDisplayUpdate < 500) return;
  lastDisplayUpdate = now;
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2 - 10);
  display.println(msg);
  
  String dots = "";
  for (int i = 0; i < dotCount; i++) dots += ".";
  display.getTextBounds(dots.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2 + 10);
  display.print(dots);
  dotCount = (dotCount + 1) % 4;
  
  display.setTextSize(1);
  int battPct = getBatteryPercent();
  display.setCursor(82, 2);
  display.printf("%d%%", battPct);
  drawBatteryIcon(108, 2, battPct);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(100);  
  Serial.println("Vlink Starting");
  
  Wire.begin();
  analogReadResolution(12);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display init failed!");
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 20);
  display.println("Vlink");
  display.display();
  delay(1500);

  Serial.println("Initializing MPU6050...");
  int mpuRetries = 0;
  while (!mpuInitialized && mpuRetries < 5) {
    if (mpu.begin()) {
      mpuInitialized = true;
      Serial.println("MPU6050 initialized!");
    } else {
      mpuRetries++;
      Serial.printf("MPU init attempt %d failed, retrying...\n", mpuRetries);
      
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(10, 20);
      display.printf("MPU Init %d/5", mpuRetries);
      display.display();
      delay(500);
    }
  }
  
  if (!mpuInitialized) {
    Serial.println("MPU6050 initialization failed!");
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 25);
    display.println("MPU ERR");
    display.display();
    while (1);
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);

  Serial.println("Starting calibration...");
  calibrateGyro();
  Serial.println("Calibration complete!");

  Serial.println("Starting BLE...");
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 28);
  display.println("Starting BLE...");
  display.display();
  
  bleMouse.begin();
  delay(500); 
  
  smoothedBatteryVoltage = readBatteryVoltage();
  lastDisplayUpdate = millis();
  lastUpdate = millis();
  
  Serial.println("Setup Complete");
  Serial.println("Waiting for BLE connection");
}

void loop() {
  if (!bleMouse.isConnected()) {
    showStatusAnimated("Waiting");
    delay(100);
    return;
  }

  static bool shownConnected = false;
  if (!shownConnected) {
    Serial.println("BLE CONNECTED");
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("Connected");
    display.display();
    delay(1500);
    shownConnected = true;
    Serial.println("Mouse Mode Active");
  }

  sensors_event_t a, g, t;
  if (!mpu.getEvent(&a, &g, &t)) {
    Serial.println("MPU read error!");
    delay(10);
    return;
  }
  
  unsigned long now = millis();
  float dt = (now - lastUpdate) / 1000.0;
  lastUpdate = now;

  float yawRate = g.gyro.z - gyroYawOffset;
  float pitchRate = g.gyro.y - gyroPitchOffset;
  float rollRate = g.gyro.x - gyroRollOffset;

  if (fabs(yawRate) < GYRO_DEAD) yawRate = 0;
  if (fabs(pitchRate) < GYRO_DEAD) pitchRate = 0;
  if (fabs(rollRate) < GYRO_DEAD) rollRate = 0;
  
  int8_t moveX = 0;
  int8_t moveY = 0;
  
  if (fabs(yawRate) > MOTION_THRESHOLD) {
    moveX = constrain((int8_t)(-yawRate * CURSOR_SENSITIVITY), -127, 127);
  }
  if (fabs(pitchRate) > MOTION_THRESHOLD) {
    moveY = constrain((int8_t)(-pitchRate * CURSOR_SENSITIVITY), -127, 127);
  }
 
  int8_t scrollAmount = 0;
  if (scrolling && fabs(pitchRate) > SCROLL_TILT_DEAD) {
    scrollAmount = constrain((int8_t)(pitchRate * SCROLL_SENSITIVITY), -3, 3);
  }

  int fI = analogRead(FLEX_INDEX);
  int fM = analogRead(FLEX_MIDDLE);
  int fT = analogRead(FLEX_THUMB);
  int fR = analogRead(FLEX_RING);
  int fP = analogRead(FLEX_PINKY);

  bool iBent = fI > FLEX_INDEX_THRESH;
  bool mBent = fM > FLEX_MIDDLE_THRESH;
  bool tBent = fT > FLEX_THUMB_THRESH;
  bool rBent = fR > FLEX_RING_THRESH;
  bool pBent = fP > FLEX_PINKY_THRESH;

  if (tBent && !dragging) {
    if (leftPressed) {
      bleMouse.release(MOUSE_LEFT);
      leftPressed = false;
    }
    if (rightPressed) {
      bleMouse.release(MOUSE_RIGHT);
      rightPressed = false;
    }
    scrolling = false;
    
    bleMouse.press(MOUSE_LEFT);
    dragging = true;
    Serial.println("→ DRAG START");
  } else if (!tBent && dragging) {
    bleMouse.release(MOUSE_LEFT);
    dragging = false;
    Serial.println("→ DRAG END");
  }

  if (mBent && !scrolling && !dragging) {
    if (leftPressed) {
      bleMouse.release(MOUSE_LEFT);
      leftPressed = false;
    }
    if (rightPressed) {
      bleMouse.release(MOUSE_RIGHT);
      rightPressed = false;
    }
    
    scrolling = true;
    Serial.println("→ SCROLL MODE ON");
  } else if (!mBent && scrolling) {
    scrolling = false;
    Serial.println("→ SCROLL MODE OFF");
  }

  if (!dragging && !scrolling) {
    if (iBent && !leftPressed && !rBent) {
      bleMouse.press(MOUSE_LEFT);
      leftPressed = true;
      Serial.println("→ LEFT DOWN");
    } else if ((!iBent || rBent) && leftPressed) {
      bleMouse.release(MOUSE_LEFT);
      leftPressed = false;
      Serial.println("→ LEFT UP");
    }

    if (rBent && !rightPressed && !iBent) {
      bleMouse.press(MOUSE_RIGHT);
      rightPressed = true;
      Serial.println("→ RIGHT DOWN");
    } else if ((!rBent || iBent) && rightPressed) {
      bleMouse.release(MOUSE_RIGHT);
      rightPressed = false;
      Serial.println("→ RIGHT UP");
    }
  }

  if (!scrolling && (moveX != 0 || moveY != 0)) {
    bleMouse.move(moveX, moveY, 0);
  } else if (scrolling && scrollAmount != 0) {
    bleMouse.move(0, 0, scrollAmount);
  }

  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int battPct = getBatteryPercent();
  display.setCursor(82, 2);
  display.printf("%d%%", battPct);
  drawBatteryIcon(108, 2, battPct);
  
  display.setTextSize(2);
  display.setCursor(5, 15);
  display.print("Mouse Mode");
  
  display.setTextSize(1);
  
  String control = "";
  if (scrolling) {
    control = "SCROLL";
  } else if (dragging) {
    control = "DRAG";
  } else if (leftPressed) {
    control = "LEFT CLICK";
  } else if (rightPressed) {
    control = "RIGHT CLICK";
  } else {
    control = "Ready";
  }
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(control.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 38);
  display.print(control);
  
  display.display();

  delay(10);
}
