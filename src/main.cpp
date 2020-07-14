#include <Arduino.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FFat.h>

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
void handleRoot();
void startConfigWebpage();

const char *apSsid = "ESP32 IoT";
const char *apPassword = "";

uint8_t configButton = 23;
uint8_t builtinLed = 5;
bool keepConfigWegpage;

unsigned long connectTimeout;

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

void setup() {
  Serial.begin(115200);
  Serial.println("OK");

  pinMode(configButton, INPUT);
  pinMode(builtinLed, OUTPUT);

  if (!FFat.begin(true)){
    Serial.println("Couldn't mount the filesystem.");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  startConfigWebpage();
}

void loop() {
  // put your main code here, to run repeatedly:
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  
}

void handleRoot()
{
  Serial.println("HandleRoot");
  SPIFFS.begin();
  File file = SPIFFS.open("/index.html.gz", "r");
  server.streamFile(file, "text/html");
  file.close();
  SPIFFS.end();
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found");
}

void startConfigWebpage()
{
  Serial.println("Starting AP for configuration");
  keepConfigWegpage = true;
  WiFi.mode(WIFI_AP_STA);
  //WiFi.disconnect();
  WiFi.softAP(apSsid, apPassword);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Starting Websocket loop");
  while(keepConfigWegpage)
  {
    webSocket.loop();
    server.handleClient();
  }  
}