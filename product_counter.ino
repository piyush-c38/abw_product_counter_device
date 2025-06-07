//Changes to make: currently the program is sending the message on pressing the key, send it after certain interval.

#include <WiFi.h>
#include <HX711.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>

// WiFi Credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed

// HX711
#define DT 4
#define SCK 5
HX711 scale;
float calibration_factor = -7050;

// Table Settings
const int table_id = 21;
const float unit_weight = 150.0;

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {19, 18, 5, 17};  // Update as per your wiring
byte colPins[COLS] = {16, 4, 0, 2};    // Update as per your wiring
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables
String job_id = "";
String process_id = "";
int product_count = 0;
float measured_weight = 0.0;
bool job_registered = false;
const unsigned long send_interval = 10000; // 10 seconds
unsigned long last_sent = 0;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);

  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    lcd.print(".");
    wifi_attempts++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected!");
    delay(1500);
  } else {
    lcd.print("WiFi Failed!");
    while (1); // halt
  }

  scale.begin(DT, SCK);
  if (calibration_mode) {
    //INITIAL CALIBRATION: ONLY FOR DEBUGGING PURPOSE
    calibrate();
  }
  scale.set_scale(calibration_factor);

  lcd.clear();
  lcd.print("Welcome!");
  delay(1500);
  lcd.clear();

  job_registration();  // Start input
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

  job_registered = true;
  lcd.clear();
  lcd.print("Started Job");
  delay(1000);
}

String getInputFromKeypad() {
  String input = "";
  char key;
  lcd.setCursor(0, 1);
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == '#') break;        // End input on #
      else if (key == '*') {        // Clear input on *
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
      } else if (key >= '0' && key <= '9') {
        input += key;
        lcd.print(key);
      }
    }
  }
  return input;
}

void calibrate() {
  Serial.println("HX711 Calibration");

  Serial.println("Place the scale on a stable surface and remove all weight...");
  delay(2000);

  scale.set_scale();  // Set an initial dummy scale
  scale.tare();       // Reset the scale to 0

  Serial.println("Tare complete. Place a known weight on the scale.");

  // Wait 5 seconds for you to place a known weight
  delay(5000);

  long reading = scale.get_units(10);
  Serial.print("Raw reading with known weight: ");
  Serial.println(reading);

  // Ask user to enter actual weight in grams
  Serial.println("Enter actual weight in grams into the code and adjust calibration_factor.");
}

void loop() {
  if (!job_registered) return;

  measured_weight = scale.get_units(10);
  product_count = (int)(measured_weight / unit_weight);

  // Show weight and count
  lcd.setCursor(0, 0);
  lcd.print("Wt: ");
  lcd.print(measured_weight, 1);
  lcd.print("g     ");

  lcd.setCursor(0, 1);
  lcd.print("Cnt:");
  lcd.print(product_count);
  lcd.print("  ");

  // Send data every send_interval ms
  if (millis() - last_sent >= send_interval) {
    send_info();
    delay(1000);
    last_sent = millis();
  }

  // Optionally allow job reset with 'A' key
  char key = keypad.getKey();
  if (key == 'A') {
    lcd.clear();
    lcd.print("Ending Job...");
    delay(1000);

    job_registered = false;
    job_id = "";
    process_id = "";
    product_count = 0;
    measured_weight = 0;
    last_sent = 0;

    job_registration();  // Start next job
  }

  delay(100);  // Smooth polling
}

void send_info() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://<your-server-ip>/receive";  // Replace with your endpoint
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"table_id\":" + String(table_id) + ",";
    json += "\"job_id\":\"" + job_id + "\",";
    json += "\"process_id\":\"" + process_id + "\",";
    json += "\"weight\":" + String(measured_weight, 2) + ",";
    json += "\"product_count\":" + String(product_count);
    json += "}";

    int httpResponseCode = http.POST(json);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  } else {
    lcd.clear();
    lcd.print("No WiFi Conn!");
    delay(2000);
  }
}