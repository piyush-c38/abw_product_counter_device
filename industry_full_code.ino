//pins not to use
// GPIO 36, 39, 34, 35 → ❌ Do not use
// GPIO 0 → risky, avoid if possible
// GPIO 2, 15 → can be used but have boot mode restrictions

#include <WiFi.h>
#include <HX711.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <EEPROM.h>

// === WiFi Credentials ===
const char* ssid = "TP-Link_76F6";
const char* password = "abwtplinkpass";

// === MQTT Settings ===
const char* mqtt_server = "192.168.0.107";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// === LCD ===
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === HX711 ===
#define DT 4
#define SCK 5
HX711 scale;
float calibration_factor = -15.44;

// === EEPROM Settings ===
#define EEPROM_SIZE 32
#define CALIB_ADDR 0

// === Table Settings ===
const int table_id = 21;
const float unit_weight = 200.0;

// === Keypad ===
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {12, 14, 27, 26};
byte colPins[COLS] = {25, 33, 32,13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// === Variables ===
String job_id = "";
String process_id = "";
int product_count = 0;
float measured_weight = 0.0;
bool job_registered = false;
const unsigned long send_interval = 10000;
unsigned long last_sent = 0;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  lcd.print("Connecting WiFi");
  // Serial.println("Connecting WiFi");
  WiFi.begin(ssid, password);
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    lcd.print(".");
    // Serial.print(".");
    wifi_attempts++;
  }
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected");
    // Serial.println("WiFi Connected");
    client.setServer(mqtt_server, mqtt_port);
    reconnect_mqtt();
  } else {
    lcd.print("WiFi Failed!");
    // Serial.println("WiFi Failed!");
    while (1);
  }
  delay(1000);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(CALIB_ADDR, calibration_factor);
  if (isnan(calibration_factor) || calibration_factor == 0 || calibration_factor < -100000 || calibration_factor > 100000)
    calibration_factor = -15.44;

  lcd.clear();
  lcd.print("Calib:");
  lcd.print(calibration_factor, 1);
  // Serial.print("Calib: ");
  // Serial.println(calibration_factor, 1);
  delay(1500);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();

  lcd.clear();
  lcd.print("D=Calibrate");
  // Serial.println("D=Calibrate");
  delay(2000);
  char key = waitForKey(3000);
  if (key == 'D') calibrate();

  lcd.clear();
  lcd.print("System Ready");
  // Serial.println("System Ready");
  delay(1000);
  lcd.clear();
  job_registration();
}

void loop() {
  if (!job_registered) return;

  measured_weight = scale.get_units(10);
  product_count = (int)(measured_weight / unit_weight);

  lcd.setCursor(0, 0);
  lcd.print("Wt:");
  lcd.print(measured_weight, 1);
  lcd.print("g    ");
  // Serial.print("Wt: ");
  // Serial.print(measured_weight, 1);
  // Serial.println(" g");

  lcd.setCursor(0, 1);
  lcd.print("Cnt:");
  lcd.print(product_count);
  lcd.print("    ");
  // Serial.print("Count: ");
  // Serial.println(product_count);

  if (millis() - last_sent >= send_interval) {
    send_info();
    last_sent = millis();
  }

  char key = keypad.getKey();
  if (key == 'A') {
    lcd.clear();
    lcd.print("Ending Job...");
    // Serial.println("Ending Job...");
    delay(1000);
    job_registered = false;
    resetJob();
    job_registration();
  }

  client.loop();
  delay(100);
}

void job_registration() {
  lcd.clear();
  lcd.print("Enter Job ID:");
  // Serial.println("Enter Job ID:");
  job_id = getInputFromKeypad();

  lcd.clear();
  lcd.print("Enter Proc ID:");
  // Serial.println("Enter Proc ID:");
  process_id = getInputFromKeypad();

  lcd.clear();
  lcd.print("Press A to Start");
  // Serial.println("Press A to Start");
  while (keypad.getKey() != 'A') delay(10);

  job_registered = true;
  lcd.clear();
  lcd.print("Started Job");
  // Serial.println("Started Job");
  delay(1000);
}

String getInputFromKeypad() {
  String input = "";
  char key;
  lcd.setCursor(0, 1);
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      else if (key == '*') {
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
        Serial.println("Input cleared");
        lcd.setCursor(0, 1);
      } else if (key >= '0' && key <= '9') {
        input += key;
        lcd.print(key);
        Serial.print(key);
      }
    }
  }
  Serial.println(); // Move to new line
  return input;
}

void calibrate() {
  lcd.clear();
  lcd.print("Taring...");
  scale.set_scale();
  scale.tare();
  delay(2000);

  lcd.clear();
  lcd.print("Put weight now");
  //write a function to display reverse counter on LCD.
  delay(5000);

  long reading = scale.get_units(10);
  Serial.print("Raw reading: ");
  Serial.println(reading);

  lcd.clear();
  lcd.print("Enter Wt(g)#");
  String known_weight_str = getInputFromKeypad();
  float known_weight = known_weight_str.toFloat();

  calibration_factor = (float)reading / known_weight;
  scale.set_scale(calibration_factor);

  lcd.clear();
  lcd.print("Calib:");
  lcd.print(calibration_factor, 1);
  delay(1500);

  EEPROM.put(CALIB_ADDR, calibration_factor);
  EEPROM.commit();

  lcd.clear();
  lcd.print("Saved EEPROM");
  delay(1000);
}

void reconnect_mqtt() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) break;
    delay(2000);
  }
}
void send_info() {
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  String payload = String("{\"table_id\":") + table_id +
                 ",\"job_id\":\"" + job_id +
                 "\",\"process_id\":\"" + process_id +
                 "\",\"weight\":" + measured_weight +
                 ",\"count\":" + product_count + "}";
  client.publish(("table/" + String(table_id) + "/data").c_str(), payload.c_str());
  Serial.println("Published: " + payload);
}

void resetJob() {
  job_id = "";
  process_id = "";
  product_count = 0;
  measured_weight = 0;
  last_sent = 0;
}

char waitForKey(int timeout_ms) {
  unsigned long start = millis();
  while (millis() - start < timeout_ms) {
    char key = keypad.getKey();
    if (key) return key;
  }
  return 0;
}