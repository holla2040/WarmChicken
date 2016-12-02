#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#include "/home/holla/r/p/projects/WarmChicken/src/wcGateway/auth"

/* 'auth' file has these lines
  const char *ssid      = "????";
  const char *password  = "????";

  #define LOCATION      "????"
  String apiKey         = "????";
*/


boolean debug = 0;

ESP8266WebServer httpd(80);

WiFiServer telnetd(23); // for command line and avrdude dfu

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

#define HTMLLEN 800
void handleRoot() {
  char temp[HTMLLEN];
  snprintf(temp, HTMLLEN, "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>%s Data</title>\
    <style>\
      body { }\
    </style>\
  </head>\
  <body>\
    <pre>\
location:             %s<br><hr>\
batteryVoltage:       %f<br>\
doorMotorRuntime:     %d<br>\
doorMotorSpeed:       %d<br>\
doorState:            %s<br>\
doorStatef:           %f<br>\
heaterPower:          %d<br>\
lightLevelExterior:   %u<br>\
lightLevelInterior:   %u<br>\
switchDoorClosed:     %d<br>\
switchDoorOpen:       %d<br>\
switchJog:            %s<br>\
switchLight:          %d<br>\
switchLimitUpper:     %d<br>\
switchRunStop:        %s<br>\
systemCorrelationId:  %lu<br>\
systemMode:           %s<br>\
systemUptime:         %lu<br>\
temperatureExterior:  %f<br>\
temperatureInterior:  %f<br>\   
   </pre>\
  </body>\
</html>", LOCATION, LOCATION,
           batteryVoltage,
           doorMotorRuntime,
           doorMotorSpeed,
           doorState,
           doorStatef,
           heaterPower,
           lightLevelExterior,
           lightLevelInterior,
           switchDoorClosed,
           switchDoorOpen,
           switchJog,
           switchLight,
           switchLimitUpper,
           switchRunStop,
           systemCorrelationId,
           systemMode,
           systemUptime,
           temperatureExterior,
           temperatureInterior
          );

  httpd.send ( 200, "text/html", temp );
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
  WiFi.begin(ssid, password );
  if (debug) Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (debug) Serial.print(".");
  }

  if (debug) {
    Serial.println("");
    Serial.print("Connected to " );
    Serial.println(ssid );
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP() );
  }

  if (MDNS.begin(LOCATION) ) {
    if (debug) Serial.println("MDNS responder started");
  }

  httpd.on ("/", handleRoot);
  httpd.on ("/inline", []() {
    httpd.send (200, "text/plain", "this works as well");
  } );
  httpd.onNotFound (handleNotFound);
  httpd.begin();
  timeoutPost = 0;
  jsonValid = 0;
  if (debug) Serial.println ("HTTP server started");

  telnetd.begin();
  //  telnetd.setNoDelay(true);
}

void post() {
  if (tsClient.connect(tsAPIHost, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(systemUptime);
    postStr += "&field2=";
    postStr += String(batteryVoltage);
    postStr += "&field3=";
    postStr += String(doorState);
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

    tsClient.print("POST /update HTTP/1.1\n");
    tsClient.print("Host: api.thingspeak.com\n");
    tsClient.print("Connection: close\n");
    tsClient.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    tsClient.print("Content-Type: application/x-www-form-urlencoded\n");
    tsClient.print("Content-Length: ");
    tsClient.print(postStr.length());
    tsClient.print("\n\n");
    tsClient.print(postStr);
    if (debug) Serial.println(postStr);
  }
  if (debug) Serial.println("post");
}

char c;

void jsonProcess() {
  StaticJsonBuffer<350> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);

  if (!root.success()) {
    return;
  }

  systemCorrelationId = root["systemCorrelationId"];
  systemUptime = root["systemUptime"];
  batteryVoltage = root["batteryVoltage"];

  if (strstr(root["doorState"], "open")) {
    doorStatef = 1;
  }
  if (strstr(root["doorState"], "closed")) {
    doorStatef = 0;
  }
  if (strstr(root["doorState"], "stopped")) {
    doorStatef = 0.5;
  }

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

  strcpy(systemMode,root["systemMode"]);
  strcpy(doorState,root["doorState"]);
  strcpy(switchRunStop,root["switchRunStop"]);
  strcpy(switchJog,root["switchJog"]);

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
}

/*

  what firmware sends

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


