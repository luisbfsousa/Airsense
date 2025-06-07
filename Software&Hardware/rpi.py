#!/usr/bin/env python3
"""
BLE Gateway com:
- Timestamp em formato data/hora
- Valores S88 divididos por 10,000
- Conexão robusta
"""

import asyncio
import json
import os
import logging
from datetime import datetime
from bleak import BleakClient, BleakScanner
import subprocess

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('ble_gateway.log')
    ]
)
logger = logging.getLogger(__name__)

class BLEController:
    def __init__(self):
        self.DEVICE_NAME = "ESP32_BLE_SENSOR_HUB"
        self.SERVICE_UUID = "ec985f67-b58d-4322-89a2-e209f7a326d7"
        self.CHAR_UUID = "4cb2e36f-7bc6-4bf1-8590-272ef80dd51b"
        self.client = None

        self.data_dir = "sensor_data"
        self.data_file = os.path.join(self.data_dir, "sensor_data.json")
        os.makedirs(self.data_dir, exist_ok=True)

    async def reset_bluetooth(self):
        try:
            logger.warning("Reiniciando interface Bluetooth...")
            subprocess.run(["sudo", "hciconfig", "hci0", "down"], check=True)
            subprocess.run(["sudo", "hciconfig", "hci0", "up"], check=True)
            await asyncio.sleep(2)
        except Exception as e:
            logger.error(f"Erro ao resetar Bluetooth: {e}")

    async def connect(self):
        try:
            logger.info("Procurando dispositivo BLE...")
            device = await BleakScanner.find_device_by_name(
                self.DEVICE_NAME,
                timeout=20.0
            )

            if not device:
                logger.warning("Dispositivo não encontrado")
                return False

            self.client = BleakClient(device)
            await self.client.connect(timeout=20.0)
            await self.client.start_notify(self.CHAR_UUID, self.handle_data)
            
            logger.info(f"Conectado ao dispositivo {device.address}")
            return True

        except Exception as e:
            logger.error(f"Erro de conexão: {str(e)}")
            await self.reset_bluetooth()
            return False

    def parse_sensor_data(self, data_str: str) -> dict:
        try:
            data = {}
            for pair in data_str.split(','):
                key, value = pair.split('=', 1)
                
                if key.upper() == "S88CO2":
                    data[key.upper()] = int(value) / 10000 #para efeitos de visualizacao
                else:
                    data[key.upper()] = float(value) if '.' in value else int(value)
            
            return data
        except Exception as e:
            logger.error(f"Erro ao processar dados: {e}")
            return {}

    async def handle_data(self, sender, data: bytearray):
        """Manipula os dados recebidos"""
        try:
            timestamp = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")
            sensor_data = self.parse_sensor_data(data.decode('utf-8'))
            
            if not sensor_data:
                return

            sensor_data["TIMESTAMP"] = timestamp

            temp_file = f"{self.data_file}.tmp"
            with open(temp_file, 'w') as f:
                json.dump(sensor_data, f, indent=4)
            os.replace(temp_file, self.data_file)

            logger.info(f"Dados recebidos e processados às {timestamp}")

        except Exception as e:
            logger.error(f"Erro no handler: {e}")

    async def run(self):
        logger.info("Iniciando gateway BLE...")
        
        try:
            while True:
                if not self.client or not self.client.is_connected:
                    if not await self.connect():
                        await asyncio.sleep(5)
                        continue
                
                await asyncio.sleep(1)
                
        except KeyboardInterrupt:
            logger.info("Encerrando gateway...")
        finally:
            if self.client and self.client.is_connected:
                await self.client.disconnect()

if __name__ == "__main__":
    controller = BLEController()
    asyncio.run(controller.run())
