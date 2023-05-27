#include <SPI.h>
#include <RFID.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define SS_PIN D2
#define RST_PIN D1
#define LED_SWITCH_PIN D8
#define LED_IN_STOCK_PIN D3
#define LED_OUT_STOCK_PIN D4

const char* ssid = "tongog";
const char* password = "0917766891";
String server = "http://34.143.224.3";
const int port = 443;

RFID rfid(SS_PIN, RST_PIN);
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.init();

  pinMode(LED_SWITCH_PIN, INPUT);
  pinMode(LED_IN_STOCK_PIN, OUTPUT);
  pinMode(LED_OUT_STOCK_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

int mode = 0;   // 0 = not found || 1 = is found || 2 = on hold
int state = 0;  // 0 = IN_STOCK || 1 = OUT_STOCK
int code = 0;

int current_state_switch = 0;
int last_state_switch = 0;

void loop() {
  int current_state_switch = digitalRead(LED_SWITCH_PIN);

  if (current_state_switch == 1 && last_state_switch == 0) {
    if (state) state = 0;
    else state = 1;
  }

  digitalWrite(LED_IN_STOCK_PIN, !state);
  digitalWrite(LED_OUT_STOCK_PIN, state);

  if (rfid.isCard() && rfid.readCardSerial() && mode == 0) {
    mode = 1;
    code = encode(rfid.serNum[0], rfid.serNum[1], rfid.serNum[2], rfid.serNum[3], rfid.serNum[4]);
  }

  if (mode == 1) {
    Serial.print("Read product code=");
    Serial.println(code);
    if (state) {
      digitalWrite(LED_IN_STOCK_PIN, LOW);
      digitalWrite(LED_OUT_STOCK_PIN, LOW);
      sendData(String(code), "OUT_STOCK");
    } else {
      digitalWrite(LED_IN_STOCK_PIN, LOW);
      digitalWrite(LED_OUT_STOCK_PIN, LOW);
      sendData(String(code), "IN_STOCK");
    }
    mode = 2;
  }

  if (!rfid.readCardSerial()) {
    mode = 0;
  }

  rfid.halt();

  last_state_switch = current_state_switch;
}

void sendData(String id, String status) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, server + "/esp");

  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST("{\"id\": \"" + id + "\",\"status\": \"" + status + "\"}");

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode != 200) {
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_IN_STOCK_PIN, HIGH);
      digitalWrite(LED_OUT_STOCK_PIN, HIGH);
      delay(250);
      digitalWrite(LED_IN_STOCK_PIN, LOW);
      digitalWrite(LED_OUT_STOCK_PIN, LOW);
      delay(250);
    }
  }
  http.end();
}

int encode(int a, int b, int c, int d, int e) {
  int result = a;
  result = result * 100 + b;
  result = result * 100 + c;
  result = result * 100 + d;
  result = result * 100 + e;
  return result;
}