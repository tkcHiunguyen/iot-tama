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

  private:
      IPAddress host;
      IPAddress gateway;
      IPAddress NMask;
      int port;
      String path;
};

#endif
