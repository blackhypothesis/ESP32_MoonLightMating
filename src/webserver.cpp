#include "webserver.h"

AsyncWebServer* newWebServer() {
  // ---------------------------------------------------------
  // Web Server
  // ---------------------------------------------------------
  // Create AsyncWebServer object on port 80
  AsyncWebServer *webserver;
  webserver = new AsyncWebServer(80);
  // ---------------------------------------------------------

  // Web Server API
  webserver->on("/", HTTP_GET, requestRootURL);
  webserver->on("/", HTTP_POST, requestSaveHiveWifiConfig);
  webserver->on("/getversion", HTTP_GET, requestGetVersion);
  webserver->on("/getdatetime", HTTP_GET, requestGetDateTime);
  webserver->on("/gethiveconfig", HTTP_GET, requestGetHiveConfig);
  webserver->on("/getwificonfig", requestGetWifiConfig);
  webserver->on("/resetdefaultconfig", HTTP_GET, requestResetDefaultConfig);
  webserver->on("/reboot", HTTP_GET, requestReboot);
  webserver->on("/getconfigstatus", HTTP_GET, requestGetConfigStatus);
  webserver->on("/getconfigstatusclient", HTTP_GET, requestGetConfigStatusClient);
  webserver->on("/setdatetime", HTTP_GET, requestSetDateTime);
  webserver->on("/setscheduleconfig", HTTP_GET, requestSetScheduleConfig);
  webserver->on("/getclientstates", HTTP_GET, requestGetClientStates);
  webserver->on("/scanwifi", HTTP_GET, requestScanWifi);
  webserver->on("/secondssinceboot", HTTP_GET, requestSecondsSinceBoot);
  webserver->on("/mctrl", HTTP_GET, requestMotorControl);

  webserver->serveStatic("/", SPIFFS, "/");

  // Start OTA capability to webserver
  // ElegantOTA.begin(&server);

  // Start server
  // webserver->begin();

  return webserver;
}

AsyncWebSocket* newWebSocket() {
  // Create a WebSocket object
  AsyncWebSocket *ws;
  ws = new AsyncWebSocket("/ws");
  ws->onEvent(onEvent);
  return ws;
}

void notifyClients(AsyncWebSocket *ws, String state) {
  ws->textAll(state);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    JsonDocument control_motor;
    Serial.printf("String data: %s\n", (char *)data);
    DeserializationError error = deserializeJson(control_motor, (char *)data);
    if (error) {
      Serial.printf("%s Error:deserialization failed: %s\n", getDateTime().c_str(), error.f_str());
    }
    else {
      int motor_nr = control_motor["motor"];
      int steps = control_motor["steps"];
      int command = control_motor["command"];
      Serial.printf("%s handleWebSocketMessage: steps = %d, command = %d\n", getDateTime().c_str(), steps, command);
      queueMotorControl(motor_nr, (MotorCommand)command, steps);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("%s WebSocket client #%u connected from %s\n", getDateTime().c_str(), client->id(), client->remoteIP().toString().c_str());
      //Notify client of state
      notifyClients(server, "state");
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("%s WebSocket client #%u disconnected\n", getDateTime().c_str(), client->id());
      break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
     break;
  }
}