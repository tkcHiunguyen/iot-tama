/*this file contain funtions that used to be in main fiole, but no longer use, however useful in the future*/
// int handleGoogleSheet(String status, String dateTimeStart, String dateTimeEnd, unsigned long duration, String MachineID, float averageCurrent, String uuid) {
//       Serial.println("start void handleGoogleSheet");
//       if (WiFi.status() == WL_CONNECTED) {
//             HTTPClient http;
//             http.begin(ggsheet);
//             http.addHeader("Content-Type", "application/json");
//             String jsonPayload =
//                 "{\"dateTimeStart\":\"" + dateTimeStart + "\",\"dateTimeEnd\":\"" +
//                 dateTimeEnd + "\",\"value\":\"" + status + "\",\"duration\":\"" +
//                 calcTimerMillis(duration) + "\",\"machineID\":\"" + MachineID +
//                 "\",\"averageCurrent\": \"" + averageCurrent + "\",\"uuid\": \"" + uuid + "\"}";
//             int httpResponseCode = http.POST(jsonPayload);
//             Serial.println(jsonPayload);
//             if (httpResponseCode > 0) {
//                   String response = http.getString();
//                   Serial.println("post google sheet ok!");
//             } else {
//                   Serial.print("Error on sending POST: ");
//                   Serial.println(httpResponseCode);
//             }
//             http.end();
//             return httpResponseCode;
//             Serial.print("code: ");
//             Serial.println(httpResponseCode);
//       }
//       return 9999;
// }

// void testRTOSTask(void *pvParameters) {
//       while (1) {
//             Serial.println("testcore");
//             vTaskDelay(100 / portTICK_PERIOD_MS);
//       }
//       vTaskDelete(NULL);
// }