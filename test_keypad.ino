#include <Keypad.h>

// Keypad configuration
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

// Define the keymap
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Connect keypad ROW0, ROW1, ROW2 and ROW3 to these ESP32 pins:
byte rowPins[ROWS] = {13, 12, 14, 27}; 

// Connect keypad COL0, COL1, COL2 and COL3 to these ESP32 pins:
byte colPins[COLS] = {26, 25, 33, 32}; 

// Create keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  Serial.println("4x4 Keypad Test Starting...");
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
  }
}