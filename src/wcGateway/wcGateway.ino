#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <WiFiClient.h>

#include "/home/holla/r/p/projects/WarmChicken/src/wcGateway/auth"

/* 'auth' file has these lines, need to replace this
  String apiKey         = "????";
*/

boolean debug = 0;

ESP8266WebServer httpServer(80);
WebSocketsServer webSocketServer = WebSocketsServer(81);
ESP8266HTTPUpdateServer httpUpdater;
const char *update_path = "/firmware";
WiFiManager wifiManager;
#include "fs.h"

WiFiServer telnetd(23); // for command line and avrdude dfu

#define rebootWarmChicken() Serial.println("R")
#define JSONSIZE 1000
char json[JSONSIZE];
int jsonValid;
unsigned int jsonIndex;

const char* tsAPIHost = "api.thingspeak.com";
WiFiClient tsClient;

#define TIMEOUTPOST 60000L
unsigned long timeoutPost;

unsigned long systemCorrelationId;  // 434,
unsigned long systemUptime;         // 573,
float         temperatureInterior;  // 32.6,
float         temperatureExterior;  // 29,
unsigned int  lightLevelExterior;   // 413,
unsigned int  lightLevelInterior;   // 0,
float         batteryVoltage;       // 12.57,
char          systemMode[10];       // "auto",
int           doorMotorSpeed;       // 0,
char          doorState[10];        // "open",
float         doorStatef;
int           heaterPower;          // 0,
int           switchLight;          // 0,
char          switchRunStop[14];    // "auto",
char          switchJog[10];        // "off",
unsigned int  switchLimitUpper;     // 0,
unsigned int  switchDoorOpen;       // 1,
unsigned int  switchDoorClosed;     // 0,
unsigned int  doorMotorRuntime;     // 0

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      break;
    case WStype_TEXT:
      switch (payload[0]) {
        case 'r':
          rebootWarmChicken();
          break;
        case 'o':
          Serial.print("M");
          delay(100);
          Serial.print("O");
          break;
        case 'c':
          Serial.print("M");
          delay(100);
          Serial.print("C");
          break;
        case 'a':
          Serial.print("A");
          break;
        case 'm':
          Serial.print("M");
          break;
        case 'h':
          Serial.print("H");
          break;
        case 'l':
          Serial.print("L");
          break;
        case 'f':
          Serial.print("F");
          break;
        case 't':
          Serial.print("t");
          break;
      }
      break;
    case WStype_BIN:
      // hexdump(payload, length);
      // echo data back to browser
      break;
    default:
      break;
  }
}

void setup(void) {
  Serial.begin(57600);
  if (debug) {
    Serial.println("\n\nwcGateway begin");
  }

  fsSetup();

  //  wifiManager.resetSettings();
  wifiManager.autoConnect(LOCATION);

  if (MDNS.begin("warmchicken") ) {
    if (debug) Serial.println("MDNS responder started");
  }

  varInit();

  httpServer.on ("/r", []() {
    rebootWarmChicken();
  } );
  httpServer.on ("/o", []() { // open
    Serial.println("M");
    delay(100);
    Serial.println("O");
  } );
  httpServer.on ("/c", []() { // close
    Serial.println("M");
    delay(100);
    Serial.println("C");
  } );
  httpServer.on ("/a", []() { // auto
    Serial.println("A");
  } );
  httpServer.on ("/m", []() { // manual
    Serial.println("M");
  } );
  httpServer.on ("/h", []() { // heat on
    Serial.println("H");
  } );
  httpServer.on ("/l", []() { // light on
    Serial.println("L");
  } );
  httpServer.on ("/f", []() { // light off
    Serial.println("F");
  } );
  httpServer.on ("/t", []() { // light toggle
    Serial.println("t");
  } );

  httpUpdater.setup(&httpServer, update_path);
  httpServer.begin();
  timeoutPost = 0;
  jsonValid = 0;
  if (debug) Serial.println ("HTTP server started");

  telnetd.begin();
  //  telnetd.setNoDelay(true);

  // we're reseting warmchicken board because esp8266 boot
  // process spews a bunch of crap, don't know where it is
  webSocketServer.begin();
  webSocketServer.onEvent(webSocketEvent);

  delay(1000);
  rebootWarmChicken();
}

void post() {
  if (tsClient.connect(tsAPIHost, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(systemUptime);
    postStr += "&field2=";
    postStr += String(batteryVoltage);
    postStr += "&field3=";
    postStr += String(doorStatef);
    postStr += "&field4=";
    postStr += String(heaterPower);
    postStr += "&field5=";
    postStr += String(lightLevelExterior);
    postStr += "&field6=";
    postStr += String(temperatureInterior);
    postStr += "&field7=";
    postStr += String(temperatureExterior);
    postStr += "&field8=";
    postStr += String(lightLevelInterior);
    //    postStr += "&field8=";
    //    postStr += String(other);
    if (debug) Serial.println(postStr);

    tsClient.print("POST /update HTTP/1.1\n");
    tsClient.print("Host: api.thingspeak.com\n");
    tsClient.print("Connection: close\n");
    tsClient.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    tsClient.print("Content-Type: application/x-www-form-urlencoded\n");
    tsClient.print("Content-Length: ");
    tsClient.print(postStr.length());
    tsClient.print("\n\n");
    tsClient.print(postStr);
  }

  if (tsClient.connect(tsAPIHost, 80)) {
    String postStr = "PO2TBFL6DZWAIJFF";
    postStr += "&field1=";
    postStr += String(int(millis()/1000));

    tsClient.print("POST /update HTTP/1.1\n");
    tsClient.print("Host: api.thingspeak.com\n");
    tsClient.print("Connection: close\n");
    tsClient.print("X-THINGSPEAKAPIKEY: PO2TBFL6DZWAIJFF\n");
    tsClient.print("Content-Type: application/x-www-form-urlencoded\n");
    tsClient.print("Content-Length: ");
    tsClient.print(postStr.length());
    tsClient.print("\n\n");
    tsClient.print(postStr);
  }
}

char c;

/* this is coming from the board */
void jsonProcess() {
  char line[40];

  webSocketServer.broadcastTXT(json,strlen(json));

  JsonObject root;
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return;
  } 

  root = doc.as<JsonObject>();

  systemCorrelationId = root["systemCorrelationId"];
  systemUptime = root["systemUptime"];
  batteryVoltage = root["batteryVoltage"];
  heaterPower = root["heaterPower"];
  lightLevelExterior = root["lightLevelExterior"];
  lightLevelInterior = root["lightLevelInterior"];
  temperatureInterior = root["temperatureInterior"];
  temperatureExterior = root["temperatureExterior"];
  doorMotorSpeed = root["doorMotorSpeed"];
  switchLight = root["switchLight"];
  switchLimitUpper = root["switchLimitUpper"];
  switchDoorOpen = root["switchDoorOpen"];
  switchDoorClosed = root["switchDoorClosed"];
  doorMotorRuntime = root["doorMotorRuntime"];

  if (switchDoorOpen == 1) {
    doorStatef = 1;
  } else {
    if (switchDoorClosed == 1) {
      doorStatef = 0;
    } else {
      doorStatef = 0.5;
    }
  }
  sprintf(line,"{\"wcGatewayUptime\":%u}",millis()/1000);
  webSocketServer.broadcastTXT(line,strlen(line));

  strcpy(systemMode, root["systemMode"]);
  strcpy(doorState, root["doorState"]);
  strcpy(switchRunStop, root["switchRunStop"]);
  strcpy(switchJog, root["switchJog"]);

  jsonValid = 1;
}


void loopSerial() {
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c == '{') {
      jsonIndex = 0;
    }
    json[jsonIndex] = c;
    if (c == '}') {
      json[++jsonIndex] = 0;
      jsonProcess();
    }
    jsonIndex++;
    if (jsonIndex > JSONSIZE) {
      jsonIndex = 0;
    }
  }
}

void loopPost() {
  if ((millis() > timeoutPost) && (jsonValid)) {
    post();
    timeoutPost = millis() + TIMEOUTPOST;
    jsonValid = 0;
  }
}

void loopTelnetd() {
  WiFiClient client = telnetd.available();
  if (client) {
    if (debug) Serial.println("Client connect");
    while (client.connected()) {
      if (client.available()) {
        Serial.write(client.read());
      }
      if (Serial.available()) {
        client.write(Serial.read());
      }
    }
    if (debug) Serial.println("Client disconnect");
    client.stop();
  }
}

void loop(void) {
  httpServer.handleClient();
  loopSerial();
  loopPost();
  loopTelnetd();
  webSocketServer.loop();
  ESP.wdtFeed(); 
  yield();
}

void varInit(void) {
  if (debug) {
    Serial.println("varInit");
  }

  batteryVoltage        = 0;
  doorMotorRuntime      = 0;
  doorMotorSpeed        = 0;
  strcat(doorState, "???");
  doorStatef            = 0.0;
  heaterPower           = 0;
  lightLevelExterior    = 0;
  lightLevelInterior    = 0;
  switchDoorClosed      = 0;
  switchDoorOpen        = 0;
  strcat(switchJog, "???");
  switchLight           = 0;
  switchLimitUpper      = 0;
  strcat(switchRunStop, "???");
  systemCorrelationId   = 0;
  strcat(systemMode, "???");
  systemUptime          = 0;
  temperatureExterior   = 0;
  temperatureInterior   = 0;
}

/*
  what firmware sends
  {"systemCorrelationId":222,"systemUptime":271,"temperatureInterior":68.8,"temperatureExterior":88.6,"lightLevelExterior":977,"lightLevelInterior":0,"batteryVoltage":13.59,"systemMode":"auto","doorMotorSpeed":0,"doorState":"open","heaterPower":0,"switchLight":0,"switchRunStop":"auto","switchJog":"off","switchLimitUpper":0,"switchDoorOpen":1,"switchDoorClosed":0,"doorMotorRuntime":35}

  {
  "systemCorrelationId": 434,
  "systemUptime": 573,
  "temperatureInterior": 32.6,
  "temperatureExterior": 29,
  "lightLevelExterior": 413,
  "lightLevelInterior": 0,
  "batteryVoltage": 12.57,
  "systemMode": "auto",
  "doorMotorSpeed": 0,
  "doorState": "open",
  "heaterPower": 0,
  "switchLight": 0,
  "switchRunStop": "auto",
  "switchJog": "off",
  "switchLimitUpper": 0,
  "switchDoorOpen": 1,
  "switchDoorClosed": 0,
  "doorMotorRuntime": 0
  }

*/
