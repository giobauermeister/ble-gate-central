#include <Arduino.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FFat.h>
#include <ArduinoJson.h>

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
void handleRoot();
void startConfigWebpage();

const char *wifiSsid = "xxxx";
const char *wifiPassword = "xxxx";
const char *apSsid = " ESP32 BLE Gate";
const char *apPassword = "";

uint8_t configButton = 23;
uint8_t builtinLed = 5;
bool keepConfigWegpage;

uint8_t wifiStatus;
unsigned long connectTimeout;

String deviceIndex;
String deviceId;

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
  switch (type)
  {
    case WStype_DISCONNECTED:
    {
      Serial.printf("[%u] Disconnected!\n", num); 
    }    
    break;
    case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      size_t len;

      DynamicJsonDocument gateInfo(JSON_ARRAY_SIZE(500));
      gateInfo["gateInfo"]["macAddress"] = WiFi.macAddress();
      len = measureJson(gateInfo);
      char jsonToSend[len];
      serializeJson(gateInfo, Serial);
      serializeJson(gateInfo, jsonToSend, len + 1);
      webSocket.sendTXT(num, jsonToSend, strlen(jsonToSend));
    }
    break;
    case WStype_TEXT:
    {
      Serial.printf("[%u] get Text: %s\n", num, payload);

      StaticJsonDocument<512> jsonBuffer;
      deserializeJson(jsonBuffer, payload);
      JsonObject jsonObject = jsonBuffer.as<JsonObject>();

      if(strcmp((const char*)payload, "ping") == 0)
      {
        webSocket.sendTXT(num, "{\"pong\":true}");
      }

      if(strcmp((const char*)payload, "deviceInfo") == 0)
      {
        //webSocket.sendTXT(num, "{\"pong\":true}");
        //{"deviceInfo":{"deviceIndex":0,"deviceId":"123"}}
        JsonVariant _deviceIndex = jsonObject["deviceInfo"]["deviceIndex"];
        JsonVariant _deviceId = jsonObject["deviceInfo"]["deviceId"];
        deviceIndex = _deviceIndex.as<String>();
        deviceId = _deviceId.as<String>();

        int len = measureJson(jsonObject);
        char buff[len];
        serializeJson(jsonObject, buff, len + 1);
        File file = FFat.open("/deviceIdDB.txt", "w");
        if(!file) {
          Serial.println("device id file open error");
          return;
        }
        if(file.print(buff)){
          Serial.println("device id file was written");
          Serial.println(buff);
          String savedDeviceId = "{\"savedId\":{\"deviceIndex\":\"" + deviceIndex + "\",\"deviceId\":\"" + deviceId + "\"}}";
          Serial.println(savedDeviceId);
          webSocket.sendTXT(num, savedDeviceId);
        } else {
          Serial.println("device id file failed");
        }
        file.close();
        //blankDevice = false;
        
      }
    } 
    break;
    case WStype_BIN: 
    case WStype_PING:
    case WStype_PONG:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    break;
  }
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

// void startConfigWebpage()
// {
//   Serial.println("Starting AP for configuration");
//   keepConfigWegpage = true;
//   WiFi.mode(WIFI_AP_STA);
//   //WiFi.disconnect();
//   WiFi.softAP(apSsid, apPassword);

//   Serial.print("IP: ");
//   Serial.println(WiFi.softAPIP());

//   Serial.println("Starting Websocket loop");
//   while(keepConfigWegpage)
//   {
//     webSocket.loop();
//     server.handleClient();
//   }  
// }

void startConfigWebpage()
{
  Serial.println("Starting AP for configuration");
  keepConfigWegpage = true;
  WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  WiFi.begin(wifiSsid, wifiPassword);

  connectTimeout = millis() + (15 * 1000);
  while(true) {
    wifiStatus = WiFi.status();

    if ((wifiStatus == WL_CONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL) ||
    (wifiStatus == WL_CONNECT_FAILED) || (millis() >= connectTimeout)) 
      break;

    delay(100);
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting Websocket loop");
  while(keepConfigWegpage)
  {
    webSocket.loop();
    server.handleClient();
  }  
}

