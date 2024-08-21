#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>

class NetworkConfig {
  public:
      NetworkConfig();
      IPAddress getHost();
      int getPort();
      String getPath();
      IPAddress getGateway();
      IPAddress getNMask();
      String getSocketHost();
      const char *getTimeServer();
      long getUTCTimezoneSecond();
      const char *getSsid();
      const char *getPassword();
      long getSocketport();

  private:
      IPAddress host;
      IPAddress gateway;
      IPAddress NMask;
      int port;
      String path;
      String socket_host;
      long socket_port;
      const char *time_server;
      long timezone_second;
      const char *ssid;
      const char *password;
};

#endif
