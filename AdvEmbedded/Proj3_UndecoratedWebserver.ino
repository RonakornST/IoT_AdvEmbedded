#include <WiFi.h>
#include <Wire.h>
#include <time.h>
#include "Adafruit_SHT4x.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>


// setup wifi
const char* ssid      = "Ronakorn_2.4G";      //ต้องแก้ไข
const char* password  = "";      //ต้องแก้ไข

// Set your static IP address and gateway
IPAddress local_IP(192, 168, 0, 99); // Static IP

WiFiServer server(80);   // Web server on port 80

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600*7;  // Your timezone offset in seconds thailand gmt +7hours = 3600*7 seconds

// Daylight saving time offset in seconds is the practice of advancing clocks (typically by one hour) 
//during warmer months so that darkness falls at a later clock time.

//Thailand currently observes Indochina Time (ICT) all year. Daylight Saving Time has never been used here.
// Clocks do not change in Thailand. There is no previous DST change in Thailand.

const int daylightOffset_sec = 0; // 1 hour = 3600 sec

// Global variables for time
struct tm timeinfo;

Adafruit_SHT4x sht4x = Adafruit_SHT4x();
Adafruit_BMP280 bmp280;
Adafruit_MPU6050 mpu;


// Global variables to store last read values
float temp_sht4x, humid_sht4x, temp_bmp280, pressure_bmp280;
float accX, accY, accZ, gyroX, gyroY, gyroZ, temp_mpu6050;

QueueHandle_t sensorDataQueue;


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  // Configure the Wi-Fi connection
  if (!WiFi.config(local_IP)) {
      Serial.println(F("STA Failed to configure"));
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
}

void displayLocalTime(void *parameter) {
  for (;;) {

    if (getLocalTime(&timeinfo)) {
      Serial.print("Local Time: ");
      Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
      Serial.println("Failed to obtain time");
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // delay for 1 second
  }
}

void updateNTPTime(void *parameter) {
  for (;;) {

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    if (getLocalTime(&timeinfo)) {
      Serial.println("Time updated with NTP server");
    } else {
      Serial.println("Failed to update time with NTP server");
    }
    vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 1 minute
  }
}

void readSHT4X(void *parameter){
  for (;;) {

    sensors_event_t humidity, temp;
    sht4x.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
    temp_sht4x = temp.temperature;
    humid_sht4x = humidity.relative_humidity;

    char json_body[100];
    const char json_tmpl[] = "{\"temp_sht4x\": %.2f,"
                             "\"humid_sht4x\": %.2f}";
    sprintf(json_body, json_tmpl, temp_sht4x, humid_sht4x);
    Serial.println(json_body);

    vTaskDelay(pdMS_TO_TICKS(10000)); // Read sensor data every 10 second
  }
}

void readBMP280(void *parameter){
  for (;;) {

    temp_bmp280 = bmp280.readTemperature();
    pressure_bmp280 = bmp280.readPressure() / 100.0F; // Convert Pa to hPa
    char json_body[100];
    const char json_tmpl[] = "{\"temp_bmp280\": %.2f," 
                             "\"pressure_bmp280\": %.2f}";
    sprintf(json_body, json_tmpl, temp_bmp280, pressure_bmp280);
    Serial.println(json_body);
    vTaskDelay(pdMS_TO_TICKS(10000)); // Read sensor data every 10 second
  }
}

void readMPU6050(void *parameter){
  for (;;) {

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    Serial.print("MPU6050 sensor: ");
    accX = a.acceleration.x;
    accY = a.acceleration.y;
    accZ = a.acceleration.z;
    gyroX = g.gyro.x;
    gyroY = g.gyro.y;
    gyroZ = g.gyro.z;
    temp_mpu6050 = temp.temperature;

    char json_body[800];
    const char json_tmpl[] = "{\"accX\": %.2f,"
                             "\"accY\": %.2f,"
                             "\"accZ\": %.2f,"
                             "\"gyroX\": %.2f,"
                             "\"gyroY\": %.2f,"
                             "\"gyroZ\": %.2f,"
                             "\"temp_mpu6050\": %.2f}";
                     
    // Updated sprintf to include all sensor data in the JSON string
    sprintf(json_body, json_tmpl, 
            accX, accY, accZ, 
            gyroX, gyroY, gyroZ, 
            temp_mpu6050);

    Serial.println(json_body);
    

    vTaskDelay(pdMS_TO_TICKS(10000)); // Read sensor data every 10 second
  }
}

// Web server task
// Web server task
void webServerTask(void *parameter) {
  server.begin();

  for (;;) {
    WiFiClient client = server.available();
    if (client) {
      String request = client.readStringUntil('\r');
      client.flush();

      // Get the current time
      if (getLocalTime(&timeinfo)) {
        char timeString[20];
        sprintf(timeString, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        // Prepare HTML response
        String response = "<!DOCTYPE html><html><body>";
        response += "<h1>Sensor Data</h1>";
        response += "<p>Last Update: " + String(timeString) + "</p>";
        response += "<p>SHT4X Temperature: " + String(temp_sht4x) + " °C</p>";
        response += "<p>SHT4X Humidity: " + String(humid_sht4x) + " %</p>";
        response += "<p>BMP280 Temperature: " + String(temp_bmp280) + " °C</p>";
        response += "<p>BMP280 Pressure: " + String(pressure_bmp280) + " hPa</p>";
        response += "<p>MPU6050 Acceleration: X=" + String(accX) + " m/s², Y=" + String(accY) + " m/s², Z=" + String(accZ) + " m/s²</p>";
        response += "<p>MPU6050 Gyroscope: X=" + String(gyroX) + " rad/s, Y=" + String(gyroY) + " rad/s, Z=" + String(gyroZ) + " rad/s</p>";
        response += "<p>MPU6050 Temperature: " + String(temp_mpu6050) + " °C</p>";
        response += "</body></html>";

        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
        client.print(response);
      } else {
        Serial.println("Failed to obtain time");
      }
      client.stop();
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Check for client every 0.1 second
  }
}


void setup() {
  Serial.begin(115200);

  // set I2C pin for cucumber board SDA > 41  , SCL > 40
  Wire.begin(41,40);

  // setup for sht4x
  if (!sht4x.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }

  // setup for bmp280
  if (!bmp280.begin(0x76)) {
    Serial.println("Failed to initialize BMP280 sensor!");
    while (1);
  }

  bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL,
                     Adafruit_BMP280::SAMPLING_X2,
                     Adafruit_BMP280::SAMPLING_X16,
                     Adafruit_BMP280::FILTER_X16,
                     Adafruit_BMP280::STANDBY_MS_500);

  // setup for mpu6050
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }

  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }


  setup_wifi();

  xTaskCreate(
    displayLocalTime,        // Function to run
    "DisplayTimeTask",      // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter to pass
    1,                       // Task priority
    NULL                    // Task handle
  );

  xTaskCreate(
    updateNTPTime,          // Function to run
    "UpdateNTPTimeTask",    // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter to pass
    2,                       // Task priority (higher priority)
    NULL                    // Task handle
  );

  xTaskCreate(
    readSHT4X,               // Function to read SHT4X sensor
    "ReadSHT4XTask",         // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter to pass
    1,                       // Task priority
    NULL                     // Task handle
  );

  xTaskCreate(
    readBMP280,              // Function to read BMP280 sensor
    "ReadBMP280Task",        // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter to pass
    1,                       // Task priority
    NULL                     // Task handle
  );

  xTaskCreate(
    readMPU6050,             // Function to read MPU6050 sensor
    "ReadMPU6050Task",       // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter to pass
    1,                       // Task priority
    NULL                     // Task handle
  );

    xTaskCreate(
      webServerTask,          // Function to run
      "WebServerTask",        // Task name
      10000,                  // Stack size (bytes)
      NULL,                   // Parameter to pass
      1,                      // Task priority
      NULL                    // Task handle
  );

}

void loop() {
  // Empty because all operations are handled in FreeRTOS tasks
}
