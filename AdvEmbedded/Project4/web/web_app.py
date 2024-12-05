import random
import json
from flask import Flask, jsonify, render_template
from requests import get, exceptions
from base64 import b64decode
from Crypto.Cipher import AES

app = Flask(__name__)

# Constants
ESP32_IP = "192.168.1.99"
DH_PRIME_MODULUS = 23
DH_GENERATOR = 5

# Generate keys for DH key exchange
_private_key = random.randint(1, DH_PRIME_MODULUS - 1)
_public_key = pow(DH_GENERATOR, _private_key, DH_PRIME_MODULUS)
_shared_secret = None

@app.route("/")
def home():
    """Render the main webpage."""
    return render_template("index.html")

@app.route("/dh-key-exchange")
def dh_key_exchange():
    """Handle the Diffie-Hellman key exchange process."""
    global _shared_secret
    try:
        # Send public key to ESP32 and receive its public key
        response = get(f"http://{ESP32_IP}/dh-key-exchange", params={"public_key": _public_key})
        response.raise_for_status()
        esp32_pub_key = int(response.text.strip())

        # Compute the shared secret
        _shared_secret = pow(esp32_pub_key, _private_key, DH_PRIME_MODULUS)
        print(f"Shared Secret: {_shared_secret}")

        return jsonify({"message": "Key exchange successful", "shared_secret": _shared_secret})
    except exceptions.RequestException as req_err:
        print(f"Request error: {req_err}")
        return jsonify({"error": "Key exchange failed"})
    except Exception as err:
        print(f"Unexpected error: {err}")
        return jsonify({"error": "An unexpected error occurred"})

@app.route("/sensor")
def fetch_sensor_data():
    """Fetch and decrypt sensor data from ESP32."""
    try:
        if _shared_secret is None:
            raise ValueError("Shared secret not established.")

        # Retrieve encrypted sensor data
        response = get(f"http://{ESP32_IP}/sensor")
        response.raise_for_status()
        encrypted_base64 = response.text.strip()

        # Derive AES key from shared secret
        aes_key = bytearray(((_shared_secret >> (8 * (i % 4))) & 0xFF) for i in range(16))
        print(f"AES Key: {aes_key}")

        # Static IV (aligned with ESP32)
        iv = b'1234567890abcdef'

        # Decrypt the data
        encrypted_data = b64decode(encrypted_base64)
        cipher = AES.new(bytes(aes_key), AES.MODE_CBC, iv)
        decrypted_data = cipher.decrypt(encrypted_data)

        # Remove PKCS#7 padding
        padding_len = decrypted_data[-1]
        if not (1 <= padding_len <= 16):
            raise ValueError("Invalid padding detected.")
        decrypted_data = decrypted_data[:-padding_len]

        # Parse JSON from decrypted text
        sensor_data = json.loads(decrypted_data.decode("utf-8"))
        print(f"Sensor Data: {sensor_data}")

        return jsonify(sensor_data)
    except exceptions.RequestException as req_err:
        print(f"Request error: {req_err}")
        return jsonify({"error": "Failed to retrieve sensor data"})
    except (ValueError, json.JSONDecodeError) as parse_err:
        print(f"Parsing error: {parse_err}")
        return jsonify({"error": "Failed to decrypt or parse data"})
    except Exception as err:
        print(f"Unexpected error: {err}")
        return jsonify({"error": "An unexpected error occurred"})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
