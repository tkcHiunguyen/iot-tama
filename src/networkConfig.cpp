#include "NetworkConfig.h"

NetworkConfig::NetworkConfig()
{
    host = IPAddress(192, 168, 5, 1); // Cấu hình địa chỉ IP tĩnh của điểm truy cập
    gateway = IPAddress(192, 168, 5, 1);
    NMask = IPAddress(255, 255, 255, 0);
    port = 8008;
    path = "/";
}

IPAddress NetworkConfig::getHost()
{
    return host;
}

int NetworkConfig::getPort()
{
    return port;
}

String NetworkConfig::getPath()
{
    return path;
}

IPAddress NetworkConfig::getGateway()
{
    return gateway;
}
IPAddress NetworkConfig::getNMask()
{
    return NMask;
}
