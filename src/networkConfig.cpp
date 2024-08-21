#include "NetworkConfig.h"
NetworkConfig::NetworkConfig() {
      host = IPAddress(192, 168, 5, 1); // Cấu hình địa chỉ IP tĩnh của điểm truy cập
      gateway = IPAddress(192, 168, 5, 1);
      NMask = IPAddress(255, 255, 255, 0);
      port = 8008;
      path = "/";
      // socket_host = "iot.skybot.id.vn";
      // socket_host = "192.168.1.56:3000";
      socket_host = "192.168.2.13:3000";
      time_server = "time.google.com";
      timezone_second = 25200; // 7 hours * 3600 seconds/hour, set time zone GMT+7
      ssid = "ESP-32";
      password = "esp32tama";
      socket_port = 3000;
}

IPAddress NetworkConfig::getHost() {
      return host;
}

int NetworkConfig::getPort() {
      return port;
}

String NetworkConfig::getPath() {
      return path;
}

IPAddress NetworkConfig::getGateway() {
      return gateway;
}
IPAddress NetworkConfig::getNMask() {
      return NMask;
}

String NetworkConfig::getSocketHost() {
      return socket_host;
}

const char *NetworkConfig::getTimeServer() {
      return time_server;
}
long NetworkConfig::getUTCTimezoneSecond() {
      return timezone_second;
}
long NetworkConfig::getSocketport() {
      return socket_port;
}
const char *NetworkConfig::getPassword() {
      return password;
}
const char *NetworkConfig::getSsid() {
      return ssid;
}
