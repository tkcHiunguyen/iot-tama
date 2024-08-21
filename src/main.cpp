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
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoSocketIOClient.h>
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
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define MAINCORE 1
#define SUBCORE 0
#define UUID_LIST_SIZE 500

int global_count_wifi;
float global_current PROGMEM = 0.00;
String global_status PROGMEM = "UNDEFINED";
String globalWifiScanResult PROGMEM;
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
// const char *ssid = "ESP-32";
// const char *password = "esp32tama";
const char *ggsheet =
    "https://script.google.com/macros/s/"
    "AKfycbyQWoQK_i1k4Mfx_vy-KYGwLxmJK9FIXuke5Jt6VJrfUQYpco39sqf_OVTHG8EcUgTY/"
    "exec";

// uint16_t port = 3000;
bool global_status_wifi PROGMEM = false;
bool global_scan_wifi_flag PROGMEM = false;
long global_loopTimeout = 0;
long utcOffsetInSeconds = 25200; // 7 hours * 3600 seconds/hour, set time zone GMT+7
SemaphoreHandle_t wifiScanSemaphore PROGMEM;
ArduinoSocketIOClient socket;
File SDCard;
WiFiUDP ntpUDP;
NetworkConfig networkConfig;
const char host[] = "iot.skybot.id.vn";
// String host = networkConfig.getSocketHost();
long port = networkConfig.getSocketport();
const char *time_server = networkConfig.getTimeServer();
NTPClient timeClient(ntpUDP, time_server, networkConfig.getUTCTimezoneSecond(), 60000); // get GMT +7
SemaphoreHandle_t sendRealTimeSemaphore;
WebServer server(networkConfig.getPort());
UUIDGenerator uuidGen;
String idMachine = "node_1";
String idESP = "01";
String uuidListCheck[UUID_LIST_SIZE];
int countList = 0;
unsigned long global_epochtime = 0;
unsigned long global_millis_point = 0;
String global_local_nowtime = "null";
struct pzem_data {
      float current;
      String machineSatus;
};
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
bool checkSDCard() {
      if (!SD.begin()) {
            return false;
      }
      return true;
}
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
      if (checkSDCard()) {
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
      if (checkSDCard) {
            // Serial.printf("Appending to file: %s\n", path);

            File file = fs.open(path, FILE_APPEND);
            if (!file) {
                  Serial.println("Failed to open file for appending");
                  return;
            }
            if (file.print(message)) {
                  // Serial.println("File appended");
            } else {
                  Serial.println("Append failed");
            }
            file.close();
      }
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
      if (checkSDCard) {
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
void updateLocalTime() {
      unsigned long currentEpochTime = global_epochtime + (millis() - global_millis_point) / 1000;
      struct tm *ptm = gmtime((time_t *)&currentEpochTime);

      int day = ptm->tm_mday;
      int month = ptm->tm_mon + 1;
      int year = ptm->tm_year + 1900;
      int hour = ptm->tm_hour;
      int minute = ptm->tm_min;
      int second = ptm->tm_sec;

      char formattedDateTime[20];
      sprintf(formattedDateTime, "%02d/%02d/%04d %02d:%02d:%02d", month, day, year, hour, minute, second);

      global_local_nowtime = String(formattedDateTime);
}
void syncTimeLocal() {
      if (global_status_wifi) {
            timeClient.update();
            time_t epochTime = timeClient.getEpochTime();
            global_epochtime = epochTime;
            global_millis_point = millis();
            updateLocalTime();
      }
}
String getCurrentTimeString() {
      if (global_status_wifi) {
            syncTimeLocal();
      } else {
            updateLocalTime();
      }
      return global_local_nowtime;
}
String checkSDWifi(int list, fs::FS &fs, const char *path) {
      if (checkSDCard()) {
            for (int i = 0; i < list; i++) {
                  File file = fs.open(path, FILE_READ);
                  if (!file) {
                        // Serial.println("Failed to open file for reading");
                        ESP.restart();
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
      }
      return "NULL";
}
void startPage() {
      String htmlContent = HTML_CONTENT;
      String cssContent = CSS_CONTENT;
      const String start_content = mergeHTML_CSS("Welcome!!", htmlContent, cssContent);
      server.setContentLength(start_content.length());
      server.send(200, "text/html", start_content);
}
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
void sendPzemData() {
      StaticJsonDocument<200> jsonPzem;
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
      return WiFi.status() == WL_CONNECTED;
}
void checkReset() {
      Serial.println("RESET ESP");
      // createFile(SD, "/data.txt");
      ESP.restart();
}
void getDeleteData() {
      StaticJsonDocument<200> jsonPzem;
      if ((checkSDCard)) {
            createFile(SD, "/data.txt");
            jsonPzem["data"] = "cleared file data.txt ok";

      } else {
            jsonPzem["data"] = "Fail to clear file data.txt ";
      }
      String jsonString;
      serializeJson(jsonPzem, jsonString);
      server.send(200, "application/json", jsonString);
}
void getOpendata() {
      if (!SD.begin()) {
            server.send(500, "application/json", "{\"error\":\"Failed to initialize SD card\"}");
            return;
      }

      File file = SD.open("/data.txt");
      if (!file) {
            server.send(404, "application/json", "{\"error\":\"File not found\"}");
            return;
      }

      server.streamFile(file, "application/octet-stream");
      file.close();
}
void getOpenwifi() {
      if (!SD.begin()) {
            server.send(500, "application/json", "{\"error\":\"Failed to initialize SD card\"}");
            return;
      }

      File file = SD.open("/wifi.txt");
      if (!file) {
            server.send(404, "application/json", "{\"error\":\"File not found\"}");
            return;
      }

      StaticJsonDocument<1024> jsonDoc;
      String fileContent = "";

      while (file.available()) {
            fileContent += char(file.read());
      }
      file.close();

      jsonDoc["data"] = fileContent;
      String jsonString;
      serializeJson(jsonDoc, jsonString);
      server.send(200, "application/json", jsonString);
}
void getDeletewifi() {
      StaticJsonDocument<200> jsonPzem;
      if ((checkSDCard)) {
            createFile(SD, "/wifi.txt");
            jsonPzem["data"] = "cleared file data.txt ok";

      } else {
            jsonPzem["data"] = "Fail to clear file data.txt ";
      }
      String jsonString;
      serializeJson(jsonPzem, jsonString);
      server.send(200, "application/json", jsonString);
}
void getCheckSD() {
      StaticJsonDocument<200> jsonPzem;
      if ((checkSDCard)) {
            jsonPzem["data"] = "true";

      } else {
            jsonPzem["data"] = "false";
      }
      String jsonString;
      serializeJson(jsonPzem, jsonString);
      server.send(200, "application/json", jsonString);
}
void add_uuid_queue(const char *payload, size_t length) {
      if (countList < UUID_LIST_SIZE) {
            uuidListCheck[countList] = String(payload); // Thêm UUID vào danh sách
            countList++;                                // Tăng biến đếm
      } else {
            Serial.println("UUID list is full, cannot add new UUID!");
      }
}
void check_list_uuid() {
      if (checkSDCard && countList != 0) {
            File fileread = SD.open("/data.txt", FILE_READ);
            String newContent = "";
            Serial.print("countList: ");
            Serial.println(countList);
            createFile(SD, "/data_temp.txt ");
            File filewrite = SD.open("/data_temp.txt", FILE_WRITE);
            if (!filewrite) {
                  Serial.println("Failed to open file for writing");
                  fileread.close();
                  return;
            }
            while (fileread.available()) {
                  String line = fileread.readStringUntil('\n');
                  bool found = false;

                  // Kiểm tra xem dòng này có chứa bất kỳ UUID nào không
                  for (int i = 0; i < countList; i++) {
                        if (line.indexOf(uuidListCheck[i]) > 0) {
                              found = true;
                              Serial.println(uuidListCheck[i]);
                              break; // Thoát khỏi vòng lặp nếu tìm thấy UUID
                        }
                  }

                  // Nếu không tìm thấy bất kỳ UUID nào, thêm dòng vào nội dung mới
                  if (!found) {
                        newContent += line + "\n";
                        filewrite.println(line);
                  }
            }
            fileread.close();
            filewrite.close();

            // Xóa hết UUID trong danh sách
            for (int i = 0; i < countList; i++) {
                  uuidListCheck[i] = "";
            }
            countList = 0; // Đặt lại số lượng UUID

            // Ghi lại nội dung mới vào file
            // File filewrite = SD.open("/data.txt", FILE_WRITE);
            // filewrite.print(newContent);
            // filewrite.close();
            SD.remove("/data.txt");
            SD.rename("/data_temp.txt", "/data.txt");
            Serial.println("Processed and updated file content.");
      }
}
void onConnect(const char *payload, size_t length) {
      Serial.println("Connected to Socket.IO server");
}
void onDisconnect(const char *payload, size_t length) {
      Serial.println("Disconnected from Socket.IO server");
}
void setupDevice(const char *payload, size_t length) {
      idMachine = payload;
}
void init_esp_mode_and_wifi() {
      if (xSemaphoreTake(wifiScanSemaphore, portMAX_DELAY) == pdTRUE) {
            // WiFi.begin("Cafe Trung Nguyen", "88888888");
            // WiFi.begin("Galaxy S22 Ultra C5C9", "stsvn2024");
            // WiFi.begin("Tama-Guest", "tama12!@");
            // WiFi.begin("SKYBOT", "skybot@2023");
            int listWifi = WiFi.scanNetworks();
            String found_wifi = checkSDWifi(listWifi, SD, "/wifi.txt");
            Serial.print("pass found: ");
            ;
            if (found_wifi != "NULL") {
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
                  syncTimeLocal();
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
      // socket.begin("192.168.2.13", 3000);
      socket.begin(host);
      socket.on("connect", onConnect);
      socket.on("disconnect", onDisconnect);
      socket.on("setup_device", setupDevice);
      socket.on("check_uuid" + idESP, add_uuid_queue);
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
      while (true) {
            server.handleClient();
            vTaskDelay(1 / portTICK_PERIOD_MS);
            // xSemaphoreGive(wifiScanSemaphore);
      }
}
void getNowDateTime() {
      StaticJsonDocument<200> jsonDatetime;
      jsonDatetime["dataTime"] = getTimeString();
      String jsonString;
      serializeJson(jsonDatetime, jsonString);
      server.send(200, "application/json", jsonString);
}
String readPZEMTask() {
      pzem_data pzem1 = pzemSensor();
      global_current = pzem1.current;
      global_status = pzem1.machineSatus;
      String clienttime = getCurrentTimeString();
      // Serial.print("current: ");
      // Serial.println(global_current);
      String uuid = uuidGen.generateUUIDv4();
      String jsonPayload =
          "{\"device\": \"" + idESP + "\",\"data\":{\"code\":\"realtime\",\"machineID\":\"" + idMachine +
          "\",\"current\": \"" + global_current + "\",\"uuid\":\"" + uuid + "\",\"clienttime\":\"" + clienttime + "\",\"status\": \"" + global_status + "\"}}\n";
      // socket.emit("device_info", idMachine);
      return jsonPayload;
}
void resendDataInSDCard() {
      if (checkSDCard()) {
            File fileread = SD.open("/data.txt", FILE_READ);
            int count = 0;
            while (fileread.available() && count < 30) {
                  String line = fileread.readStringUntil('\n');
                  socket.emit("readpzem", line);
                  count++;
            }
            fileread.close();
      }
}
int checkLineInFile() {
      if (checkSDCard()) {
            File fileread = SD.open("/data.txt", FILE_READ);
            int count = 0;
            while (fileread.available()) {
                  String line = fileread.readStringUntil('\n');
                  count++;
            }
            fileread.close();
            return count;
      }
      return 0;
}
void sendRealtimeRawData(String data, fs::FS &fs, const char *path) {
      if (!checkSDCard()) {
            socket.emit("readpzem", data);
            return;
      } else {
            appendFile(fs, path, data.c_str());
      }
}
void setup() {
      Serial.begin(115200);
      if (!checkSDCard()) {
            Serial.println("SD Card Mount Failed");
      } else {
            Serial.println("SD Card Mount Successful");
            printSDCardInfo();
            listDir(SD, "/", 0);
      }
      // createFile(SD, "/wifi.txt");
      // createFile(SD, "/data.txt");
      // appendFile(SD, "/wifi.txt", "SKYBOT:skybot@2023\n");
      // appendFile(SD, "/wifi.txt", "Galaxy S22 Ultra C5C9:stsvn2024\n");
      // appendFile(SD, "/wifi.txt", "TAMA-IOT-2.4G-ext:12345678\n");
      WiFi.mode(WIFI_MODE_APSTA);
      IPAddress IP = networkConfig.getHost();
      IPAddress NMask = networkConfig.getNMask();
      IPAddress gateway = networkConfig.getGateway();
      const char *ssid = networkConfig.getSsid();
      const char *password = networkConfig.getPassword();
      WiFi.softAPConfig(IP, gateway, NMask);
      WiFi.softAP(ssid, password);
      Serial.print("SoftAP IP address: ");
      Serial.println(WiFi.softAPIP());
      wifiScanSemaphore = xSemaphoreCreateBinary();
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
      server.on("/delete_data", HTTP_GET, getDeleteData);
      server.on("/checkSD", HTTP_GET, getCheckSD);
      server.on("/opendata", HTTP_GET, getOpendata);
      server.on("/openwifi", HTTP_GET, getOpenwifi);
      server.on("/deletewifi", HTTP_GET, getDeletewifi);
      server.begin();
      Serial.println("HTTP server started");
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
      init_esp_mode_and_wifi();
      // createFile(SD, "/data.txt");
      // readFile(SD, "/data.txt");
}
void loop() {
      if (!checkSDCard()) {
            Serial.println("NO SD CARD");
            // ESP.restart();
      }
      if (checkWifi() == false) {
            init_esp_mode_and_wifi();
      }
      if (millis() - global_loopTimeout >= 5000) {
            sendRealtimeRawData(readPZEMTask(), SD, "/data.txt");
            resendDataInSDCard();
            check_list_uuid();
            Serial.print("total line: ");
            Serial.println(checkLineInFile());
            global_loopTimeout = millis();
      }

      socket.loop();
}