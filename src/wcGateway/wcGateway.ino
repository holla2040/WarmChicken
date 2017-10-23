#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager

#include "/home/holla/r/p/projects/WarmChicken/src/wcGateway/auth"

/* 'auth' file has these lines
  const char *ssid      = "????";
  const char *password  = "????";

  #define LOCATION      "????"
  String apiKey         = "????";
*/


boolean debug = 0;

ESP8266WebServer httpd(80);
ESP8266HTTPUpdateServer httpUpdater;
const char *update_path = "/firmware";
WiFiManager wifiManager;
// curl -F "image=@/tmp/arduino_build_651930/wcGateway.ino.bin" http://192.168.0.10/firmware

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
char          doorState[10];            // "open",
float         doorStatef;
int           heaterPower;          // 0,
int           switchLight;          // 0,
char          switchRunStop[14];        // "auto",
char          switchJog[10];            // "off",
unsigned int  switchLimitUpper;     // 0,
unsigned int  switchDoorOpen;       // 1,
unsigned int  switchDoorClosed;     // 0,
unsigned int  doorMotorRuntime;     // 0

#define HTMLLEN 1000
void handleRoot() {
  String postStr = "<head><meta http-equiv='refresh' content='5'/><title>";
  postStr += String(LOCATION);
  postStr += "Data</title><style>a {font:bold 11px Arial;text-decoration:none;background-color:#EEEEEE;color:#333333;padding:2px 6px 2px 6px;border-top:1px solid #CCCCCC;border-right:1px solid #333333;border-bottom:1px solid #333333;border-left:1px solid #CCCCCC;}</style>";
  postStr += "</head><body><pre>";
  postStr += "location:             ";
  postStr += String(LOCATION);
  postStr += "<br>gatewayUptime:        ";
  postStr += String(int(millis() / 1000));
  postStr += "<hr>batteryVoltage:       ";
  postStr += String(batteryVoltage);

  postStr += "<br>doorMotorRuntime:     ";
  postStr += String(doorMotorRuntime);
  postStr += "<br>doorMotorSpeed:       ";
  postStr += String(doorMotorSpeed);
  postStr += "<br>doorState:            ";
  postStr += String(doorState);
  postStr += "<br>doorStatef:           ";
  postStr += String(doorStatef);
  postStr += "<br>heaterPower:          ";
  postStr += String(heaterPower);
  postStr += "<br>lightLevelExterior    ";
  postStr += String(lightLevelExterior);
  postStr += "<br>lightLevelInterior:   ";
  postStr += String(lightLevelInterior);
  postStr += "<br>switchDoorClosed:     ";
  postStr += String(switchDoorClosed);
  postStr += "<br>switchDoorOpen:       ";
  postStr += String(switchDoorOpen);
  postStr += "<br>switchJog:            ";
  postStr += String(switchJog);
  postStr += "<br>switchLight:          ";
  postStr += String(switchLight);

  postStr += "<br>switchLimitUpper:     ";
  postStr += String(switchLimitUpper);
  postStr += "<br>switchRunStop:        ";
  postStr += String(switchRunStop);
  postStr += "<br>systemCorrelationId:  ";
  postStr += String(systemCorrelationId);
  postStr += "<br>systemMode:           ";
  postStr += String(systemMode);
  postStr += "<br>systemUptime:         ";
  postStr += String(systemUptime);
  postStr += "<br>temperatureExterior:  ";
  postStr += String(temperatureExterior);
  postStr += "<br>temperatureInterior:  ";
  postStr += String(temperatureInterior);
  postStr += "</pre><hr><a href='/o'>Open</a>&nbsp;&nbsp;<a href='/c'>Close</a>&nbsp;&nbsp;<a href='/h'>Heat</a>&nbsp;&nbsp;<a href='/t'>Light</a>&nbsp;&nbsp;<a href='/a'>Auto</a>&nbsp;&nbsp;<a href='/m'>Manual</a>&nbsp;&nbsp;<a href='/r'>Reset</a></body></html>";
  httpd.send ( 200, "text/html", postStr);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpd.uri();
  message += "\nMethod: ";
  message += ( httpd.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpd.args();
  message += "\n";

  for (uint8_t i = 0; i < httpd.args(); i++) {
    message += " " + httpd.argName(i) + ": " + httpd.arg(i) + "\n";
  }

  httpd.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(57600);
  if (debug) {
    Serial.println("");
    Serial.println("\n\nwcGateway begin");
  }

  //  wifiManager.resetSettings();
  wifiManager.autoConnect(LOCATION);

  if (MDNS.begin(LOCATION) ) {
    if (debug) Serial.println("MDNS responder started");
  }

  varInit();

  httpd.on ("/", handleRoot);
  httpd.on ("/inline", []() {
    httpd.send (200, "text/plain", "this works as well");
  } );
  httpd.on ("/r", []() {
    rebootWarmChicken();
    handleRoot();
  } );
  httpd.on ("/o", []() { // open
    Serial.println("M");
    delay(100);
    Serial.println("O");
    handleRoot();
  } );
  httpd.on ("/c", []() { // close
    Serial.println("M");
    delay(100);
    Serial.println("C");
    handleRoot();
  } );
  httpd.on ("/a", []() { // auto
    Serial.println("A");
    handleRoot();
  } );
  httpd.on ("/m", []() { // manual
    Serial.println("M");
    handleRoot();
  } );
  httpd.on ("/h", []() { // heat on
    Serial.println("H");
    handleRoot();
  } );
  httpd.on ("/l", []() { // light on
    Serial.println("L");
    handleRoot();
  } );
  httpd.on ("/f", []() { // light off
    Serial.println("F");
    handleRoot();
  } );
  httpd.on ("/t", []() { // light toggle
    Serial.println("t");
    handleRoot();
  } );


  httpd.onNotFound (handleNotFound);

  httpUpdater.setup(&httpd, update_path);
  httpd.begin();
  timeoutPost = 0;
  jsonValid = 0;
  if (debug) Serial.println ("HTTP server started");

  telnetd.begin();
  //  telnetd.setNoDelay(true);

  // we're reseting warmchicken board because esp8266 boot
  // process spews a bunch of crap, don't know where it is
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
  if (debug) Serial.println("post");
}

char c;

void jsonProcess() {
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);

  if (!root.success()) {
    return;
  }

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

  strcpy(systemMode, root["systemMode"]);
  strcpy(doorState, root["doorState"]);
  strcpy(switchRunStop, root["switchRunStop"]);
  strcpy(switchJog, root["switchJog"]);

  if (debug) {
    Serial.print("jsonProcess ");
    Serial.println(root.success());
  }
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
  httpd.handleClient();
  loopSerial();
  loopPost();
  loopTelnetd();
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


