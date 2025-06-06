#include "HX711.h"

// HX711 circuit wiring
#define DT 4   // HX711 data pin
#define SCK 5  // HX711 clock pin

HX711 scale;

//DEBUGGER VARIABLES
bool debug_mode = true;
const int table_id = 21;  //TABLE ID IS PRE-SET
const float unit_weight = 150.00; 

//PROGRAM VARIABLES
bool job_status = false;
int job_id = 0;
int process_id = 0;
int prd_cnt = 0;
float m_weight = 0;

// CALIBRATION_FACTOR
float calibration_factor = -7050;

void calibrate() {
  Serial.println("HX711 Calibration");

  scale.begin(DT, SCK);

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

void job_registration(){
  //ASK FOR JOB NUMBER IN DISPLAY
  
  //MAKE THE KEYPAD ACTIVE AND TAKE INPUT OF JOB NUMBER.

  //ASK FOR PROCESS NUMBER IN DISPLAY
  
  //MAKE THE KEYPAD ACTIVE AND TAKE INPUT OF PROCESS NUMBER.

  //IF CONFIRM THEN, MAKE THE job_status TRUE
}

void connect_router(){
  //FUNCTION TO CONNECT TO THE ROUTER WITH CERTAIN SSID AND PASSWORD
}

void send_info(){
  //SEND THE INFORMATION TO THE ROUTER 

  //INFO TO BE SENT: table_id+job_id+process_id+m_weight+prd_cnt
}

void setup() {

  //GREETING MESSAGE: TO BE DISPLAYED ON LCD OF WEIGHT TRAY

  //SERIAL MONITOR INITIATION
  Serial.begin(115200);
  delay(500);

  if (debug_mode) {
    //INITIAL CALIBRATION: ONLY FOR DEBUGGING PURPOSE
    calibrate();
  }
}

void loop() {

  //CALIBRATION
  scale.set_scale(calibration_factor);  // Apply the calibration factor

  //WEIGHT_MEASUREMENT
  Serial.print("Weight: ");
  m_weight = scale.get_units(10), 2;
  Serial.print(m_weight);  // 10 readings averaged to 2 decimal places
  Serial.println(" g");

  //PRODUCT_COUNT
  prd_cnt = (int)(m_weight / unit_weight);
  Serial.print("Product count: ");
  Serial.println(prd_cnt);

  //SEND THE PRODUCT COUNT TO THE SERVER IN EVERY 30 SECs
  delay(1000);
}
