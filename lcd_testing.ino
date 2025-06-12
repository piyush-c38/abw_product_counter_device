#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C address may differ (0x27, 0x3F, etc.)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  Serial.println("LCD Test");

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("LCD Working!");
  lcd.setCursor(0, 1);
  lcd.print("Line 2 Test");
}

void loop() {
  // Do nothing here
}