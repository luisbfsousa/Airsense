#include <Arduino.h>
#include <Wire.h>
#include "SensirionI2CSen5x.h"
#include "Adafruit_SGP30.h"
#include <NimBLEDevice.h>
#include "RTClib.h"

TwoWire I2C_RTC = TwoWire(2);
RTC_DS3231 rtc;


#define DEVICE_NAME "ESP32_BLE_SENSOR_HUB"
#define SERVICE_UUID        "ec985f67-b58d-4322-89a2-e209f7a326d7"
#define CHARACTERISTIC_UUID "4cb2e36f-7bc6-4bf1-8590-272ef80dd51b"
#define MAX_RETRIES_WITHOUT_CONNECTION 5

static char dataBuffer[256];

TwoWire I2C_SEN55 = TwoWire(0);
SensirionI2CSen5x sen5x;

float g_pm1 = 0, g_pm2_5 = 0, g_pm4 = 0, g_pm10 = 0;
float g_humidity = 0, g_temp = 0, g_vocIndex = 0, g_noxIndex = 0;

TwoWire I2C_SGP30 = TwoWire(1);
Adafruit_SGP30 sgp;
uint16_t g_sgpTVOC = 0, g_sgpCO2eq = 0;
uint32_t sgpBaselineSaveTime = 0;

#define RX_PIN 40
#define TX_PIN 41
int g_s88CO2 = -1;

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
int count = 0;
bool clientConnected = false;
bool wasConnected = false;
int connectionRetries = 0; 

// ==================== BLE Callbacks ====================
class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    Serial.println("Client connected.");
    clientConnected = true;
    wasConnected = true;
    connectionRetries = 0; 
  }
  
  void onDisconnect(NimBLEServer* pServer) {
    Serial.println("Client disconnected, restarting advertising...");
    clientConnected = false;
    pServer->startAdvertising();
  }
};

class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string rxValue(pCharacteristic->getValue().c_str());
    if (!rxValue.empty()) {
      Serial.print("Received via BLE: ");
      Serial.println(rxValue.c_str());
    }
  }
};

void checkConnectionStatus() {
  if (clientConnected) {
    return; 
  }
  
  if (wasConnected) {
    Serial.println("Connection lost. Resetting ESP32...");
    delay(1000); 
    esp_restart();
  }
  
  connectionRetries++;
  if (connectionRetries >= MAX_RETRIES_WITHOUT_CONNECTION) {
    Serial.println("No connection after multiple attempts. Resetting ESP32...");
    delay(1000);
    esp_restart();
  }
}

void setupBLE() {
    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); 

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
    );
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pCharacteristic->setValue("idle");

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    NimBLEAdvertisementData advData;
    advData.setName(DEVICE_NAME);
    advData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
    pAdvertising->setAdvertisementData(advData);

    NimBLEAdvertisementData scanRespData;
    scanRespData.setName(DEVICE_NAME);
    pAdvertising->setScanResponseData(scanRespData);

    pAdvertising->start();
    Serial.println("NimBLE Service & Characteristic started, now advertising...");
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
    g_sgpTVOC = sgp.TVOC; 
    g_sgpCO2eq = sgp.eCO2;
    
    uint16_t TVOC_base, eCO2_base;
    if (sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
        Serial.printf("SGP30 Baseline - eCO2: 0x%04X, TVOC: 0x%04X\n", eCO2_base, TVOC_base);
    }
    
    if ((millis() - sgpBaselineSaveTime) > 3600000) {
        sgpBaselineSaveTime = millis();
        uint16_t baselineTVOC, baselineCO2;
        sgp.getIAQBaseline(&baselineCO2, &baselineTVOC);
        Serial.printf("Saving baseline - eCO2: 0x%04X, TVOC: 0x%04X\n", baselineCO2, baselineTVOC);
    }
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
            g_s88CO2 = (response[3] << 8) | response[4];
        }
    }
}

// ==================== Setup & Loop ====================
void setup() {
    I2C_RTC.begin(17, 18);
    if (!rtc.begin(&I2C_RTC)) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (rtc.lostPower()) {
        Serial.println("RTC lost power, setting the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    DateTime now = rtc.now();
    Serial.printf("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());

    Serial.begin(115200);
    delay(1000);
    Serial.println("=== SENSOR HUB STARTING ===");

    I2C_SEN55.begin(21, 20);
    sen5x.begin(I2C_SEN55);
    uint16_t error = sen5x.startMeasurement();
    if(error) {
      Serial.println("Error: SEN55 failed to startMeasurement");
    }

    I2C_SGP30.begin(7, 15);
    if (!sgp.begin(&I2C_SGP30)) {
        Serial.println("ERROR: SGP30 sensor not found! Check wiring.");
    } else {
        sgp.setIAQBaseline(0x8E68, 0x8F41); 
    }

    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

    setupBLE();
    Serial.println("=== SENSOR HUB READY ===");
}

void loop() {
    checkConnectionStatus();
    
    Serial.println("\n================ SENSOR HUB DATA ================");
    readSEN55();
    readSGP30();
    readS88();
    
    Serial.printf("PM1.0: %.2f, PM2.5: %.2f, PM4.0: %.2f, PM10: %.2f\n", g_pm1, g_pm2_5, g_pm4, g_pm10);
    Serial.printf("Humidity: %.2f, Temperature: %.2f, VOC: %.2f, NOx: %.2f\n",
                g_humidity, g_temp, g_vocIndex, g_noxIndex);
    Serial.printf("TVOC: %d, CO2eq: %d, S88CO2: %d\n", g_sgpTVOC, g_sgpCO2eq, g_s88CO2);

    count++;
    snprintf(
        dataBuffer,
        sizeof(dataBuffer),
        "COUNT=%d,PM1.0=%.2f,PM2.5=%.2f,PM4.0=%.2f,PM10=%.2f,"
        "H=%.2f,T=%.2f,VOC=%.2f,NOx=%.2f,TVOC=%u,CO2eq=%u,S88CO2=%d",
        count,
        g_pm1,
        g_pm2_5,
        g_pm4,
        g_pm10,
        g_humidity,
        g_temp,
        g_vocIndex,
        g_noxIndex,
        g_sgpTVOC,
        g_sgpCO2eq,
        g_s88CO2
    );

    if (pCharacteristic) {
        pCharacteristic->setValue((uint8_t*)dataBuffer, strlen(dataBuffer));
        pCharacteristic->notify();
    
    DateTime now = rtc.now();
    Serial.printf("RTC: %02d:%02d:%02d\n",
                  now.hour(), now.minute(), now.second());
            
    Serial.println("================================================");
}

    delay(5000);
}