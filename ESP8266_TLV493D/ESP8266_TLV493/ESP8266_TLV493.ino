#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <TLV493D.h>
#include "Wire.h"
#include "Page_mem.h"
#include "jQueryRotate.js"
#include "Circle_Logo.svg"

TLV493D sensor1;

const int sensor1_pwr_pin = 13;
const int i2c_sda = 4;
const char* ssid = "your_ssid";
const char* password = "your_password";
const char* host = "your_host_name";

uint16_t x = 0;
uint8_t num;

String WStext_old;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void handleRoot() {
  //Serial.println(sensor1.update());
  server.send(200, "text/plain", String(sensor1.m_dPhi_xy));

}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);


        // send message to client

        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      // send message to client

      // webSocket.sendTXT(num, "message here");



      // send data to all connected clients

      // webSocket.broadcastTXT("message here");

      break;

    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
      // send message to client

      // webSocket.sendBIN(num, payload, length);
      break;

  }
}

void setup()
{
  Serial.begin(74880);
  //WIFI INIT
  Serial.printf("Connecting to %s\n", ssid);

  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("");
  pinMode(sensor1_pwr_pin, OUTPUT);
  //pinMode(sensor2_pwr_pin, OUTPUT);
  pinMode(i2c_sda, OUTPUT);

  digitalWrite(sensor1_pwr_pin, HIGH);
  //digitalWrite(sensor2_pwr_pin, LOW);
  digitalWrite(i2c_sda, LOW);

  delay(500);

  //init sensor1
  digitalWrite(sensor1_pwr_pin, LOW);
  digitalWrite(i2c_sda, LOW); //0x1F
  Serial.println("Starting sensor 1");
  delay(500);

  Wire.begin(); // Begin I2C wire communication

  //initialize sensor 1
  Serial.print("Initializing sensor 1: 0x");
  Serial.println(sensor1.init(LOW), HEX);
  server.on("/inline", handleRoot);

  server.on("/", []() {
    server.send(200, "text/html", MAIN_page);
  });

  server.on("/jQueryRotate.js", []() {
    server.send(200, "application/javascript", jQueryRotate);
  });
  server.on("/Circle_Logo.svg", []() {
    server.send(200, "image/svg+xml", Circle_Logo);
  });
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();

  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  /*if (x > 30000) {
    //Serial.print("sensor1: 0x");
    Serial.println(sensor1.update());
    Serial.print("dBx: ");
    Serial.print(sensor1.m_dBx);
    Serial.print(";");//\t");
    Serial.print("dBy: ");
    Serial.print(sensor1.m_dBy);
    Serial.print(";");//\t");
    //Serial.print("dBz: "); m,.
    x++;*/
  if (sensor1.update() == 0 && x > 10) {
    String WStext = String(sensor1.m_dPhi_xy);
    char const *WS = WStext.c_str();
    if (WStext != WStext_old) {
      if (!webSocket.sendTXT(num, WS)) {
        Serial.print(":");
      }
    }
    x = 0;
  }
  x++;
  server.handleClient();
  webSocket.loop();
}
