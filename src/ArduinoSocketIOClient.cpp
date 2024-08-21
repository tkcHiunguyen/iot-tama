/*
 * ArduinoSocketIOClient.cpp
 *
 *  Created on: Nov 04, 2021
 *      Author: nqnghia285
 */
#include "ArduinoSocketIOClient.h"

ArduinoSocketIOClient::ArduinoSocketIOClient() {}

ArduinoSocketIOClient::~ArduinoSocketIOClient() {}

/**
 * @brief Configure connect to server
 *
 * @param host  const char *
 * @param port  uint16_t
 * @param nsp const char *
 * @param url const char *
 * @param protocol const char *
 */
void ArduinoSocketIOClient::begin(const char *host, uint16_t port, const char *nsp, const char *url, const char *protocol) {
   _nsp = nsp;
   WebSocketsClient::beginSocketIO(host, port, url, protocol);
   WebSocketsClient::enableHeartbeat(60 * 1000, 90 * 1000, 5);
   initClient();
}

/**
 * @brief Configure connect to server
 *
 * @param host String
 * @param port uint16_t
 * @param nsp String
 * @param url String
 * @param protocol String
 */
void ArduinoSocketIOClient::begin(String host, uint16_t port, String nsp, String url, String protocol) {
   _nsp = nsp.c_str();
   WebSocketsClient::beginSocketIO(host, port, url, protocol);
   WebSocketsClient::enableHeartbeat(60 * 1000, 90 * 1000, 5);
   initClient();
}
#if defined(HAS_SSL)
/**
 * @brief Configure connect to server with ssl
 *
 * @param host const char *
 * @param port uint16_t
 * @param nsp const char *
 * @param url const char *
 * @param protocol const char *
 */
void ArduinoSocketIOClient::beginSSL(const char *host, const char *nsp, uint16_t port, const char *url, const char *protocol) {
   _nsp = nsp;
   WebSocketsClient::beginSocketIOSSL(host, port, url, protocol);
   WebSocketsClient::enableHeartbeat(60 * 1000, 90 * 1000, 5);
   initClient();
}

/**
 * @brief Configure connect to server with ssl
 *
 * @param host String
 * @param port uint16_t
 * @param nsp String
 * @param url String
 * @param protocol String
 */
void ArduinoSocketIOClient::beginSSL(String host, String nsp, uint16_t port, String url, String protocol) {
   _nsp = nsp.c_str();
   WebSocketsClient::beginSocketIOSSL(host, port, url, protocol);
   WebSocketsClient::enableHeartbeat(60 * 1000, 90 * 1000, 5);
   initClient();
}
#if defined(SSL_BARESSL)
/**
 * @brief Configure connect to server with ssl + ca
 *
 * @param host const char *
 * @param port uint16_t
 * @param nsp const char *
 * @param url const char *
 * @param CA_cert const char *
 * @param protocol const char *
 */
void ArduinoSocketIOClient::beginSSLWithCA(const char *host, const char *nsp, uint16_t port, const char *url, const char *CA_cert, const char *protocol) {
   _nsp = nsp;
   WebSocketsClient::beginSocketIOSSLWithCA(host, port, url, CA_cert, protocol);
   WebSocketsClient::enableHeartbeat(60 * 1000, 90 * 1000, 5);
   initClient();
}

/**
 * @brief Configure connect to server with ssl + ca
 *
 * @param host const char *
 * @param port uint16_t
 * @param nsp const char *
 * @param url const char *
 * @param CA_cert BearSSL::X509List *
 * @param protocol const char *
 */
void ArduinoSocketIOClient::beginSSLWithCA(const char *host, const char *nsp, uint16_t port, const char *url, BearSSL::X509List *CA_cert, const char *protocol) {
   _nsp = nsp;
   WebSocketsClient::beginSocketIOSSLWithCA(host, port, url, CA_cert, protocol);
   WebSocketsClient::enableHeartbeat(60 * 1000, 90 * 1000, 5);
   initClient();
}

/**
 * @brief Set cert key
 *
 * @param clientCert const char *
 * @param clientPrivateKey const char *
 */
void ArduinoSocketIOClient::setSSLClientCertKey(const char *clientCert, const char *clientPrivateKey) { WebSocketsClient::setSSLClientCertKey(clientCert, clientPrivateKey); }

/**
 * @brief Set cert key
 *
 * @param clientCert BearSSL::X509List *
 * @param clientPrivateKey BearSSL::PrivateKey *
 */
void ArduinoSocketIOClient::setSSLClientCertKey(BearSSL::X509List *clientCert, BearSSL::PrivateKey *clientPrivateKey) { WebSocketsClient::setSSLClientCertKey(clientCert, clientPrivateKey); }

#endif
#endif

void ArduinoSocketIOClient::configureEIOping(bool disableHeartbeat) { _disableHeartbeat = disableHeartbeat; }

/**
 * @brief Initiate client and bind to function param in function onEvent. You
 * can override it for your customizing
 *
 */
void ArduinoSocketIOClient::initClient(void) {
   onEvent(std::bind(&ArduinoSocketIOClient::socketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

   if (_client.cUrl.indexOf("EIO=4") != -1) {
      SOCKETIOCLIENT_DEBUG("[wsIOc] found EIO=4 disable EIO ping on client\n");
      configureEIOping(true);
   }
}

/**
 * @brief Add a listener function into _packets, this listener can handle event
 * that is sent from server
 *
 * @param event const char *
 * @param func std::function<void(const char *payload, size_t length)>
 */
void ArduinoSocketIOClient::on(const char *event, std::function<void(const char *payload, size_t length)> func) { _events[event] = func; }

/**
 * @brief Add a listener function into _packets, this listener can handle event
 * that is sent from server
 *
 * @param event String
 * @param func std::function<void(const char *payload, size_t length)>
 */
void ArduinoSocketIOClient::on(String event, std::function<void(const char *payload, size_t length)> func) { on(event.c_str(), func); }

/**
 * @brief Remove the event handle function in _events of class
 * ArduinoSocketIOClient
 *
 * @param event const char *
 */
void ArduinoSocketIOClient::remove(const char *event) {
   auto e = _events.find(event);
   if (e != _events.end()) {
      _events.erase(e);
   } else {
      SOCKETIOCLIENT_DEBUG("[SIoC] event %s not found, can not be removed", event);
   }
}

/**
 * @brief Remove the event handle function in _events of class
 * ArduinoSocketIOClient
 *
 * @param event String
 */
void ArduinoSocketIOClient::remove(String event) { remove(event.c_str()); }

/**
 * @brief Remove all of event handle functions in _events of class
 * ArduinoSocketIOClient
 *
 */
void ArduinoSocketIOClient::removeAll() { _events.clear(); }

/**
 * @brief Function send event + message to server. This function support format
 * JSON message
 *
 * @param event const char *
 * @param payload const char *
 */
void ArduinoSocketIOClient::emit(const char *event, const char *payload) {
   if (isConnected()) {
      const int capacity = FACTOR * (String(_nsp).length() + String(event).length() + String(payload).length());

      DynamicJsonDocument _doc(capacity);
      JsonArray _ms = _doc.to<JsonArray>();

      // Add event name
      // Hint: socket.on('event_name', ....
      _ms.add(event);

      // Add message
      _ms.add(payload);

      // JSON to String (serializion)
      String message;
      serializeJson(_ms, message);

      // Add namespace
      if (!String(_nsp).equals("/")) {
         // Hint: _nsp,[_event_name,_message]
         message = String(_nsp) + "," + message;
      }

      // SOCKETIOCLIENT_DEBUG("[SIoC] add packet %s\n", message.c_str());
      _packets.push_back(message);

      // Clear store
      _doc.clear();
   } else {
      SOCKETIOCLIENT_DEBUG("[SIoC]: Disconnected!");
   }
}

/**
 * @brief Function send event + message to server. This function support format
 * JSON message
 *
 * @param event String
 * @param payload String
 */
void ArduinoSocketIOClient::emit(String event, String payload) { emit(event.c_str(), payload.c_str()); }

/**
 * @brief This function is used for calling listener function in _packets to
 * handle event.
 *
 * @param event const char *
 * @param payload const char *
 * @param length size_t
 */
void ArduinoSocketIOClient::trigger(const char *event, const char *payload, size_t length) {
   auto e = _events.find(event);
   if (e != _events.end()) {
      // SOCKETIOCLIENT_DEBUG("[SIoC] trigger event %s\n", event);
      e->second(payload, length);
   } else {
      SOCKETIOCLIENT_DEBUG("[SIoC] event %s not found. %d events available\n", event, _events.size());
   }
}

/**
 * @brief Get event name of message string that is sent from server
 *
 * @param msg String
 * @return String
 */
String ArduinoSocketIOClient::getEventName(String msg) {
   const int capacity = FACTOR * msg.length();
   DynamicJsonDocument doc(capacity);

   String result = "";

   // Remove prefix namespace of message
   if (!String(_nsp).equals("/")) {
      msg = msg.substring(msg.indexOf("["));
   }

   // Deserialize msg to array: [event,payload]
   DeserializationError err = deserializeJson(doc, msg);

   if (err == DeserializationError::Ok) {
      // Get a reference to the root array
      JsonArray arr = doc.as<JsonArray>();
      // Get first element that is event name
      result = String((const char *)arr.getElement(0));

      // Remove character "
      if (result.startsWith("\"")) {
         result.remove(0, 1);
      }

      if (result.endsWith("\"")) {
         result.remove(result.length() - 1);
      }
   }

   // Clear store
   doc.clear();

   // Return result
   return result;
}

/**
 * @brief Get payload of message string that is sent from server
 *
 * @param msg String
 * @return String
 */
String ArduinoSocketIOClient::getEventPayload(String msg) {
   const int capacity = FACTOR * msg.length();
   DynamicJsonDocument doc(capacity);

   String result = "";

   // Remove prefix namespace of message
   if (!String(_nsp).equals("/")) {
      msg = msg.substring(msg.indexOf("["));
   }

   // Deserialize msg to array: [event,payload]
   DeserializationError err = deserializeJson(doc, msg);

   if (err == DeserializationError::Ok) {
      // Get a reference to the root array
      JsonArray arr = doc.as<JsonArray>();
      // Get second element that is payload
      result = String((const char *)arr.getElement(1));

      // Remove character "
      if (result.startsWith("\"")) {
         result.remove(0, 1);
      }

      if (result.endsWith("\"")) {
         result.remove(result.length() - 1);
      }
   }

   // Clear store
   doc.clear();

   // Return result
   return result;
}

/**
 * @brief This function is used for handling event that is sent from server.
 *
 * @param payload uint8_t *
 */
void ArduinoSocketIOClient::handleEvent(uint8_t *payload) {
   String msg = String((char *)payload);
   trigger(getEventName(msg).c_str(), getEventPayload(msg).c_str(), getEventPayload(msg).length());
}

/**
 * @brief Set callback function. This function is used for customizing your
 * event handle function
 *
 * @param cbEvent SocketIOClientEvent
 */
void ArduinoSocketIOClient::onEvent(SocketIOClientEvent cbEvent) { _cbEvent = cbEvent; }

/**
 * @brief Default event handle function. This will be binded in initClient
 * function. You can override it with onEvent function.
 *
 * @param type socketIOmessageType_t
 * @param payload uint8_t *
 * @param length size_t
 */
void ArduinoSocketIOClient::socketEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
   switch (type) {
   case sIOtype_DISCONNECT:
      SOCKETIOCLIENT_DEBUG("[SIoC] Disconnected!\n");
      break;
   case sIOtype_CONNECT:
      SOCKETIOCLIENT_DEBUG("[SIoC] Connected to url: %s\n", payload);

      // join default namespace (no auto join in Socket.IO V4)
      // Connect to server with default path: "/" or with namespace
      send(sIOtype_CONNECT, _nsp);

      break;
   case sIOtype_EVENT:
      SOCKETIOCLIENT_DEBUG("[SIoC] get event: %s\n", payload);

      // Serial.println("Message: " + String((char *)payload));
      // Handle message sent from server
      handleEvent(payload);
      break;
   // case sIOtype_ACK:
   //    SOCKETIOCLIENT_DEBUG("[SIoC] get ack: %u\n", length);
   //    hexdump(payload, length);
   //    break;
   // case sIOtype_ERROR:
   //    SOCKETIOCLIENT_DEBUG("[SIoCc] get error: %u\n", length);
   //    hexdump(payload, length);
   //    break;
   // case sIOtype_BINARY_EVENT:
   //    SOCKETIOCLIENT_DEBUG("[SIoC] get binary: %u\n", length);
   //    hexdump(payload, length);
   //    break;
   // case sIOtype_BINARY_ACK:
   //    SOCKETIOCLIENT_DEBUG("[SIoC] get binary ack: %u\n", length);
   //    hexdump(payload, length);
   //    break;
   }
}

/**
 * @brief Check status connection to server
 *
 * @return bool
 */
bool ArduinoSocketIOClient::isConnected(void) { return WebSocketsClient::isConnected(); }

/**
 * send text data to client
 * @param type socketIOmessageType_t
 * @param payload uint8_t *
 * @param length size_t
 * @param headerToPayload bool (see sendFrame for more details)
 * @return true if ok
 */
bool ArduinoSocketIOClient::send(socketIOmessageType_t type, uint8_t *payload, size_t length, bool headerToPayload) {
   bool ret = false;
   if (length == 0) {
      length = strlen((const char *)payload);
   }

   if (clientIsConnected(&_client) && _client.status == WSC_CONNECTED) {
      if (!headerToPayload) {
         // webSocket Header
         ret = WebSocketsClient::sendFrameHeader(&_client, WSop_text, length + 2, true);
         // Engine.IO / Socket.IO Header
         if (ret) {
            uint8_t buf[3] = {eIOtype_MESSAGE, type, 0x00};
            ret = WebSocketsClient::write(&_client, buf, 2);
         }
         if (ret && payload && length > 0) {
            ret = WebSocketsClient::write(&_client, payload, length);
         }
         return ret;
      } else {
         // TODO implement
      }
   }
   return false;
}

bool ArduinoSocketIOClient::send(socketIOmessageType_t type, const uint8_t *payload, size_t length) { return send(type, (uint8_t *)payload, length); }

bool ArduinoSocketIOClient::send(socketIOmessageType_t type, char *payload, size_t length, bool headerToPayload) { return send(type, (uint8_t *)payload, length, headerToPayload); }

bool ArduinoSocketIOClient::send(socketIOmessageType_t type, const char *payload, size_t length) { return send(type, (uint8_t *)payload, length); }

bool ArduinoSocketIOClient::send(socketIOmessageType_t type, String &payload) { return send(type, (uint8_t *)payload.c_str(), payload.length()); }

/**
 * send text data to client
 * @param num uint8_t client id
 * @param payload uint8_t *
 * @param length size_t
 * @param headerToPayload bool  (see sendFrame for more details)
 * @return true if ok
 */
bool ArduinoSocketIOClient::sendEVENT(uint8_t *payload, size_t length, bool headerToPayload) { return send(sIOtype_EVENT, payload, length, headerToPayload); }

bool ArduinoSocketIOClient::sendEVENT(const uint8_t *payload, size_t length) { return sendEVENT((uint8_t *)payload, length); }

bool ArduinoSocketIOClient::sendEVENT(char *payload, size_t length, bool headerToPayload) { return sendEVENT((uint8_t *)payload, length, headerToPayload); }

bool ArduinoSocketIOClient::sendEVENT(const char *payload, size_t length) { return sendEVENT((uint8_t *)payload, length); }

bool ArduinoSocketIOClient::sendEVENT(String &payload) { return sendEVENT((uint8_t *)payload.c_str(), payload.length()); }

/**
 * @brief Loop function is used for handling and sending events to server
 *
 */
void ArduinoSocketIOClient::loop(void) {
   WebSocketsClient::loop();
   unsigned long t = millis();
   if (!_disableHeartbeat && (t - _lastHeartbeat) > EIO_HEARTBEAT_INTERVAL) {
      _lastHeartbeat = t;
      SOCKETIOCLIENT_DEBUG("[wsIOc] send ping\n");
      WebSocketsClient::sendTXT(eIOtype_PING);
   }

   for (auto packet = _packets.begin(); packet != _packets.end();) {
      if (sendEVENT(*packet)) {
         // SOCKETIOCLIENT_DEBUG("[SIoC] packet \"%s\" emitted\n",
         // packet->c_str());
         packet = _packets.erase(packet);
      } else {
         ++packet;
      }
   }
}

/**
 * @brief Handle event that is sent from server. This function is called in
 * websocket layer.
 *
 * @param type WStype_t
 * @param payload uint8_t *
 * @param length size_t
 */
void ArduinoSocketIOClient::handleCbEvent(WStype_t type, uint8_t *payload, size_t length) {
   switch (type) {
   case WStype_DISCONNECTED:
      runIOCbEvent(sIOtype_DISCONNECT, NULL, 0);
      SOCKETIOCLIENT_DEBUG("[wsIOc] Disconnected!\n");
      break;
   case WStype_CONNECTED: {
      SOCKETIOCLIENT_DEBUG("[wsIOc] Connected to url: %s\n", payload);
      // send message to server when Connected
      // Engine.io upgrade confirmation message (required)
      WebSocketsClient::sendTXT("2probe");
      WebSocketsClient::sendTXT(eIOtype_UPGRADE);
      runIOCbEvent(sIOtype_CONNECT, payload, length);
   } break;
   case WStype_TEXT: {
      if (length < 1) {
         break;
      }

      engineIOmessageType_t eType = (engineIOmessageType_t)payload[0];
      switch (eType) {
      case eIOtype_PING:
         payload[0] = eIOtype_PONG;
         SOCKETIOCLIENT_DEBUG("[wsIOc] get ping send pong (%s)\n", payload);
         WebSocketsClient::sendTXT(payload, length, false);
         break;
      case eIOtype_PONG:
         SOCKETIOCLIENT_DEBUG("[wsIOc] get pong\n");
         break;
      case eIOtype_MESSAGE: {
         if (length < 2) {
            break;
         }
         socketIOmessageType_t ioType = (socketIOmessageType_t)payload[1];
         uint8_t *data = &payload[2];
         size_t lData = length - 2;
         switch (ioType) {
         case sIOtype_EVENT:
            SOCKETIOCLIENT_DEBUG("[wsIOc] get event (%d): %s\n", lData, data);
            break;
         case sIOtype_CONNECT:
            SOCKETIOCLIENT_DEBUG("[wsIOc] connected (%d): %s\n", lData, data);
            return;
         case sIOtype_DISCONNECT:
         case sIOtype_ACK:
         case sIOtype_ERROR:
         case sIOtype_BINARY_EVENT:
         case sIOtype_BINARY_ACK:
         default:
            SOCKETIOCLIENT_DEBUG("[wsIOc] Socket.IO Message Type %c (%02X) is not implemented\n", ioType, ioType);
            SOCKETIOCLIENT_DEBUG("[wsIOc] get text: %s\n", payload);
            break;
         }

         runIOCbEvent(ioType, data, lData);
      } break;
      case eIOtype_OPEN:
      case eIOtype_CLOSE:
      case eIOtype_UPGRADE:
      case eIOtype_NOOP:
      default:
         SOCKETIOCLIENT_DEBUG("[wsIOc] Engine.IO Message Type %c (%02X) is not implemented\n", eType, eType);
         SOCKETIOCLIENT_DEBUG("[wsIOc] get text: %s\n", payload);
         break;
      }
   } break;
   case WStype_ERROR:
   case WStype_BIN:
   case WStype_FRAGMENT_TEXT_START:
   case WStype_FRAGMENT_BIN_START:
   case WStype_FRAGMENT:
   case WStype_FRAGMENT_FIN:
   case WStype_PING:
   case WStype_PONG:
      break;
   }
}
