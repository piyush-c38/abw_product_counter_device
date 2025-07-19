# Production Monitoring Device

This project deals with the monitoring of products produced in an industry. I have prepared this system for a weapon manufacturing industry. 

The system basically measures the weight of the products produced by the employees, further determining the count of the products produced. The data is continuously sent to the central server wirelessly via the `WiFi` connection to the intranet setup of the industry.

## Hardware Used

* ESP32

### GPIO pins not to use

* GPIO 36, 39, 34, 35 → ❌ Do not use
* GPIO 0 → risky, avoid if possible
* GPIO 2, 15 → can be used but have boot mode restrictions

## Changes required for new network setup

In case of changing the router, the network configurations need to be changed in both the router and the device program.

### WiFi Router Setup

1. Login to the admin portal of the router using the given manual of your router.
2. Using a LAN cable or wirelessly connect the computer (in which the dashboard application is setup) to this router.
3. Navigate to the IP settings in the admin portal and assign static IP `192.168.0.107` to the connected computer.
4. Save the settings and reboot your router.

### Device Code

1. Install the Arduino IDE on your computer.
2. Install the required libraries mentioned in the "Required Libraries" section.
3. Configure the IDE for ESP32 board.
4. Create a new script file in the IDE.
5. Copy the code in the `industry_full_code.ino` file of this repository to the new script file.
6. Connect the IoT device and upload the code to it using the upload button in the IDE.

### Required Libraries

1. **WiFi** - Built-in ESP32 library for WiFi connectivity
2. **HX711** - Library for HX711 load cell amplifier
3. **Keypad** - Library for matrix keypad interface
4. **LiquidCrystal_I2C** - Library for I2C LCD display
5. **PubSubClient** - MQTT client library for ESP32
6. **EEPROM** - Built-in library for EEPROM memory operations
7. **math.h** - Built-in C math library 

## Support

For issues or questions, please refer to the project repository or contact the development team.

---
*Last updated: July 2025*