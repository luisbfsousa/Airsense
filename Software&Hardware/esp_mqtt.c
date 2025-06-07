#include <Arduino.h>
#include <Wire.h>
#include "SensirionI2CSen5x.h"
#include "Adafruit_SGP30.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "secret";
const char* password = "secret";
const char* mqtt_server = "secret";
const int mqtt_port = "secret";
const char* mqtt_user = "airsense";
const char* mqtt_password = "airsense";
const char* mqtt_topic = "sensor_data/sensor_data";

TwoWire I2C_SEN55 = TwoWire(0);
SensirionI2CSen5x sen5x;

float g_pm1 = 0, g_pm2_5 = 0, g_pm4 = 0, g_pm10 = 0;
float g_humidity = 0, g_temp = 0, g_vocIndex = 0, g_noxIndex = 0;

TwoWire I2C_SGP30 = TwoWire(1);
Adafruit_SGP30 sgp;
uint16_t g_sgpTVOC = 0, g_sgpCO2eq = 0;

#define RX_PIN 40
#define TX_PIN 41
int g_s88CO2 = -1;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (256)
char msg[MSG_BUFFER_SIZE];
int count = 0;

// ==================== WiFi & MQTT Functions ====================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
}

// ==================== Sensor Reading ====================
void readSEN55() {
    float pm1, pm2_5, pm4, pm10, humidity, temp, vocIndex, noxIndex;
    uint16_t error = sen5x.readMeasuredValues(pm1, pm2_5, pm4, pm10,
                                              humidity, temp, vocIndex, noxIndex);
    if (error) {
        Serial.println("Error reading SEN55 data.");
        return;
    }
    Serial.println("[SEN55] Air Quality Sensor:");
    Serial.printf("PM1.0: %.2f, PM2.5: %.2f, PM4.0: %.2f, PM10: %.2f\n", pm1, pm2_5, pm4, pm10);
    Serial.printf("Humidity: %.2f, Temperature: %.2f, VOC: %.2f, NOx: %.2f\n",
                  humidity, temp, vocIndex, noxIndex);

    g_pm1 = pm1; 
    g_pm2_5 = pm2_5; 
    g_pm4 = pm4; 
    g_pm10 = pm10;
    g_humidity = humidity; 
    g_temp = temp; 
    g_vocIndex = vocIndex; 
    g_noxIndex = noxIndex;
}

void readSGP30() {
    if (!sgp.IAQmeasure()) {
        Serial.println("ERROR: Failed to read SGP30! Check power and I2C.");
        return;
    }
    Serial.println("\n[SGP30] Gas Sensor:");
    Serial.printf("TVOC: %d, CO2eq: %d\n", sgp.TVOC, sgp.eCO2);
    g_sgpTVOC = sgp.TVOC; 
    g_sgpCO2eq = sgp.eCO2;
}

void readS88() {
    byte cmd[] = {0xFE, 0x44, 0x00, 0x08, 0x02, 0x9F, 0x25};
    byte response[7];

    while (Serial2.available()) {
        Serial2.read();
    }
    Serial2.write(cmd, sizeof(cmd));
    delay(150);

    int bytesAvailable = Serial2.available();
    if (bytesAvailable >= 7) {
        Serial2.readBytes(response, 7);
        if (response[0] == 0xFE) {
            int co2 = (response[3] << 8) | response[4];
            Serial.printf("\n[S88] CO2 Level: %d\n", co2);
            g_s88CO2 = co2;
        } else {
            Serial.println("ERROR: Invalid response header from S88.");
        }
    } else {
        Serial.println("ERROR: No valid response from S88.");
    }
}

// ==================== Setup & Loop ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== SENSOR HUB STARTING ===");

    I2C_SEN55.begin(21, 20);
    sen5x.begin(I2C_SEN55);
    uint16_t error = sen5x.startMeasurement();
    if(error) {
      Serial.println("Error: SEN55 failed to startMeasurement");
    } else {
      Serial.println("SEN55 OK");
    }

    I2C_SGP30.begin(7, 15);
    if (!sgp.begin(&I2C_SGP30)) {
        Serial.println("ERROR: SGP30 sensor not found! Check wiring.");
    } else {
        Serial.println("SGP30 OK");
        sgp.setIAQBaseline(0x8973, 0x8AAE);
        sgp.setIAQBaseline(0x89B3, 0x8AAE);
        Serial.println("SGP30 Baseline Reset Done.");
    }

    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("S88 Ready.");

    setup_wifi();
    setupMQTT();
    Serial.println("=== SENSOR HUB READY ===");
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    Serial.println("\n================ SENSOR HUB DATA ================");
    readSEN55();
    readSGP30();
    readS88();
    Serial.println("================================================");

    count++;
    
    StaticJsonDocument<512> doc;
    
    doc["count"] = count;
    doc["pm1"] = g_pm1;
    doc["pm2_5"] = g_pm2_5;
    doc["pm4"] = g_pm4;
    doc["pm10"] = g_pm10;
    doc["humidity"] = g_humidity;
    doc["temperature"] = g_temp;
    doc["voc"] = g_vocIndex;
    doc["nox"] = g_noxIndex;
    doc["tvoc"] = g_sgpTVOC;
    doc["co2eq"] = g_sgpCO2eq;
    doc["s88co2"] = g_s88CO2;
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    
    Serial.println("[MQTT] Publishing data:");
    Serial.println(jsonBuffer);

    client.publish(mqtt_topic, jsonBuffer);

    delay(5000);
}