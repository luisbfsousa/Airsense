# Airsense - Data Processing and InfluxDB Setup

## 📌 Project Overview
This part of the **Airsense** project is responsible for:
- Setting up and running an **InfluxDB container** in Docker to store air quality data.
- Receiving **sensor data via Bluetooth Low Energy (BLE)** from an ESP32 device.
- Processing and storing the received data in **InfluxDB**.

## 🚀 Installation & Setup

### 1️⃣ Install System Dependencies
Before running the scripts, install required system dependencies:
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y python3-pip python3-venv bluez docker.io docker-compose
```

### 2️⃣ Set Up the Virtual Environment
Create and activate a Python **virtual environment**:
```bash
python3 -m venv ble_env
source ble_env/bin/activate
```

### 3️⃣ Install Python Dependencies
Ensure all necessary Python packages are installed using:
```bash
pip install -r requirements.txt
```


### 4️⃣ Configuration of the Environment Variables
Instead of storing sensitive information in the scripts, they are stored in a **.env** file
Add the following information to the **.env** file:
```
# InfluxDB authentication
INFLUXDB_TOKEN=your_influxdb_token

# Organization name
INFLUXDB_ORG=yourorg

# URL of the InfluxDB server
INFLUXDB_URL=http://172.20.10.4:8080

# Bucket name where data will be stored
INFLUXDB_BUCKET=AirsenseDB
```
Save and exit.

### 5️⃣ Run the InfluxDB Docker Container
Start the InfluxDB container with:
```bash
docker run -d --name=airsensedb -p 8080:8086 -v airsensedb_data:/var/lib/influxdb2 influxdb:latest
```
Where **airsensedb** is your container name and **influxdb:latest** is the database Image.

### 6️⃣ Start the Data Collection Scripts

#### 📡 Start the BLE Data Translator
This script connects to the ESP32 and reads sensor data over **Bluetooth Low Energy (BLE)**.
```bash
source ble_env/bin/activate
python3 data_translator.py
```

#### 🛢️ Start the Data Processor
This script reads the sensor data and writes it to **InfluxDB**.
```bash
python3 data_processor.py
```

## ✅ How It Works
1. **`data_translator.py`** connects to the ESP32 via BLE and receives sensor data.
2. The received data is saved as a JSON file (`sensor_data.json`).
3. **`data_processor.py`** reads the JSON file, extracts the sensor values, and writes them to InfluxDB.
4. The data is now stored in InfluxDB and can be queried or visualized.

## 🔧 Troubleshooting
### 🔹 Check if InfluxDB is Running
```bash
docker ps
```
If the container is not running, restart it:
```bash
docker start airsensedb
```

### 🔹 Verify BLE Connectivity
Check if the ESP32 is discoverable:
```bash
bluetoothctl scan on
```
If it doesn't show up, reset the ESP32 and try again.

### 🔹 Debug Missing Data
Check if `sensor_data.json` is being updated:
```bash
cat ~/json/sensor_data.json
```
If not, ensure `data_translator.py` is running and receiving data.


---


