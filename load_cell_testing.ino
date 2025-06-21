#include <HX711.h>

// HX711 Pin Definitions
#define DT 4
#define SCK 5

HX711 scale;

// Calibration factor
float calibration_factor = -64.44;  // Put your calibration factor here after calibration

void setup() {
  Serial.begin(115200);
  Serial.println("HX711 Weight Sensor Test");

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();

  Serial.println("Tare complete. Ready to read weight...");
}

void loop() {
  float weight = scale.get_units(10);  // average of 10 readings
  Serial.print("Weight: ");
  Serial.print(weight, 2);
  Serial.println(" g");
  delay(500);
}
