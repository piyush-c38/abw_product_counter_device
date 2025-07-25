#include <WiFi.h>
#include <HX711.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <math.h>

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
float calibration_factor = 16.4;

// === EEPROM Settings ===
#define EEPROM_SIZE 32
#define CALIB_ADDR 0
#define BUFFER_WT 200

// === Table Settings ===
const int table_id = 2;
volatile float unit_weight = -1;

// === Keypad ===
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 13, 12, 14, 27 };
byte colPins[COLS] = { 26, 25, 33, 32 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// === Variables ===
String job_id = "";
String process_id = "";
float measured_weight = 0.0;
float last_measured_weight = 0.0;
int product_count = 0;
bool job_registered = false;
int job_status = 0;
const unsigned long send_interval = 1000;
unsigned long last_sent = 0;
int maxCount = 0;
String remarks = "";
unsigned long last_status_sent = 0;
const unsigned long status_interval = 2000;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("WiFi Connected");
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);  // Set MQTT callback
    reconnect_mqtt();
    lcd.clear();
  } else {
    checkConnections();
  }

  delay(1000);

  EEPROM.begin(EEPROM_SIZE);
  // EEPROM.get(CALIB_ADDR, calibration_factor);
  // if (isnan(calibration_factor) || calibration_factor == 0 || calibration_factor < -100000 || calibration_factor > 100000)
  //   calibration_factor = 15.55;

  lcd.clear();
  lcd.print("Calib:");
  lcd.print(calibration_factor, 1);
  delay(1500);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  delay(100);
  scale.tare();

  // lcd.clear();
  // lcd.print("D=Calibrate");
  // delay(2000);
  // char key = waitForKey(3000);
  // if (key == 'D') calibrate();

  lcd.clear();
  lcd.print("System Ready");

  Serial.println("First time status and info sent...");
  send_info();
  last_sent = millis();
  send_status("online");
  last_status_sent = millis();

  delay(1000);
  lcd.clear();
  job_registration();
}

void loop() {
  checkConnections();
  client.loop();
  Serial.println("Inside main loop...");
  if (!job_registered) return;

  measured_weight = scale.get_units(10);

  //ignoring the small fluctuations at begining
  // if (abs(measured_weight) < 5.0) {
  //   measured_weight = 0.0;
  // }

  // Reject counting if no unit_weight received yet
  if (unit_weight <= 0) return;

  //My Developed: product counting algorithm
  if (fmod(measured_weight, unit_weight) <= BUFFER_WT) {  //Handling Wt+20 range
    product_count = (int)(measured_weight / unit_weight);
  } else if (fmod(measured_weight, unit_weight) > BUFFER_WT) {
    if (fmod((measured_weight + 20), unit_weight) <= BUFFER_WT) {  //Handling Wt-20 range
      product_count = (int)((measured_weight + BUFFER_WT) / unit_weight);
    } else {  //Handling remaining usual cases
      product_count = (int)(measured_weight / unit_weight);
    }
  }

  //storing maxCount yet
  if (product_count > maxCount) {
    maxCount = product_count;
  }

  //avoid the first time continuous taring
  // if (product_count == 0 && maxCount > 1) {
  //   scale.tare();
  //   lcd.clear();
  //   lcd.print("Taring...");
  //   delay(1000);
  //   maxCount = 0;
  // }

  // lcd.setCursor(0, 0);
  // lcd.print("Wt:");
  // lcd.print(measured_weight, 1);
  // lcd.print("g    ");

  lcd.setCursor(0, 0);
  lcd.print("Cnt:");
  lcd.print(product_count);
  lcd.print("    ");

  float added_weight = measured_weight - last_measured_weight;

  if (added_weight > 1.25 * unit_weight) {
    lcd.clear();
    lcd.print("Item Error!");
    delay(2000);
    lcd.clear();
    return;
  }

  if (millis() - last_sent >= send_interval) {
    send_info();
    last_sent = millis();
  }
  Serial.println("Sending status from main loop...");
  if (millis() - last_status_sent > status_interval) {
    send_status("online");
    last_status_sent = millis();
  }

  char key = keypad.getKey();
  if (key == 'A') {
    lcd.clear();
    lcd.print("End Job? A:Yes");
    lcd.setCursor(0, 1);
    lcd.print("         B:No");

    unsigned long start = millis();
    char confirm = NO_KEY;
    while (millis() - start < 5000) {
      confirm = keypad.getKey();
      if (confirm == 'A' || confirm == 'B') break;
    }

    if (confirm == 'B') {
      lcd.clear();
      lcd.print("Cancelled...");
      delay(1000);
      return;
    } else if (confirm == 'A') {
      lcd.clear();
      lcd.print("Ending Job...");
      job_status = 0;
      send_info();
      job_registered = false;
      last_sent = millis();
      delay(1000);
      resetJob();
      job_registration();
      return;
    } else {
      lcd.clear();
      lcd.print("No input...");
      delay(1000);
      return;
    }
  } else if (key == 'C') {
    lcd.clear();
    lcd.print("Restarting...");
    delay(1000);
    ESP.restart();
  }

  last_measured_weight = measured_weight;
  delay(500);
}

void job_registration() {
  lcd.clear();
  lcd.print("Enter Job ID:");
  job_id = getInputFromKeypad();

  lcd.clear();
  lcd.print("Enter Proc ID:");
  process_id = getInputFromKeypad();

  lcd.clear();
  lcd.print("Press A to Start");
  while (keypad.getKey() != 'A') delay(10);

  remarks = "request_unit_weight";
  send_info();
  remarks = "";

  lcd.clear();
  lcd.print("Waiting Weight...");
  Serial.println("Waiting Weight...");

  unsigned long waitStart = millis();
  while (unit_weight <= 0 && millis() - waitStart < 10000) {
    client.loop();  // Let MQTT receive message
    lcd.setCursor(0, 1);
    lcd.print(unit_weight);
    delay(100);
    //send online status
    Serial.println("inside waiting weight loop");
    if (millis() - last_status_sent > status_interval) {
      send_status("online");
      last_status_sent = millis();
    }
  }

  if (unit_weight > 0) {
    job_registered = true;
    job_status = 1;
    lcd.clear();
    lcd.print("Started Job");
    delay(1000);
    lcd.clear();
    Serial.println("Got the Unit weight, inside unt_wt>0 loop...");
    if (millis() - last_status_sent > status_interval) {
      send_status("online");
      last_status_sent = millis();
    }
  } else {
    lcd.clear();
    lcd.print("No weight recvd");
    delay(2000);
    lcd.clear();
    resetJob();
    job_registration();
  }
  scale.tare();
}

String getInputFromKeypad() {
  String input = "";
  char key;
  lcd.setCursor(0, 1);
  while (true) {
    delay(50);
    Serial.println("Inside get Input for keyborad loop...");
    if (millis() - last_status_sent > status_interval) {
      send_status("online");
      last_status_sent = millis();
    }
    key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      else if (key == '*') {
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
      } else if (key >= '0' && key <= '9') {
        input += key;
        lcd.print(key);
      } else if (key == 'C') {
        lcd.clear();
        lcd.print("Restarting...");
        delay(1000);
        ESP.restart();
      }
    }
  }
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
  int timer = 5;
  while (timer > 0) {
    lcd.setCursor(0, 1);
    lcd.print(timer);
    delay(1000);
    timer--;
  }

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
    lcd.clear();
    lcd.print("Connecting MQTT.");
    delay(1000);
    lcd.clear();
    lcd.print("Connecting MQTT");
    delay(1000);
    lcd.clear();
  }
  client.subscribe(("table/" + String(table_id) + "/command").c_str());  // Subscribe after reconnect
  Serial.println("Inside reconnect mqtt loop...");
  if (millis() - last_status_sent > status_interval) {
    send_status("online");
    last_status_sent = millis();
  }
}

void send_info() {
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  String payload = String("{\"table_id\":") + table_id + ",\"job_id\":\"" + job_id + "\",\"process_id\":\"" + process_id + "\",\"product_count\":\"" + product_count + "\",\"weight\":" + measured_weight + ",\"job_status\":" + job_status + ",\"remarks\":\"" + remarks + "\"}";

  client.publish(("table/" + String(table_id) + "/data").c_str(), payload.c_str());
  Serial.println("Published: " + payload);
}

void resetJob() {
  job_id = "";
  process_id = "";
  measured_weight = 0;
  product_count = 0;
  last_sent = 0;
  job_status = 0;
}

char waitForKey(int timeout_ms) {
  unsigned long start = millis();
  while (millis() - start < timeout_ms) {
    char key = keypad.getKey();
    if (key) return key;
  }
  return 0;
}

void checkConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.print("WiFi Lost...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      lcd.print(".");
      attempts++;
    }

    lcd.clear();
    if (WiFi.status() == WL_CONNECTED) {
      lcd.print("WiFi Restored");
      delay(1000);
      client.setServer(mqtt_server, mqtt_port);
      reconnect_mqtt();
    } else {
      lcd.print("WiFi Fail");
      delay(1000);
    }
  }

  if (!client.connected()) {
    lcd.clear();
    lcd.print("MQTT Lost...");
    reconnect_mqtt();
    lcd.clear();
    lcd.print("MQTT Restored");
    delay(1000);
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  Serial.println("MQTT Command Received: " + msg);

  if (msg.indexOf("msg") != -1) {
    int startIdx = msg.indexOf(":") + 2;
    int endIdx = msg.lastIndexOf("\"");
    String weightStr = msg.substring(startIdx, endIdx);

    if (weightStr == "Ineligible weight") {
      if (!job_registered) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unregistered");
        lcd.setCursor(0, 1);
        lcd.print("Process");
        delay(2000);
        lcd.clear();
        resetJob();
        job_registration();
      }
    } else {
      unit_weight = weightStr.toFloat();
      Serial.print("Received unit weight: ");
      Serial.println(unit_weight);
      if (unit_weight <= 0) {
        Serial.println("❌ Invalid unit weight received. Aborting job.");
        lcd.clear();
        lcd.print("Invalid Weight");
        delay(2000);
        resetJob();
        job_registration();
        return;
      }
    }
  }
}
void send_status(String state) {
  if (!client.connected()) {
    Serial.println("❌ MQTT not connected. Skipping status send.");
    return;
  }
  client.loop();

  String statusPayload = "{\"status\":\"" + state + "\"}";
  client.publish(("table/" + String(table_id) + "/status").c_str(), statusPayload.c_str());
  Serial.println("📡 Status Sent: " + statusPayload);
}