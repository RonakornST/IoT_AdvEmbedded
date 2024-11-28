#include <WiFi.h>
#include <Wire.h>
#include <Base64.h>
#include "mbedtls/aes.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include "Adafruit_SHT4x.h"

const char* ssid = "WIFI_SSID";
const char* password = "PASSWORD";
const int TCP_PORT = 8266;

WiFiServer server(TCP_PORT);

// Sensor and communication variables (similar to previous implementation)
const uint32_t P = 23;
const uint32_t G = 5;
uint32_t private_key;
uint32_t public_key;
uint32_t shared_secret;

Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

float pressure_bmp, temp_bmp, ax, ay, az, gx, gy, gz, temp_mpu, temp_sht4, humid_sht4;

// Modular exponentiation function
uint32_t mod_exp(uint32_t base, uint32_t exp, uint32_t mod) {
    uint32_t result = 1;
    for (uint32_t i = 0; i < exp; i++) {
        result = (result * base) % mod;
    }
    return result;
}

// Generate DH keys
void generate_dh_keys() {
    private_key = random(1, P - 1);
    public_key = mod_exp(G, private_key, P);
    Serial.printf("Generated Public Key: %d\n", public_key);
}

// Format sensor data to JSON
String format_sensor_data() {
    String jsonResponse = "{\"pressure_bmp\": " + String(pressure_bmp / 100) +
                          ", \"temp_bmp\": " + String(temp_bmp) +
                          ", \"ax\": " + String(ax) +
                          ", \"ay\": " + String(ay) +
                          ", \"az\": " + String(az) +
                          ", \"gx\": " + String(gx) +
                          ", \"gy\": " + String(gy) +
                          ", \"gz\": " + String(gz) +
                          ", \"temp_mpu\": " + String(temp_mpu) +
                          ", \"temp_sht4\": " + String(temp_sht4) +
                          ", \"humid_sht4\": " + String(humid_sht4) + "}";
    return jsonResponse;
}

// Encrypt sensor data
String encrypt_sensor_data(String jsonResponse) {
    uint8_t key[16];
    for (int i = 0; i < 16; i++) {
        key[i] = (shared_secret >> (8 * (i % 4))) & 0xFF;
    }

    uint8_t iv[16] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                      0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66};

    size_t jsonLen = jsonResponse.length();
    size_t paddedLen = ((jsonLen + 15) / 16) * 16;
    uint8_t plainText[paddedLen];
    memset(plainText, 0, paddedLen);
    memcpy(plainText, jsonResponse.c_str(), jsonLen);
    uint8_t paddingValue = paddedLen - jsonLen;
    for (size_t i = jsonLen; i < paddedLen; i++) {
        plainText[i] = paddingValue;
    }

    uint8_t encryptedData[paddedLen];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen, iv, plainText, encryptedData);
    mbedtls_aes_free(&aes);

    int base64Len = Base64.encodedLength(paddedLen);
    char base64Output[base64Len + 1];
    Base64.encode(base64Output, (char*)encryptedData, paddedLen);
    base64Output[base64Len] = '\0';

    return String(base64Output);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(41, 40);

    // WiFi setup
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize server
    server.begin();
    Serial.println("TCP Server started");
    Serial.println(WiFi.localIP());

    // Initialize sensors (similar to previous implementation)
    generate_dh_keys();
    bmp.begin(0x76);
    mpu.begin();
    sht4.begin();
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        Serial.println("Client connected");
        while (client.connected()) {
            if (client.available()) {
                String request = client.readStringUntil('\n');
                request.trim();

                if (request.startsWith("PUBLIC_KEY:")) {
                    // DH Key Exchange
                    uint32_t client_public_key = request.substring(11).toInt();
                    shared_secret = mod_exp(client_public_key, private_key, P);
                    Serial.printf("Shared Secret: %d\n", shared_secret);
                    
                    // Send ESP32's public key back
                    client.println("PUBLIC_KEY:" + String(public_key));
                }
                else if (request == "GET_SENSOR_DATA") {
                    // Update sensor data (this could be in a separate thread in a more complex implementation)
                    sensors_event_t a, g, temp_mpu_event;
                    mpu.getEvent(&a, &g, &temp_mpu_event);
                    ax = a.acceleration.x;
                    ay = a.acceleration.y;
                    az = a.acceleration.z;
                    gx = g.gyro.x;
                    gy = g.gyro.y;
                    gz = g.gyro.z;
                    temp_mpu = temp_mpu_event.temperature;

                    pressure_bmp = bmp.readPressure();
                    temp_bmp = bmp.readTemperature();

                    sensors_event_t humidity, temp_sht4_event;
                    sht4.getEvent(&humidity, &temp_sht4_event);
                    temp_sht4 = temp_sht4_event.temperature;
                    humid_sht4 = humidity.relative_humidity;

                    // Format and encrypt sensor data
                    String jsonData = format_sensor_data();
                    String encryptedData = encrypt_sensor_data(jsonData);
                    
                    // Send encrypted data
                    client.println(encryptedData);
                }
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }
}
