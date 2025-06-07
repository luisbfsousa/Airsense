import asyncio
from bleak import BleakClient

ESP32_ADDRESS = "7C:DF:A1:F2:62:85"
CHARACTERISTIC_UUID = "4cb2e36f-7bc6-4bf1-8590-272ef80dd51b"

notification_count = 0

def notification_handler(sender, data):
    global notification_count  
    notification_count += 1

    text = data.decode('utf-8')

    print(f"\n=== Notification #{notification_count} ===")
    print("================ SENSOR HUB DATA ================")

    pairs = text.split(',')
    for pair in pairs:
        if '=' not in pair:
            continue
        key, value = pair.split('=', 1)
        value = value.strip()

        if key == "H":
            print(f"Humidity: {value}%")
        elif key == "T":
            print(f"Temperature: {value} C")
        elif key == "HOC":
            print(f"HOC Index: {value}")
        elif key == "NOx":
            print(f"NOx Index: {value}")
        elif key == "IVOC":
            print(f"IVOC: {value} ppb")
        elif key == "CO2eq":
            print(f"CO2eq: {value} ppm")
        elif key == "CO2":
            print(f"CO2 Level: {value} ppm")
        elif key.startswith("PM"):
            print(f"{key}: {value} pg/m3")
        else:
            print(f"{key}: {value}")

    print("================================================\n")


async def connect_and_listen():
    while True:
        try:
            print("Attempting connection to ESP32...")
            async with BleakClient(ESP32_ADDRESS, timeout=30.0) as client:
                print("Connected to ESP32!")
                await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                print("Subscribed. Waiting for data... (Press Ctrl+C to stop).")

                while True:
                    await asyncio.sleep(5) 

        except Exception as e:
            print(f"Connection lost, retrying in 5s... Error: {e}")
            await asyncio.sleep(5)

if __name__ == "__main__":
    asyncio.run(connect_and_listen())
