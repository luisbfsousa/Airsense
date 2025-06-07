import influxdb_client, os, time, json
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from dotenv import load_dotenv
import fcntl

load_dotenv()

token = os.getenv("INFLUXDB_TOKEN")
org = os.getenv("INFLUXDB_ORG")
url = os.getenv("INFLUXDB_URL")
bucket = os.getenv("INFLUXDB_BUCKET")

write_client = InfluxDBClient(url=url, token=token, org=org)
write_api = write_client.write_api(write_options=SYNCHRONOUS)

last_count = None

json_file = os.path.expanduser("~/sensor_data/sensor_data.json")

def read_json(filename):
    try:
        with open(filename, "r") as file:
            fcntl.flock(file, fcntl.LOCK_SH)  
            file.seek(0)
            data = json.load(file)
            fcntl.flock(file, fcntl.LOCK_UN)
        return data
    except Exception as e:
        print(f"Erro ao ler o ficheiro JSON: {e}")
        return None

while True:
    sensor_data = read_json(json_file)

    if sensor_data:

        if last_count == sensor_data["COUNT"]:
            print("Data already sent.")
            time.sleep(5)
            continue

        last_count = sensor_data["COUNT"]

        point_sen55 = (
            Point("AirQuality")
            .tag("Sensor", "SEN55")
            .tag("Location", "Anf IV")
            .field("PM1.0", float(sensor_data["PM1.0"]))
            .field("PM2.5", float(sensor_data["PM2.5"]))
            .field("PM4.0", float(sensor_data["PM4.0"]))
            .field("PM10", float(sensor_data["PM10"]))
            .field("Humidity", float(sensor_data["H"]))
            .field("Temperature", float(sensor_data["T"]))
        )

        point_sgp30 = (
            Point("AirQuality")
            .tag("Sensor", "SGP30")
            .tag("Location", "Anf IV")
            .field("VOC", float(sensor_data["VOC"]))
            .field("NOx", float(sensor_data["NOX"]))
            .field("CO2EQ", float(sensor_data["CO2EQ"]))
            .field("TVOC", float(sensor_data["TVOC"]))
        )

        point_s88 = (
            Point("AirQuality")
            .tag("Sensor", "S88")
            .tag("Location", "Anf IV")
            .field("S88CO2", float(sensor_data["S88CO2"]))
        )

        write_api.write(bucket=bucket, org=org, record=[point_sen55, point_sgp30, point_s88])

        print(f"Enviado para InfluxDB: {sensor_data}")

    else:
        print("Erro ao ler os dados do ficheiro JSON. Tentando novamente...")

    time.sleep(5)  
