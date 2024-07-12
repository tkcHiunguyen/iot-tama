/*
 * pin 1 - not used          |  Micro SD card     |
 * pin 2 - CS (SS)           |                   /
 * pin 3 - DI (MOSI)         |                  |__
 * pin 4 - VDD (3.3V)        |                    |
 * pin 5 - SCK (SCLK)        | 8 7 6 5 4 3 2 1   /
 * pin 6 - VSS (GND)         | ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄  /
 * pin 7 - DO (MISO)         | ▀ ▀ █ ▀ █ ▀ ▀ ▀ |
 * pin 8 - not used          |_________________|
 *                             ║ ║ ║ ║ ║ ║ ║ ║
 *                     ╔═══════╝ ║ ║ ║ ║ ║ ║ ╚═════════╗
 *                     ║         ║ ║ ║ ║ ║ ╚══════╗    ║
 *                     ║   ╔═════╝ ║ ║ ║ ╚═════╗  ║    ║
 * Connections for     ║   ║   ╔═══╩═║═║═══╗   ║  ║    ║
 * full-sized          ║   ║   ║   ╔═╝ ║   ║   ║  ║    ║
 * SD card             ║   ║   ║   ║   ║   ║   ║  ║    ║
 * Pin name         |  -  DO  VSS SCK VDD VSS DI CS    -  |
 * SD pin number    |  8   7   6   5   4   3   2   1   9 /
 *                  |                                  █/
 *                  |__▍___▊___█___█___█___█___█___█___/
 *
 * Note:  The SPI pins can be manually configured by using `SPI.begin(sck, miso, mosi, cs).`
 *        Alternatively, you can change the CS pin and use the other default settings by using `SD.begin(cs)`.
 *
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SPI Pin Name | ESP8266 | ESP32 | ESP32‑S2 | ESP32‑S3 | ESP32‑C3 | ESP32‑C6 | ESP32‑H2 |
 * +==============+=========+=======+==========+==========+==========+==========+==========+
 * | CS (SS)      | GPIO15  | GPIO5 | GPIO34   | GPIO10   | GPIO7    | GPIO18   | GPIO0    |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DI (MOSI)    | GPIO13  | GPIO23| GPIO35   | GPIO11   | GPIO6    | GPIO19   | GPIO25   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DO (MISO)    | GPIO12  | GPIO19| GPIO37   | GPIO13   | GPIO5    | GPIO20   | GPIO11   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SCK (SCLK)   | GPIO14  | GPIO18| GPIO36   | GPIO12   | GPIO4    | GPIO21   | GPIO10   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 *
 * For more info see file README.md in this library or on URL:
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
 */

#include "FS.h"
#include "SD.h"
#include "SPI.h"

/*
Uncomment and set up if you want to use custom pins for the SPI communication
#define REASSIGN_PINS
int sck = -1;
int miso = -1;
int mosi = -1;
int cs = -1;
*/
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <NTPClient.h>
#include <PZEM004Tv30.h>
#include <UUID.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <freeRTOS/FreeRTOS.h>
#include <freeRTOS/semphr.h>
#include <freeRTOS/task.h>
#include <lib_monitor/start.h>
#include <networkConfig.h>
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define MAINCORE 1
#define SUBCORE 0

PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
// connect ggSHEET
const char *ssid = "ESP-32";
const char *password = "esp32tama";
const char *ggsheet =
    "https://script.google.com/macros/s/"
    "AKfycbyQWoQK_i1k4Mfx_vy-KYGwLxmJK9FIXuke5Jt6VJrfUQYpco39sqf_OVTHG8EcUgTY/"
    "exec";

// global value
String global_status PROGMEM = "UNDEFINED";
String global_oldStatus PROGMEM = "UNDEFINED";
float global_current PROGMEM = 0.00;
unsigned long last_status_change_time PROGMEM = 0;
String DateTimeEndStatus PROGMEM = "";
String global_previousStatus PROGMEM = "null";
float global_updatetime PROGMEM;
String globalWifiScanResult PROGMEM;
SemaphoreHandle_t wifiScanSemaphore PROGMEM;
SemaphoreHandle_t wifiConnectSemaphore PROGMEM;
float global_cal_sum_current PROGMEM = 0;
int global_total_current_count PROGMEM = 1;
float global_average_current PROGMEM = 0;

// timer calculate duration on/off
long global_durationTime_sec PROGMEM;
long global_afterTime01 PROGMEM;
long global_afterTime02 PROGMEM;
long global_elapsedTime PROGMEM;

// millis wifi connect
bool global_status_wifi PROGMEM = false;
bool global_scan_wifi_flag PROGMEM = false;

// READ SD CARD
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
File SDCard;
WiFiUDP ntpUDP;
int global_count_wifi;
unsigned long previousMillis = 0; // Thời gian lưu lần cuối LED được nháy

const long utcOffsetInSeconds = 25200;                                   // 7 hours * 3600 seconds/hour, set time zone GMT+7
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000); // get GMT +7
NetworkConfig networkConfig;
SemaphoreHandle_t sendRealTimeSemaphore;
WebServer server(networkConfig.getPort());
int current_index = 0;
DynamicJsonDocument jsonDoc(1024);
struct pzem_data {
      float current;
      String machineSatus;
};
struct DataSendToGGSheet {
      unsigned long duration;
      float averageCurrent;
      String status;
      String dateTimeEnd;
      String dateTimeStart;
      String MachineID;
      String uuid;
};
DataSendToGGSheet currentStatusData PROGMEM;
// DataSendToGGSheet DataSendToGGSheetQueue[700] PROGMEM;
pzem_data pzemSensor() {
      pzem_data data;
      data.current = pzem.current();
      if (data.current > 4.5) {
            data.machineSatus = "L2";
      } else if (data.current <= 4.5 && data.current > 2.5) {
            data.machineSatus = "L1";
      } else if (data.current <= 2.5 && data.current > 0.5) {
            data.machineSatus = "STAND BY";
      } else if (data.current <= 0.5 && data.current >= -1) {
            data.machineSatus = "OFF";
      } else {
            data.machineSatus = "UNDEFINED";
            data.current = 0;
      }

      return data;
}
// void blinkLED(unsigned long interval) {
//       unsigned long currentMillis = millis(); // Lấy thời gian hiện tại
//
//       // Kiểm tra xem đã đến lúc nháy LED chưa
//       if (currentMillis - previousMillis >= interval) {
//             // Lưu thời gian nháy lần cuối
//             previousMillis = currentMillis;
//
//             // Đổi trạng thái của LED
//             ledState = !ledState;
//             digitalWrite(ledPin, ledState);
//       }
// }
void printSDCardInfo() {
      uint64_t totalBytes = SD.totalBytes();
      uint64_t usedBytes = SD.usedBytes();

      Serial.print("Total SD Card Size: ");
      Serial.print(totalBytes / (1024 * 1024)); // Chuyển đổi từ bytes sang MB
      Serial.println(" MB");

      Serial.print("Used SD Card Size: ");
      Serial.print(usedBytes / (1024 * 1024)); // Chuyển đổi từ bytes sang MB
      Serial.println(" MB");

      Serial.print("Free SD Card Size: ");
      Serial.print((totalBytes - usedBytes) / (1024 * 1024)); // Chuyển đổi từ bytes sang MB
      Serial.println(" MB");
}
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
      Serial.printf("Listing directory: %s\n", dirname);

      File root = fs.open(dirname);
      if (!root) {
            Serial.println("Failed to open directory");
            return;
      }
      if (!root.isDirectory()) {
            Serial.println("Not a directory");
            return;
      }

      File file = root.openNextFile();
      while (file) {
            if (file.isDirectory()) {
                  Serial.print("  DIR : ");
                  Serial.println(file.name());
                  if (levels) {
                        listDir(fs, file.path(), levels - 1);
                  }
            } else {
                  Serial.print("  FILE: ");
                  Serial.print(file.name());
                  Serial.print("  SIZE: ");
                  Serial.println(file.size());
            }
            file = root.openNextFile();
      }
}
void createDir(fs::FS &fs, const char *path) {
      Serial.printf("Creating Dir: %s\n", path);
      if (fs.mkdir(path)) {
            Serial.println("Dir created");
      } else {
            Serial.println("mkdir failed");
      }
}
void removeDir(fs::FS &fs, const char *path) {
      Serial.printf("Removing Dir: %s\n", path);
      if (fs.rmdir(path)) {
            Serial.println("Dir removed");
      } else {
            Serial.println("rmdir failed");
      }
}
void readFile(fs::FS &fs, const char *path) {
      Serial.printf("Reading file: %s\n", path);

      File file = fs.open(path);
      if (!file) {
            Serial.println("Failed to open file for reading");
            return;
      }

      Serial.print("Read from file: ");
      while (file.available()) {
            Serial.write(file.read());
      }
      file.close();
}
void writeFile(fs::FS &fs, const char *path, const char *message) {
      Serial.printf("Writing file: %s\n", path);

      File file = fs.open(path, FILE_WRITE);
      if (!file) {
            Serial.println("Failed to open file for writing");
            return;
      }
      if (file.print(message)) {
            Serial.println("File written");
      } else {
            Serial.println("Write failed");
      }
      file.close();
}
void appendFile(fs::FS &fs, const char *path, const char *message) {
      Serial.printf("Appending to file: %s\n", path);

      File file = fs.open(path, FILE_APPEND);
      if (!file) {
            Serial.println("Failed to open file for appending");
            return;
      }
      if (file.print(message)) {
            Serial.println("File appended");
      } else {
            Serial.println("Append failed");
      }
      file.close();
}
void deleteFile(fs::FS &fs, const char *path) {
      Serial.printf("Deleting file: %s\n", path);
      if (fs.remove(path)) {
            Serial.println("File deleted");
      } else {
            Serial.println("Delete failed");
      }
}
void createFile(fs::FS &fs, const char *path) {
      Serial.printf("Creating file: %s\n", path);

      File file = fs.open(path, FILE_WRITE);
      if (!file) {
            Serial.println("Failed to create file");
            return;
      }
      Serial.println("File created");
      file.close();
}
bool isDuplicate(fs::FS &fs, const char *path, const char *message) {
      File file = fs.open(path, FILE_READ);
      if (!file) {
            // Serial.println("Failed to open file for reading");
            return true;
      }

      String line;
      while (file.available()) {
            line = file.readStringUntil('\n');
            if (line == message) {
                  // Serial.println("i have duuplicate");
                  file.close();
                  return true;
            }
      }
      file.close();
      return false;
}
bool checkSDCard() {
      if (!SD.begin()) {
            return false;
      }
      return true;
}
String mergeHTML_CSS(String title, String bodyContent, String cssContent) {
      String mergeContent = R"=====(
    <!DOCTYPE html>
    <html lang="en">

    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>{{ title }}</title>
        <style>{{ css_content }}</style>
    </head>

    <body>
        {{ body_content }}
    </body>

    </html>
  )=====";

      int cssPos = mergeContent.indexOf("{{ css_content }}");
      if (cssPos != -1) {
            mergeContent.replace("{{ css_content }}", cssContent);
      }
      //----- ---------------------------------------------------
      int bodyPos = mergeContent.indexOf("{{ body_content }}");
      if (bodyPos != -1) {
            mergeContent.replace("{{ body_content }}", bodyContent);
      }
      //--------------------------------------------------------
      int titlePos = mergeContent.indexOf("{{ title }}");
      if (titlePos != -1) {
            mergeContent.replace("{{ title }}", title);
      }
      return mergeContent;
}
// Hàm chuyển chuỗi thời gian thành time_t (epoch time)
String calcTimerMillis(long millis) {
      // Tính toán số giờ, phút và giây
      long seconds = millis / 1000; // Chuyển đổi milliseconds thành giây
      int hours = seconds / 3600;   // Tính số giờ
      seconds %= 3600;              // Còn lại số giây sau khi trừ số giờ
      int minutes = seconds / 60;   // Tính số phút
      seconds %= 60;                // Còn lại số giây sau khi trừ số phút

      // Định dạng chuỗi thời gian
      char formattedTime[9];
      sprintf(formattedTime, "%02d:%02d:%02d", hours, minutes, seconds);

      return String(formattedTime);
}
String getTimeString() {
      if (global_status_wifi) {
            timeClient.update();
            time_t epochTime = timeClient.getEpochTime();
            // Chuyển đổi adjustedTime thành struct tm
            struct tm *ptm = gmtime((time_t *)&epochTime);

            int day = ptm->tm_mday;
            int month = ptm->tm_mon + 1;
            int year = ptm->tm_year + 1900;
            int hour = ptm->tm_hour;
            int minute = ptm->tm_min;
            int second = ptm->tm_sec;

            char formattedDateTime[20];
            sprintf(formattedDateTime, "%02d/%02d/%04d %02d:%02d:%02d", month, day,
                    year, hour, minute, second);

            return String(formattedDateTime);
      }
      return "null";
}
String getTimeString(long millis) {
      if (global_status_wifi) {
            timeClient.update();
            time_t epochTime = timeClient.getEpochTime();
            // Chuyển đổi adjustedTime thành struct tm
            epochTime -= millis / 1000;
            struct tm *ptm = gmtime((time_t *)&epochTime);

            int day = ptm->tm_mday;
            int month = ptm->tm_mon + 1;
            int year = ptm->tm_year + 1900;
            int hour = ptm->tm_hour;
            int minute = ptm->tm_min;
            int second = ptm->tm_sec;

            char formattedDateTime[20];
            sprintf(formattedDateTime, "%02d/%02d/%04d %02d:%02d:%02d", month, day,
                    year, hour, minute, second);

            return String(formattedDateTime);
      }
      return "null";
}
int handleGoogleSheet(String status, String dateTimeStart, String dateTimeEnd, unsigned long duration, String MachineID, float averageCurrent, String uuid) {
      Serial.println("start void handleGoogleSheet");
      if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(ggsheet);
            http.addHeader("Content-Type", "application/json");
            // Serial.println(global_elapsedTime);
            String jsonPayload =
                "{\"dateTimeStart\":\"" + dateTimeStart + "\",\"dateTimeEnd\":\"" +
                dateTimeEnd + "\",\"value\":\"" + status + "\",\"duration\":\"" +
                calcTimerMillis(duration) + "\",\"machineID\":\"" + MachineID +
                "\",\"averageCurrent\": \"" + averageCurrent + "\",\"uuid\": \"" + uuid + "\"}";
            int httpResponseCode = http.POST(jsonPayload);
            Serial.println(jsonPayload);
            if (httpResponseCode > 0) {
                  String response = http.getString();
                  Serial.println("post google sheet ok!");
            } else {
                  Serial.print("Error on sending POST: ");
                  Serial.println(httpResponseCode);
            }
            http.end();
            return httpResponseCode;
            Serial.print("code: ");
            Serial.println(httpResponseCode);
      }
      return 9999;
}
void startPage() {
      String htmlContent = HTML_CONTENT;
      String cssContent = CSS_CONTENT;

      // Gửi nội dung HTML và CSS đến máy khách
      // server.setContentLength(htmlContent.length());
      const String start_content =
          mergeHTML_CSS("Welcome!!", htmlContent, cssContent);
      server.setContentLength(start_content.length());
      server.send(200, "text/html", start_content);
}
// void scanPageTask(void *pvParameters) {
//
//       Serial.print("Scanning wifi ");
//       if (xSemaphoreTake(wifiScanSemaphore, portMAX_DELAY) == pdTRUE) {
//             DynamicJsonDocument doc(2048); // Adjust size as needed
//             JsonArray networks = doc.createNestedArray("networks");
//             int listNetwork = WiFi.scanNetworks();
//             if (listNetwork == 0) {
//                   Serial.println("No networks found");
//             } else {
//                   // Create a dynamic JSON document for efficient memory usage
//
//                   for (int i = 0; i < listNetwork; i++) {
//                         // Create a nested object for each network
//                         JsonObject network = networks.createNestedObject();
//
//                         // Add key-value pairs with appropriate data types
//                         network["name"] = WiFi.SSID(i);
//                         network["rssi"] = WiFi.RSSI(i);
//
//                         Serial.print("Network name: ");
//                         Serial.println(WiFi.SSID(i));
//                         Serial.print("Signal strength (RSSI): ");
//                         Serial.println(WiFi.RSSI(i));
//                         Serial.println("-----------------------");
//                   }
//
//                   // Serialize JSON object to a string (consider error handling)
//                   String jsonString;
//                   if (serializeJson(doc, jsonString) > 0) {
//                         globalWifiScanResult = jsonString;
//                         global_count_wifi = listNetwork;
//                         global_scan_wifi_flag = true;
//                   } else {
//                         Serial.println("Error serializing JSON");
//                   }
//             }
//             WiFi.scanDelete();
//             xSemaphoreGive(wifiScanSemaphore);
//             vTaskDelete(NULL);
//       }
// }
void scanPageTask() {
      if (xSemaphoreTake(wifiScanSemaphore, portMAX_DELAY) == pdTRUE) {
            Serial.print("Scanning wifi ");
            DynamicJsonDocument doc(2048); // Adjust size as needed
            JsonArray networks = doc.createNestedArray("networks");
            int listNetwork = WiFi.scanNetworks();
            if (listNetwork == 0) {
                  Serial.println("No networks found");
            } else {
                  // Create a dynamic JSON document for efficient memory usage

                  for (int i = 0; i < listNetwork; i++) {
                        // Create a nested object for each network
                        JsonObject network = networks.createNestedObject();

                        // Add key-value pairs with appropriate data types
                        network["name"] = WiFi.SSID(i);
                        network["rssi"] = WiFi.RSSI(i);

                        Serial.print("Network name: ");
                        Serial.println(WiFi.SSID(i));
                        Serial.print("Signal strength (RSSI): ");
                        Serial.println(WiFi.RSSI(i));
                        Serial.println("-----------------------");
                  }

                  //     // Serialize JSON object to a string (consider error handling)
                  String jsonString;
                  if (serializeJson(doc, jsonString) > 0) {
                        globalWifiScanResult = jsonString;
                        global_count_wifi = listNetwork;
                        global_scan_wifi_flag = true;
                  } else {
                        Serial.println("Error serializing JSON");
                  }
            }
            WiFi.scanDelete();
            xSemaphoreGive(wifiScanSemaphore);
      }
}
void scanPage() {
      server.send(200, "application/json", "{\"response_code\": \"ok\"}");
      // xTaskCreatePinnedToCore(scanPageTask, "scanPageTask", 2048, NULL, 2, NULL, SUBCORE);
      scanPageTask();
}
void handleConnect() {
      // server.send(200, "application/json", "{\"connect_wifi\":\"true\",\"ipaddress\":\"" + WiFi.localIP().toString() + "\"}");
      String body = server.arg("plain"); // Lấy dữ liệu từ yêu cầu POST

      // Phân tích JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, body);

      // Lấy tên và mật khẩu WiFi từ dữ liệu JSON
      String name = doc["name"];
      String pass = doc["pass"];

      WiFi.begin(name, pass);

      unsigned long startAttemptTime = millis();
      bool isConnected = false;

      while (WiFi.status() != WL_CONNECTED &
             (millis() - startAttemptTime < 15000)) {
            delay(500);
            Serial.print(".");
      }
      switch (WiFi.status() == WL_CONNECTED) {
      case true:
            global_status_wifi = true;
            Serial.println(WiFi.localIP());
            // jsonString = "{\"connect_wifi\":\"true\",\"ipaddress\":\"" + WiFi.localIP().toString() + "\"}";
            timeClient.begin();
            // server.send(200, "application/json", "{\"connect_wifi\":\"true\",\"ipaddress\":\"" + WiFi.localIP().toString() + "\"}");
            break;
      default:
            WiFi.disconnect();
            global_status_wifi = false;
            Serial.println("stop connect to wifi");
            // jsonString = "{\"connect_wifi\":\"false\",\"ipaddress\":\" false\"}";
            // server.send(200, "application/json", "{\"connect_wifi\":\"false\",\"ipaddress\":\" false\"}");
            break;
      }
      String status_wifi_value;
      if (global_status_wifi == true) {
            timeClient.begin();
            status_wifi_value = "true";
            String result = name + ":" + pass;
            if (!isDuplicate(SD, "/wifi.txt", result.c_str())) {
                  appendFile(SD, "/wifi.txt", "\n");
                  appendFile(SD, "/wifi.txt", result.c_str());
            }
      } else {
            status_wifi_value = "false";
      }
      String jsonString = "{\"connect_wifi\":\"" + status_wifi_value +
                          "\",\"ipaddress\":\"" + WiFi.localIP().toString() +
                          "\"}";
      server.send(200, "application/json", jsonString);
}
String checkSDWifi(int list, fs::FS &fs, const char *path) {
      for (int i = 0; i < list; i++) {
            File file = fs.open(path, FILE_READ);
            if (!file) {
                  // Serial.println("Failed to open file for reading");
                  return "NULL";
            }
            String line;
            while (file.available()) {
                  line = file.readStringUntil('\n');
                  if (line.substring(0, line.indexOf(":")) == WiFi.SSID(i)) { // GET NAME WIFI
                        file.close();
                        // return line.substring(line.indexOf(":") + 1); // RETURN PASS
                        return line;
                  }
            }
            file.close();
      }
      return "NULL";
}
// void DataSendToGGSheetAddToQueue(DataSendToGGSheet Data) {
//       DataSendToGGSheetQueue[current_index] = Data;
//       current_index = (current_index + 1) % 1000;
// }
void sendPzemData() {
      StaticJsonDocument<200> jsonPzem;
      // pzem_data pzem             = pzemSensor();
      // jsonPzem["current"]        = pzem.current;
      // jsonPzem["machine_status"] = pzem.machineSatus;
      jsonPzem["current"] = global_current;
      jsonPzem["machine_status"] = global_status;
      // Convert JSON object to string
      String jsonString;
      serializeJson(jsonPzem, jsonString);
      // Serial.println(jsonString);
      server.send(200, "application/json", jsonString);
}
void checkWifiStatus() {
      StaticJsonDocument<200> jsonWifiStatus;
      jsonWifiStatus["wifiStatus"] = global_status_wifi;
      String jsonString;
      serializeJson(jsonWifiStatus, jsonString);
      server.send(200, "application/json", jsonString);
}
bool checkWifi() {
      return global_status_wifi;
}
void checkReset() {
      Serial.println("RESET ESP");
      createFile(SD, "/data.txt");
      ESP.restart();
}
// void init_esp_mode_and_wifi(void *pvParameters) {
void init_esp_mode_and_wifi() {
      if (xSemaphoreTake(wifiScanSemaphore, portMAX_DELAY) == pdTRUE) {
            // WiFi.begin("Tiem Tra MIN", "xincamon");
            // WiFi.begin("Tama-Guest", "tama12!@");
            // WiFi.begin("Cafe Trung Nguyen", "88888888");
            // WiFi.begin("Galaxy S22 Ultra C5C9", "stsvn2024");
            // WiFi.begin("Tama-Guest", "tama12!@");
            // WiFi.begin("SKYBOT", "skybot@2023");
            // WiFi.begin("Xuan Tung", "Tung1996!1");
            int listWifi = WiFi.scanNetworks();
            String found_wifi = checkSDWifi(listWifi, SD, "/wifi.txt");
            Serial.print("pass found: ");
            ;
            if (found_wifi != "null") {
                  WiFi.begin(found_wifi.substring(0, found_wifi.indexOf(":")), found_wifi.substring(found_wifi.indexOf(":") + 1));
            }
            long start_wifi = millis();
            while (WiFi.status() != WL_CONNECTED & (millis() - start_wifi < 10000)) {
                  // delay(500);
                  // Serial.print(".");
            }
            switch (WiFi.status() == WL_CONNECTED) {
            case true:
                  global_status_wifi = true;
                  Serial.println(WiFi.localIP());
                  break;

            default:
                  WiFi.disconnect();
                  global_status_wifi = false;
                  Serial.println("stop connect to wifi");
                  break;
            }
            WiFi.scanDelete();
            Serial.print("init wifi is run on core ");
            Serial.println(xPortGetCoreID());
            xSemaphoreGive(wifiScanSemaphore);
      }
      // xSemaphoreGive(wifiConnectSemaphore);
      // vTaskDelete(NULL);
}
void getListWifi() {
      if (global_scan_wifi_flag == true) {
            server.send(200, "application/json", globalWifiScanResult);
            global_scan_wifi_flag = false;
      } else {
            server.send(200, "application/json", "{\"scan\":\"no_data\"}");
      }
}
void serverTask(void *pvParameters) {
      // if (xSemaphoreTake(wifiConnectSemaphore, portMAX_DELAY) == pdTRUE) {
      while (true) {
            server.handleClient();
            vTaskDelay(1 / portTICK_PERIOD_MS);
            // xSemaphoreGive(wifiScanSemaphore);
      }
      // }
}
void testRTOSTask(void *pvParameters) {
      while (1) {
            Serial.println("testcore");
            vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      vTaskDelete(NULL);
}
void getNowDateTime() {
      StaticJsonDocument<200> jsonDatetime;
      jsonDatetime["dataTime"] = getTimeString();
      String jsonString;
      serializeJson(jsonDatetime, jsonString);
      server.send(200, "application/json", jsonString);
}
// void readPZEMTask(void *pvParameters) {
//       while (1) {
//             pzem_data pzem1 = pzemSensor();
//             global_current = pzem1.current;
//             global_status = pzem1.machineSatus;
//             global_cal_sum_current += pzem1.current;
//             global_total_current_count++;
//             global_average_current = global_cal_sum_current / global_total_current_count;
//             if (global_oldStatus != global_status) {
//                   if (global_status == "STAND BY") {
//                         uuid = uuidGen.generateUUIDv4();
//                   }
//                   DateTimeEndStatus = getTimeString();
//                   currentStatusData.status = global_oldStatus;
//                   currentStatusData.dateTimeEnd = DateTimeEndStatus;
//                   global_oldStatus = global_status;
//                   last_status_change_time = millis();
//                   global_durationTime_sec = last_status_change_time - global_afterTime02;
//                   global_afterTime02 = last_status_change_time;
//                   currentStatusData.duration = global_durationTime_sec;
//                   currentStatusData.dateTimeStart = getTimeString(global_durationTime_sec);
//                   currentStatusData.averageCurrent = global_average_current;
//                   currentStatusData.MachineID = "T300-15";
//                   currentStatusData.uuid = uuid;
//                   global_average_current = 0;
//                   global_cal_sum_current = 0;
//                   global_total_current_count = 0;
//                   // DataSendToGGSheetAddToQueue(currentStatusData);
//             }
//             Serial.print("current: ");
//             Serial.println(global_current);
//             vTaskDelay(1000 / portTICK_PERIOD_MS);
//       }
//       // vTaskDelete(NULL);
// }
void readPZEMTask() {
      String uuid = "...";
      pzem_data pzem1 = pzemSensor();
      global_current = pzem1.current;
      global_status = pzem1.machineSatus;
      global_cal_sum_current += pzem1.current;
      global_total_current_count++;
      global_average_current = global_cal_sum_current / global_total_current_count;
      if (global_oldStatus != global_status) {
            DateTimeEndStatus = getTimeString();
            currentStatusData.status = global_oldStatus;
            currentStatusData.dateTimeEnd = DateTimeEndStatus;
            global_oldStatus = global_status;
            last_status_change_time = millis();
            global_durationTime_sec = last_status_change_time - global_afterTime02;
            global_afterTime02 = last_status_change_time;
            currentStatusData.duration = global_durationTime_sec;
            currentStatusData.dateTimeStart = getTimeString(global_durationTime_sec);
            currentStatusData.averageCurrent = global_average_current;
            currentStatusData.MachineID = "T300-15";
            currentStatusData.uuid = uuid;
            global_average_current = 0;
            global_cal_sum_current = 0;
            global_total_current_count = 0;
            // DataSendToGGSheetAddToQueue(currentStatusData);
      }
      Serial.print("current: ");
      Serial.println(global_current);
}
// void removeElement(DataSendToGGSheet arr[], int &size, int index) {
//       if (index < 0 || index >= size) {
//             return;
//       }
//       for (int i = index; i < size - 1; i++) {
//             arr[i] = arr[i + 1];
//       }
//       size--;
// }
void removeFirstLine(fs::FS &fs, const char *path) {
      File file = fs.open(path, "r");
      if (!file) {
            Serial.println("Failed to open file for reading");
            return;
      }
      // Skip the first line
      file.readStringUntil('\n');
      // Read the remaining content
      String remainingContent;
      while (file.available()) {
            remainingContent += file.readStringUntil('\n') + "\n";
      }
      file.close();

      // Write back the remaining content
      file = fs.open(path, "w");
      if (!file) {
            Serial.println("Failed to open file for writing");
            return;
      }
      file.print(remainingContent);
      file.close();
}
void sendRealtimeRawData(String code, String dateTimeStart, float current, String MachineID, String uuid, fs::FS &fs, const char *path) {
      String jsonPayload =
          "{\"dateTimeStart\":\"" + dateTimeStart + "\",\"code\":\"" +
          code + "\",\"machineID\":\"" + MachineID +
          "\",\"current\": \"" + current + "\",\"uuid\": \"" + uuid + "\"}\n";
      appendFile(fs, path, jsonPayload.c_str());
      if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(ggsheet);
            http.addHeader("Content-Type", "application/json");
            File file = fs.open(path, FILE_READ);
            if (!file) {
                  Serial.println("Failed to open file for reading");
                  file.close();
                  return;
            }
            String lines[5];
            int count = 0;
            while (file.available() && count < 5) {
                  lines[count] = file.readStringUntil('\n');
                  count++;
            }
            file.close();
            String jsonArray = "[";
            for (int i = 0; i < count; i++) {
                  jsonArray += lines[i];
                  if (i < count - 1) {
                        jsonArray += ",";
                  }
            }
            jsonArray += "]";

            int httpResponseCode = http.POST(jsonArray);
            Serial.println(jsonArray);
            if (httpResponseCode > 199 && httpResponseCode < 400) { // HTTP RESPONSE CODE
                  String response = http.getString();
                  Serial.println(httpResponseCode);
                  // Serial.println(response);
                  Serial.println("post google sheet ok!");
                  for (int i = 0; i < count; i++) {
                        removeFirstLine(fs, path);
                  }
            } else {
                  Serial.print("Error on sending POST: ");
                  Serial.println(httpResponseCode);
            }
            http.end();
      }
}
// void sendRealTimeTask(void *parameter) {
//       if (xSemaphoreTake(sendRealTimeSemaphore, portMAX_DELAY) == pdTRUE) {
//             sendRealtimeRawData("realtime", getTimeString(), global_current, "T300-15", "--");
//             xSemaphoreGive(sendRealTimeSemaphore);
//             vTaskDelete(NULL);
//       }
// }
// void redirectSerialToFS() {
//       // Redirect Serial output to file
//       Serial.flush();             // Wait for any remaining data to be sent
//       Serial.setOutput(&logFile); // Redirect Serial to logFile
// }
void setup() {
      Serial.begin(115200);
      if (!checkSDCard()) {
            Serial.println("SD Card Mount Failed");
            return;
      } else {
            Serial.println("SD Card Mount Successful");
            printSDCardInfo();
      }
      // createFile(SD, "/wifi.txt");
      // createFile(SD, "/data.txt");
      // appendFile(SD, "/wifi.txt", "SKYBOT:skybot@2023\n");
      // appendFile(SD, "/wifi.txt", "Galaxy S22 Ultra C5C9:stsvn2024\n");
      // appendFile(SD, "/wifi.txt", "TAMA-IOT-2.4G-ext:12345678\n");
      listDir(SD, "/", 0);
      WiFi.mode(WIFI_MODE_APSTA);
      IPAddress IP = networkConfig.getHost();
      IPAddress NMask = networkConfig.getNMask();
      IPAddress gateway = networkConfig.getGateway();
      WiFi.softAPConfig(IP, gateway, NMask);
      WiFi.softAP(ssid, password);
      Serial.print("SoftAP IP address: ");
      Serial.println(WiFi.softAPIP());
      wifiScanSemaphore = xSemaphoreCreateBinary();
      wifiConnectSemaphore = xSemaphoreCreateBinary();
      xSemaphoreGive(wifiScanSemaphore);

      // Bắt đầu máy chủ
      server.on("/", HTTP_GET, startPage);
      server.on("/scan", HTTP_GET, scanPage);
      server.on("/pzem", HTTP_GET, sendPzemData);
      server.on("/connect", HTTP_POST, handleConnect);
      server.on("/check_wifi_status", HTTP_GET, checkWifiStatus);
      server.on("/reset", HTTP_GET, checkReset);
      server.on("/getdatetime", HTTP_GET, getNowDateTime);
      server.on("/getlistwifi", HTTP_GET, getListWifi);
      server.begin();
      Serial.println("HTTP server started");
      global_afterTime02 = millis();
      // global_check_current_average = true;
      Serial.print("setup is running core: ");
      Serial.println(xPortGetCoreID());
      xTaskCreatePinnedToCore(serverTask,   // Hàm task
                              "serverTask", // Tên task
                              4096,         // Kích thước stack (byte)
                              NULL,         // Tham số truyền vào task
                              2,            // Độ ưu tiên của task
                              NULL,         // Handle của task
                              SUBCORE       // Chỉ định core 1
      );
      // xTaskCreatePinnedToCore(testRTOSTask, "Test task", 1028, NULL, 1,
      // NULL,
      //                         MAINCORE);
      // xTaskCreatePinnedToCore(readPZEMTask, "ReadPZEMTask", 19200, NULL, 1, NULL, MAINCORE);

      // logFile = SD.open("/log.txt", "a+");
      // if (!logFile) {
      //       Serial.println("Failed to open log file for writing");
      //       return;
      // }

      // // Redirect Serial output to log file
      // redirectSerialToFS();
}
void loop() {
      if (checkWifi() == false) {
            init_esp_mode_and_wifi();
      }
      // readFile(SD, "/data.txt");
      // while (current_index > 1) {
      //       // for (int i; i <= sizeof(DataSendToGGSheetQueue); i++) {
      //       //       Serial.println(DataSendToGGSheetQueue[1].status);
      //       // }
      //       // if (handleGoogleSheet(DataSendToGGSheetQueue[1].status, DataSendToGGSheetQueue[1].dateTimeStart, DataSendToGGSheetQueue[1].dateTimeEnd, DataSendToGGSheetQueue[1].duration,
      //       //                       DataSendToGGSheetQueue[1].MachineID, DataSendToGGSheetQueue[1].averageCurrent, uuid_now) < 400) {
      //       //       removeElement(DataSendToGGSheetQueue, current_index, 1);
      //       // }
      // }
      readPZEMTask();
      sendRealtimeRawData("realtime", getTimeString(), global_current, "T300-15", "--", SD, "/data.txt");
      delay(2000);
}
