import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

token = "EYUysmweJ1sRx_pXLGSESuwXBjTa-caTfJ6Omfs3LjRiaW0iXN2inC11SPPEA63xhTb0o38A5Y-HFDDLu67-IQ=="
org = "yourorg"
url = "http://172.20.10.4:8080"

client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)

bucket="AirsenseDB"

start = "1970-01-01T00:00:00Z"
stop = "2026-01-01T00:00:00Z"
delete_api = client.delete_api()

'''
delete_api.delete(start, stop, 'SensorType="Temperature"', bucket=bucket, org=org)
delete_api.delete(start, stop, 'SensorType="Humidity"', bucket=bucket, org=org)
delete_api.delete(start, stop, 'SensorType="CO2"', bucket=bucket, org=org)
delete_api.delete(start, stop, 'SensorType="PM2.5"', bucket=bucket, org=org)
delete_api.delete(start, stop, 'SensorType="PM10"', bucket=bucket, org=org)
'''

delete_api.delete(start, stop, 'Location="Anf IV"', bucket=bucket, org=org)

