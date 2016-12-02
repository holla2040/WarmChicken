#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#include "/home/holla/r/p/projects/WarmChicken/src/firmware/auth"

/*
const char *ssid      = "????";
const char *password  = "????";

#define LOCATION      "????"
String apiKey         = "????";
*/


boolean debug = 0;

ESP8266WebServer server(80);

#define JSONSIZE 1000
char json[JSONSIZE];
int jsonValid;
unsigned int jsonIndex;

const char* tsServer = "api.thingspeak.com";
WiFiClient tsClient;

#define TIMEOUTPOST 60000L
unsigned long timeoutPost;

struct Status {
  unsigned long uptime;
  float batteryVoltage;
  unsigned int doorState;
  unsigned int heaterPower;
  unsigned int lightLevelExterior;
  float temperatureInterior;
  float temperatureExterior;
};

Status status;

void handleRoot() {
  char temp[500];
  int sec = status.uptime;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 500, "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>%s Data</title>\
    <style>\
      body { }\
    </style>\
  </head>\
  <body>\
    <pre>\
    Location:            %s<br>\
    Uptime:              %02d:%02d:%02d<br>\
    Battery Voltage:     %f<br>\
    Door State:          %d<br>\
    Heater Power:        %d<br>\
    Light Level Ext:     %d<br>\
    Temperature Outside: %d<br>\
    Temperature Inside:  %d<br>\
    </pre>\
  </body>\
</html>", LOCATION, LOCATION, hr, min % 60, sec % 60,status.batteryVoltage,status.doorState,status.heaterPower,status.lightLevelExterior,int(status.temperatureExterior),int(status.temperatureInterior));
  server.send ( 200, "text/html", temp );
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
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

  server.on ("/", handleRoot);
  server.on ("/inline", []() {
    server.send (200, "text/plain", "this works as well");
  } );
  server.onNotFound (handleNotFound);
  server.begin();
  timeoutPost = 0;
  jsonValid = 0;
  if (debug) Serial.println ("HTTP server started");
}

void post() {
  if (tsClient.connect(tsServer, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(status.uptime);
    postStr += "&field2=";
    postStr += String(status.batteryVoltage);
    postStr += "&field3=";
    postStr += String(status.doorState);
    postStr += "&field4=";
    postStr += String(status.heaterPower);
    postStr += "&field5=";
    postStr += String(status.lightLevelExterior);
    postStr += "&field6=";
    postStr += String(status.temperatureInterior);
    postStr += "&field7=";
    postStr += String(status.temperatureExterior);
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

  status.uptime = root["systemUptime"];
  status.batteryVoltage = root["batteryVoltage"];
  if (strstr(root["doorState"], "open")) {
    status.doorState = 1;
  } else {
    status.doorState = 0;
  }
  status.heaterPower = root["heaterPower"];
  status.lightLevelExterior = root["lightLevelExterior"];
  status.temperatureInterior = root["temperatureInterior"];
  status.temperatureExterior = root["temperatureExterior"];

  if (debug) {
    Serial.print("jsonProcess ");
    Serial.println(root.success());
  }
  jsonValid = 1;
}

void loop(void) {
  server.handleClient();
  if ((millis() > timeoutPost) && (jsonValid)) {
    post();
    timeoutPost = millis() + TIMEOUTPOST;
    jsonValid = 0;
  }
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

