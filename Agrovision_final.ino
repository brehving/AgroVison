/******* BLYNK CONFIG *******/
#define BLYNK_TEMPLATE_ID "TMPL3Hf4mct9b"
#define BLYNK_TEMPLATE_NAME "BitForceCopy"
#define BLYNK_AUTH_TOKEN "wkxdDp_YJ57wRE-AVV9DB8YJjzlFsM6D"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

/******* LCD *******/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

/******* DHT *******/
#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

/******* WIFI *******/
char ssid[] = "Churchil";
char pass[] = "12345678";

/******* PINS *******/
#define SOIL_PIN   34
#define PIR_PIN    27
#define FLAME_PIN  26
#define LED_PIN    25   // Irrigation relay / indicator

/******* THRESHOLDS *******/
#define TEMP_HIGH 35
#define HUM_LOW   40
#define CSI_LIMIT 0.6

/******* SOIL CALIBRATION *******/
#define SOIL_DRY_RAW 3600
#define SOIL_WET_RAW 1800

/******* TIMER *******/
BlynkTimer timer;

/******* GLOBAL VARIABLES *******/
float temperature, humidity, CSI;
int soilRaw, soilPercent;
bool fireDetected, motionDetected;

bool manualOverride = false;

/* Alert flags (anti-spam) */
bool fireAlertSent = false;
bool csiAlertSent = false;
bool irrigationAlertSent = false;

/******* MANUAL IRRIGATION (BLYNK BUTTON V7) *******/
BLYNK_WRITE(V7) {
  manualOverride = param.asInt();   // 1 = Manual ON, 0 = Auto
}

/******* MAIN FUNCTION *******/
void readSensorsAndUpdate() {

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  soilRaw = analogRead(SOIL_PIN);
  motionDetected = digitalRead(PIR_PIN);
  fireDetected = (digitalRead(FLAME_PIN) == LOW);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT SENSOR ERROR");
    return;
  }

  soilPercent = map(soilRaw, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  int soilStress = (soilPercent < 30);
  int tempStress = (temperature > TEMP_HIGH);
  int humStress  = (humidity < HUM_LOW);

  CSI = (soilStress + tempStress + humStress) / 3.0;

  /*** IRRIGATION CONTROL ***/
  if (manualOverride) {
    digitalWrite(LED_PIN, HIGH);   // Manual ON
  } else {
    digitalWrite(LED_PIN, CSI >= CSI_LIMIT ? HIGH : LOW);
  }

  /*** FIRE ALERT ***/
  if (fireDetected && !fireAlertSent) {
    Blynk.logEvent("fire_alert", "🔥 Fire detected in AgroVision field!");
    fireAlertSent = true;
  }
  if (!fireDetected) fireAlertSent = false;

  /*** CSI ALERT ***/
  if (CSI >= CSI_LIMIT && !csiAlertSent) {
    Blynk.logEvent("csi_alert", "⚠ Crop stress high! CSI exceeded safe limit.");
    csiAlertSent = true;
  }
  if (CSI < CSI_LIMIT) csiAlertSent = false;

  /*** IRRIGATION ALERT (AUTO MODE ONLY) ***/
  if (!manualOverride && digitalRead(LED_PIN) == HIGH && !irrigationAlertSent) {
    Blynk.logEvent("irrigation_alert", "💧 Automatic irrigation started due to crop stress.");
    irrigationAlertSent = true;
  }
  if (digitalRead(LED_PIN) == LOW) irrigationAlertSent = false;

  /*** SERIAL MONITOR ***/
  Serial.println("\n------ AGROVISION AI ------");
  Serial.print("Temp: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Hum : "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Soil: "); Serial.print(soilPercent); Serial.println(" %");
  Serial.print("CSI : "); Serial.println(CSI, 2);
  Serial.print("Mode: "); Serial.println(manualOverride ? "MANUAL" : "AUTO");
  Serial.print("Irrigation: "); Serial.println(digitalRead(LED_PIN) ? "ON" : "OFF");
  Serial.print("Fire: "); Serial.println(fireDetected ? "YES" : "NO");
  Serial.print("Motion: "); Serial.println(motionDetected ? "YES" : "NO");

  /*** LCD DISPLAY ***/
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CSI:");
  lcd.print(CSI, 2);
  lcd.print(" Soil:");
  lcd.print(soilPercent);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print(manualOverride ? "MANUAL " : "AUTO   ");
  lcd.print("Irr:");
  lcd.print(digitalRead(LED_PIN) ? "ON " : "OFF");

  /*** BLYNK DATA ***/
  Blynk.virtualWrite(V0, CSI);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, soilPercent);
  Blynk.virtualWrite(V4, digitalRead(LED_PIN));
  Blynk.virtualWrite(V5, fireDetected);
  Blynk.virtualWrite(V6, motionDetected);
}

/******* SETUP *******/
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT_PULLDOWN);
  pinMode(FLAME_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.print("AgroVision AI");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.clear();
  lcd.print("System Ready");

  timer.setInterval(3000L, readSensorsAndUpdate);
}

/******* LOOP *******/
void loop() {
  Blynk.run();
  timer.run();
}