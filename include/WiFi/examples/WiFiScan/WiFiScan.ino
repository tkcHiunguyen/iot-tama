#include <HardwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "ESP-32";
const char *password = "esp32tama";
WebServer server(1234);
void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.print("Scanning wifi ");
  // int listNetwork = WiFi.scanNetworks();
  // // while (!listNetwork)
  // // {
  // //   Serial.print('.');
  // //   delay(200);
  // // }
  // if (listNetwork == 0)
  // {
  //   Serial.println("No networks found");
  // }
  // else
  // {
  //   Serial.print(listNetwork);
  //   Serial.println(" networks found");
  //   for (int i = 0; i < listNetwork; ++i)
  //   {
  //     Serial.print("Network name: ");
  //     Serial.println(WiFi.SSID(i));
  //     Serial.print("Signal strength (RSSI): ");
  //     Serial.println(WiFi.RSSI(i));
  //     Serial.println("-----------------------");
  //   }
  // }


  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("SoftAP IP address: ");
  Serial.println(IP);

  // Bắt đầu máy chủ
  server.on("/", HTTP_GET, []()
            { server.send(200, "text/plain", "Hello from ESP32 SoftAP!"); });
  server.begin();
  Serial.println("HTTP server started");
}

// the loop function runs over and over again forever
void loop()
{
  server.handleClient();
}
