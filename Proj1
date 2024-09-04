#include <WiFi.h>
#include <time.h>
#include <FreeRTOS.h>
#include <task.h>

const char *ssid = "Ronakorn_2.4G";
const char *password = "--";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600*7;  // Your timezone offset in seconds thailand gmt +7hours = 3600*7 seconds

// Daylight saving time offset in seconds is the practice of advancing clocks (typically by one hour) 
//during warmer months so that darkness falls at a later clock time.

//Thailand currently observes Indochina Time (ICT) all year. Daylight Saving Time has never been used here.
// Clocks do not change in Thailand. There is no previous DST change in Thailand.
const int daylightOffset_sec = 0; // 1 hour = 3600 sec

// Global variables for time
struct tm timeinfo;

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


void setup() {
  // put your setup code here, to run once:
   Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to :");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("");

  //create task
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

}

void loop() {
  // put your main code here, to run repeatedly:
}
