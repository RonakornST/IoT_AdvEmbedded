import socket
import json
import random
import struct
from Crypto.Cipher import AES
from base64 import b64decode

# ESP32 Connection Details
ESP32_IP = "192.168.1.37"
ESP32_PORT = 8266  # Choose an unused port

# DH Parameters (same as ESP32)
P = 23  # Prime modulus
G = 5   # Generator

class SecureSocketCommunication:
    def __init__(self, esp32_ip, esp32_port):
        self.esp32_ip = esp32_ip
        self.esp32_port = esp32_port
        self.private_key = random.randint(1, P - 1)
        self.public_key = pow(G, self.private_key, P)
        self.shared_secret = None

    def perform_dh_key_exchange(self):
        try:
            # Create TCP socket
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.connect((self.esp32_ip, self.esp32_port))
                
                # Send public key
                sock.send(f"PUBLIC_KEY:{self.public_key}".encode())
                
                # Receive ESP32's public key
                esp32_public_key_data = sock.recv(1024).decode()
                esp32_public_key = int(esp32_public_key_data.split(':')[1])
                
                # Compute shared secret
                self.shared_secret = pow(esp32_public_key, self.private_key, P)
                print(f"Shared Secret: {self.shared_secret}")
                
                return True
        except Exception as e:
            print(f"DH Key Exchange Error: {e}")
            return False

    def get_sensor_data(self):
        if not self.shared_secret:
            raise ValueError("Shared secret not established")

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.connect((self.esp32_ip, self.esp32_port))
                
                # Request sensor data
                sock.send(b"GET_SENSOR_DATA")
                
                # Receive encrypted base64 data
                encrypted_base64 = sock.recv(4096).decode().strip()
                print("Encrypted Base64:", encrypted_base64)

                # Derive AES key from shared secret
                key = bytearray(16)
                for i in range(16):
                    key[i] = (self.shared_secret >> (8 * (i % 4))) & 0xFF
                print("Derived AES Key:", key)

                # Static IV (must match ESP32)
                iv = b'1234567890abcdef'

                # Decrypt the data
                encrypted_data = b64decode(encrypted_base64)
                cipher = AES.new(bytes(key), AES.MODE_CBC, iv)
                decrypted_data = cipher.decrypt(encrypted_data)

                # Remove PKCS#7 padding
                padding_value = decrypted_data[-1]
                decrypted_data = decrypted_data[:-padding_value]

                # Decode and parse JSON
                decrypted_text = decrypted_data.decode('utf-8')
                print("Decrypted Text:", decrypted_text)
                
                return json.loads(decrypted_text)

        except Exception as e:
            print(f"Sensor Data Retrieval Error: {e}")
            return None

def main():
    secure_comm = SecureSocketCommunication(ESP32_IP, ESP32_PORT)
    
    # Perform key exchange
    if secure_comm.perform_dh_key_exchange():
        # Retrieve sensor data
        sensor_data = secure_comm.get_sensor_data()
        if sensor_data:
            print("Sensor Data:", json.dumps(sensor_data, indent=2))

if __name__ == "__main__":
    main()
