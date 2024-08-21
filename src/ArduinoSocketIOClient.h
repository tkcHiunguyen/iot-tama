/**
 * ArduinoSocketIOClient.h
 *
 *  Created on: Nov 04, 2021
 *      Author: nqnghia285@gmail.com
 */

// #pragma once

#ifndef ARDUINOSOCKETIOCLIENT_H_
#define ARDUINOSOCKETIOCLIENT_H_

#include <ArduinoJson.h>
#include <WebSockets.h>
#include <WebSocketsClient.h>
#include <map>
#include <vector>

#define EIO_HEARTBEAT_INTERVAL 20000
#define FACTOR 4

#define EIO_MAX_HEADER_SIZE (WEBSOCKETS_MAX_HEADER_SIZE + 1)
#define SIO_MAX_HEADER_SIZE (EIO_MAX_HEADER_SIZE + 1)
// #define SOCKETIOCLIENT_DEBUG(...) Serial.printf(__VA_ARGS__);
#define SOCKETIOCLIENT_DEBUG(...)
#define DEFAULT_PORT 80
#define DEFAULT_SSL_PORT 443
#define DEFAULT_URL "/socket.io/?EIO=4"
#define DEFAULT_PROTOCOL "arduino"
#define DEFAULT_PATH "/"

typedef enum {
   eIOtype_OPEN = '0',    ///< Sent from the server when a new transport is opened (recheck)
   eIOtype_CLOSE = '1',   ///< Request the close of this transport but does not
                          ///< shutdown the connection itself.
   eIOtype_PING = '2',    ///< Sent by the client. Server should answer with a pong
                          ///< packet containing the same data
   eIOtype_PONG = '3',    ///< Sent by the server to respond to ping packets.
   eIOtype_MESSAGE = '4', ///< actual message, client and server should call
                          ///< their callbacks with the data
   eIOtype_UPGRADE = '5', ///< Before engine.io switches a transport, it tests, if server and
                          ///< client can communicate over this transport. If this test
                          ///< succeed, the client sends an upgrade packets which requests the
                          ///< server to flush its cache on the old transport and switch to
                          ///< the new transport.
   eIOtype_NOOP = '6',    ///< A noop packet. Used primarily to force a poll cycle
                          ///< when an incoming websocket connection is received.
} engineIOmessageType_t;

typedef enum {
   sIOtype_CONNECT = '0',
   sIOtype_DISCONNECT = '1',
   sIOtype_EVENT = '2',
   sIOtype_ACK = '3',
   sIOtype_ERROR = '4',
   sIOtype_BINARY_EVENT = '5',
   sIOtype_BINARY_ACK = '6',
} socketIOmessageType_t;

class ArduinoSocketIOClient : protected WebSocketsClient {
 public:
#ifdef __AVR__
   typedef void (*SocketIOClientEvent)(socketIOmessageType_t type, uint8_t *payload, size_t length);
#else
   typedef std::function<void(socketIOmessageType_t type, uint8_t *payload, size_t length)> SocketIOClientEvent;
#endif

   ArduinoSocketIOClient(void);
   virtual ~ArduinoSocketIOClient(void);

   void begin(const char *host, uint16_t port = DEFAULT_PORT, const char *nsp = DEFAULT_PATH, const char *url = DEFAULT_URL, const char *protocol = DEFAULT_PROTOCOL);
   void begin(String host, uint16_t port = DEFAULT_PORT, String nsp = DEFAULT_PATH, String url = DEFAULT_URL, String protocol = DEFAULT_PROTOCOL);

#ifdef HAS_SSL
   void beginSSL(const char *host, const char *nsp = DEFAULT_PATH, uint16_t port = DEFAULT_SSL_PORT, const char *url = DEFAULT_URL, const char *protocol = DEFAULT_PROTOCOL);
   void beginSSL(String host, String nsp = DEFAULT_PATH, uint16_t port = DEFAULT_SSL_PORT, String url = DEFAULT_URL, String protocol = DEFAULT_PROTOCOL);
#ifndef SSL_AXTLS
   void beginSSLWithCA(const char *host, const char *nsp = DEFAULT_PATH, uint16_t port = DEFAULT_SSL_PORT, const char *url = DEFAULT_URL, const char *CA_cert = NULL, const char *protocol = DEFAULT_PROTOCOL);
   void beginSSLWithCA(const char *host, const char *nsp = DEFAULT_PATH, uint16_t port = DEFAULT_SSL_PORT, const char *url = DEFAULT_URL, BearSSL::X509List *CA_cert = NULL, const char *protocol = DEFAULT_PROTOCOL);
   void setSSLClientCertKey(const char *clientCert = NULL, const char *clientPrivateKey = NULL);
   void setSSLClientCertKey(BearSSL::X509List *clientCert = NULL, BearSSL::PrivateKey *clientPrivateKey = NULL);
#endif
#endif
   bool isConnected(void);

   void onEvent(SocketIOClientEvent cbEvent);

   bool sendEVENT(uint8_t *payload, size_t length = 0, bool headerToPayload = false);
   bool sendEVENT(const uint8_t *payload, size_t length = 0);
   bool sendEVENT(char *payload, size_t length = 0, bool headerToPayload = false);
   bool sendEVENT(const char *payload, size_t length = 0);
   bool sendEVENT(String &payload);

   bool send(socketIOmessageType_t type, uint8_t *payload, size_t length = 0, bool headerToPayload = false);
   bool send(socketIOmessageType_t type, const uint8_t *payload, size_t length = 0);
   bool send(socketIOmessageType_t type, char *payload, size_t length = 0, bool headerToPayload = false);
   bool send(socketIOmessageType_t type, const char *payload, size_t length = 0);
   bool send(socketIOmessageType_t type, String &payload);

   void loop(void);

   void configureEIOping(bool disableHeartbeat = false);

   void on(const char *event, std::function<void(const char *payload, size_t length)>);
   void on(String event, std::function<void(const char *payload, size_t length)>);
   void remove(const char *event);
   void remove(String event);
   void removeAll(void);
   void emit(const char *event, const char *payload = NULL);
   void emit(String event, String payload);
   void handleEvent(uint8_t *payload);

 protected:
   const char *_nsp;
   bool _disableHeartbeat = false;
   uint64_t _lastHeartbeat = 0;
   SocketIOClientEvent _cbEvent;

   std::vector<String> _packets;
   std::map<String, std::function<void(const char *payload, size_t length)>> _events;

   void trigger(const char *event, const char *payload, size_t length);
   String getEventName(String msg);
   String getEventPayload(String msg);

   virtual void runIOCbEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
      if (_cbEvent) {
         _cbEvent(type, payload, length);
      }
      // soketEvent(type, payload, length);
   }

   void initClient(void);

   void socketEvent(socketIOmessageType_t type, uint8_t *payload, size_t length);

   // Handeling events from websocket layer
   virtual void runCbEvent(WStype_t type, uint8_t *payload, size_t length) { handleCbEvent(type, payload, length); }
   void handleCbEvent(WStype_t type, uint8_t *payload, size_t length);
};

#endif /* ARDUINOSOCKETIOCLIENT_H_ */
