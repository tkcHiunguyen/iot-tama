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
const char *ssid     = "ESP-32";
const char *password = "esp32tama";
const char *ggsheet =
    "https://script.google.com/macros/s/"
    "AKfycbyQWoQK_i1k4Mfx_vy-KYGwLxmJK9FIXuke5Jt6VJrfUQYpco39sqf_OVTHG8EcUgTY/"
    "exec";

// global value
String global_status PROGMEM                  = "UNDEFINED";
String global_oldStatus PROGMEM               = "UNDEFINED";
float global_current PROGMEM                  = 0.00;
unsigned long last_status_change_time PROGMEM = 0;
String DateTimeEndStatus PROGMEM              = "";
String global_previousStatus PROGMEM          = "null";
float global_updatetime PROGMEM;
String globalWifiScanResult PROGMEM;
SemaphoreHandle_t wifiScanSemaphore PROGMEM;
// bool global_check_current_average PROGMEM = false;
float global_cal_sum_current PROGMEM   = 0;
int global_total_current_count PROGMEM = 1;
float global_average_current PROGMEM   = 0;

// timer calculate duration on/off
long global_durationTime_sec PROGMEM;
long global_afterTime01 PROGMEM;
long global_afterTime02 PROGMEM;
long global_elapsedTime PROGMEM;

// millis wifi connect
long global_start_wifi PROGMEM;
bool global_status_wifi PROGMEM;
// set realtime
WiFiUDP ntpUDP;

// uuid
UUIDGenerator uuidGen;
String uuid PROGMEM           = uuidGen.generateUUIDv4();
String uuid_now PROGMEM       = "";
const long utcOffsetInSeconds = 25200;                                    // 7 hours * 3600 seconds/hour, set time zone GMT+7
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);  // get GMT +7
NetworkConfig networkConfig;
WebServer server(networkConfig.getPort());

DynamicJsonDocument jsonDoc(1024);

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
    long seconds = millis / 1000;   // Chuyển đổi milliseconds thành giây
    int hours    = seconds / 3600;  // Tính số giờ
    seconds %= 3600;                // Còn lại số giây sau khi trừ số giờ
    int minutes = seconds / 60;     // Tính số phút
    seconds %= 60;                  // Còn lại số giây sau khi trừ số phút

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

        int day    = ptm->tm_mday;
        int month  = ptm->tm_mon + 1;
        int year   = ptm->tm_year + 1900;
        int hour   = ptm->tm_hour;
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

        int day    = ptm->tm_mday;
        int month  = ptm->tm_mon + 1;
        int year   = ptm->tm_year + 1900;
        int hour   = ptm->tm_hour;
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
    String cssContent  = CSS_CONTENT;

    // Gửi nội dung HTML và CSS đến máy khách
    // server.setContentLength(htmlContent.length());
    const String start_content =
        mergeHTML_CSS("Welcome!!", htmlContent, cssContent);
    server.setContentLength(start_content.length());
    server.send(200, "text/html", start_content);
}
void scanPageTask(void *pvParameters) {
    // Serial.print("Scanning wifi ");
    DynamicJsonDocument doc(2048);  // Adjust size as needed
    JsonArray networks = doc.createNestedArray("networks");
    int listNetwork    = WiFi.scanNetworks();
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
            // server.send(200, "application/json", jsonString);
            globalWifiScanResult = jsonString;
        } else {
            Serial.println("Error serializing JSON");
        }
    }
    WiFi.scanDelete();
    xSemaphoreGive(wifiScanSemaphore);
    vTaskDelete(NULL);
}
void scanPage() {
    xTaskCreatePinnedToCore(scanPageTask, "scanPageTask", 2048, NULL, 1, NULL, SUBCORE);
    if (xSemaphoreTake(wifiScanSemaphore, portMAX_DELAY) == pdTRUE) {
        server.send(200, "application/json", globalWifiScanResult);
    }
}
void handleConnect() {
    String body = server.arg("plain");  // Lấy dữ liệu từ yêu cầu POST

    // Phân tích JSON
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, body);

    // Lấy tên và mật khẩu WiFi từ dữ liệu JSON
    String name = doc["name"];
    String pass = doc["pass"];

    WiFi.begin(name, pass);

    unsigned long startAttemptTime = millis();
    bool isConnected               = false;

    // while (WiFi.status() != WL_CONNECTED) {
    //     if (millis() - startAttemptTime >= 15000) {  // 30 giây timeout
    //         // String jsonString = "{\"wifiStatus\":\"failed\"}";
    //         // server.send(200, "application/json", jsonString);
    //         // Serial.println("connect failed");
    //         // return;
    //     }
    //     delay(500);
    //     Serial.print(".");
    // }

    // // Khi kết nối thành công
    // timeClient.begin();
    // String jsonString = "{\"connect_wifi\":\"success\",\"ipaddress\":\""
    // +
    //                     WiFi.localIP().toString() + "\"}";
    // server.send(200, "application/json", jsonString);

    while (WiFi.status() != WL_CONNECTED &
           (millis() - startAttemptTime < 15000)) {
        delay(500);
        Serial.print(".");
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
    // Khi kết nối thành công

    String status_wifi_value;
    if (global_status_wifi == true) {
        timeClient.begin();
        status_wifi_value = "true";
    } else {
        status_wifi_value = "false";
    }
    String jsonString = "{\"connect_wifi\":\"" + status_wifi_value +
                        "\",\"ipaddress\":\"" + WiFi.localIP().toString() +
                        "\"}";
    server.send(200, "application/json", jsonString);
}
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
DataSendToGGSheet DataSendToGGSheetQueue[2] PROGMEM;
int current_index = 0;
void DataSendToGGSheetAddToQueue(DataSendToGGSheet Data) {
    DataSendToGGSheetQueue[current_index] = Data;
    current_index                         = (current_index + 1) % 1000;
}
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
        data.current      = 0;
    }

    return data;
}
void sendPzemData() {
    StaticJsonDocument<200> jsonPzem;
    // pzem_data pzem             = pzemSensor();
    // jsonPzem["current"]        = pzem.current;
    // jsonPzem["machine_status"] = pzem.machineSatus;
    jsonPzem["current"]        = global_current;
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
void checkReset() {
    Serial.println("RESET ESP");
    ESP.restart();
}
void init_esp_mode_and_wifi(void *pvParameters) {
    global_start_wifi = millis();
    // WiFi.begin("Tiem Tra MIN", "xincamon");
    // WiFi.begin("Tama-Guest", "tama12!@");
    // WiFi.begin("Cafe Trung Nguyen", "88888888");
    WiFi.begin("Galaxy S22 Ultra C5C9", "stsvn2024");
    // WiFi.begin("Tama-Guest", "tama12!@");
    // WiFi.begin("SKYBOT", "skybot@2023");
    while (WiFi.status() != WL_CONNECTED &
           (millis() - global_start_wifi < 10000)) {
        delay(500);
        Serial.print(".");
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
    Serial.print("task is run on core");
    Serial.println(xPortGetCoreID());
    vTaskDelete(NULL);
}
void serverTask(void *pvParameters) {
    while (true) {
        server.handleClient();
        delay(1);  // Tránh việc sử dụng CPU quá mức
    }
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
void readPZEMTask(void *pvParameters) {
    while (1) {
        pzem_data pzem1 = pzemSensor();
        global_current  = pzem1.current;
        global_status   = pzem1.machineSatus;
        global_cal_sum_current += pzem1.current;
        global_total_current_count++;
        global_average_current = global_cal_sum_current / global_total_current_count;
        if (global_oldStatus != global_status) {
            if (global_status == "STAND BY") {
                uuid = uuidGen.generateUUIDv4();
            }
            DateTimeEndStatus                = getTimeString();
            currentStatusData.status         = global_oldStatus;
            currentStatusData.dateTimeEnd    = DateTimeEndStatus;
            global_oldStatus                 = global_status;
            last_status_change_time          = millis();
            global_durationTime_sec          = last_status_change_time - global_afterTime02;
            global_afterTime02               = last_status_change_time;
            currentStatusData.duration       = global_durationTime_sec;
            currentStatusData.dateTimeStart  = getTimeString(global_durationTime_sec);
            currentStatusData.averageCurrent = global_average_current;
            currentStatusData.MachineID      = "T300-15";
            currentStatusData.uuid           = uuid;
            global_average_current           = 0;
            global_cal_sum_current           = 0;
            global_total_current_count       = 0;
            DataSendToGGSheetAddToQueue(currentStatusData);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
void removeElement(DataSendToGGSheet arr[], int &size, int index) {
    if (index < 0 || index >= size) {
        return;
    }
    for (int i = index; i < size - 1; i++) {
        arr[i] = arr[i + 1];
    }
    size--;
}
void sendRealtimeRawData(String code, String dateTimeStart, float current, String MachineID, String uuid) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(ggsheet);
        http.addHeader("Content-Type", "application/json");
        // Serial.println(global_elapsedTime);
        String jsonPayload =
            "{\"dateTimeStart\":\"" + dateTimeStart + "\",\"code\":\"" +
            code + "\",\"machineID\":\"" + MachineID +
            "\",\"current\": \"" + current + "\",\"uuid\": \"" + uuid + "\"}";
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
    }
}
void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_MODE_APSTA);
    IPAddress IP      = networkConfig.getHost();
    IPAddress NMask   = networkConfig.getNMask();
    IPAddress gateway = networkConfig.getGateway();
    WiFi.softAPConfig(IP, gateway, NMask);
    WiFi.softAP(ssid, password);
    Serial.print("SoftAP IP address: ");
    Serial.println(WiFi.softAPIP());
    wifiScanSemaphore = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(init_esp_mode_and_wifi, "init_esp_mode_and_wifi", 2048, NULL, 2, NULL, MAINCORE);
    // Bắt đầu máy chủ
    server.on("/", HTTP_GET, startPage);
    server.on("/scan", HTTP_GET, scanPage);
    server.on("/pzem", HTTP_GET, sendPzemData);
    server.on("/connect", HTTP_POST, handleConnect);
    server.on("/check_wifi_status", HTTP_GET, checkWifiStatus);
    server.on("/reset", HTTP_GET, checkReset);
    server.on("/getdatetime", HTTP_GET, getNowDateTime);
    server.begin();
    Serial.println("HTTP server started");
    global_afterTime02 = millis();
    // global_check_current_average = true;
    Serial.print("setup is running core: ");
    Serial.println(xPortGetCoreID());
    xTaskCreatePinnedToCore(serverTask,    // Hàm task
                            "serverTask",  // Tên task
                            4096,          // Kích thước stack (byte)
                            NULL,          // Tham số truyền vào task
                            2,             // Độ ưu tiên của task
                            NULL,          // Handle của task
                            SUBCORE        // Chỉ định core 1
    );
    // xTaskCreatePinnedToCore(testRTOSTask, "Test task", 1028, NULL, 1,
    // NULL,
    //                         MAINCORE);
    xTaskCreatePinnedToCore(readPZEMTask, "ReadPZEMTask", 19200, NULL, 1, NULL, MAINCORE);
}
// the loop function runs over and over again forever
void loop() {
    while (current_index > 1) {
        for (int i; i <= sizeof(DataSendToGGSheetQueue); i++) {
            Serial.println(DataSendToGGSheetQueue[1].status);
        }
        if (handleGoogleSheet(DataSendToGGSheetQueue[1].status, DataSendToGGSheetQueue[1].dateTimeStart, DataSendToGGSheetQueue[1].dateTimeEnd, DataSendToGGSheetQueue[1].duration,
                              DataSendToGGSheetQueue[1].MachineID, DataSendToGGSheetQueue[1].averageCurrent, uuid_now) < 400) {
            removeElement(DataSendToGGSheetQueue, current_index, 1);
        }
    }
    sendRealtimeRawData("realtime", getTimeString(), global_current, "T300-15", "--");
    delay(100);
}