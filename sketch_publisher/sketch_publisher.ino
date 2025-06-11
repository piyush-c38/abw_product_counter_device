#include <WiFi.h>
#include <PubSubClient.h>

// Replace with your WiFi credentials
const char* ssid = "TP-Link_76F6";
const char* password = "abwtplinkpass";

// Replace with your Mosquitto server IP (the PC IP where mosquitto is running)
const char* mqtt_server = "192.168.0.107";  // Replace with your actual PC IP
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Dummy payload
  String payload = "{\"job_id\":\"J333\",\"process_id\":\"P222\",\"table_id\":11,\"weight\":750.5,\"count\":1}";
  client.publish("table/21/data", payload.c_str());
  Serial.println("Published: " + payload);

  delay(1000);  // 10 seconds
}
