# Production Monitoring Device

This project deals with the monitoring of products produced in an Industry. I have prepared this system for an Weapon manufacturing industry. 

The system basically measures the weight of the products produced by the employees further determining the count of the products produced. The data is continuosly sent to the central server wirelessly via the `WiFi` connection to the Intranet setup of the industry.

### Hardware Used

* ESP32
* (rest to be reveled later)

###GPIO pins not to use

* GPIO 36, 39, 34, 35 → ❌ Do not use
* GPIO 0 → risky, avoid if possible
* GPIO 2, 15 → can be used but have boot mode restrictions

###Current Bugs

* Multiple items at once not handled properly, missing some points
* add the count zero tare