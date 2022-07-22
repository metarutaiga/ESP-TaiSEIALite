// ESP-TaiSEIALite 1.06
// Copyright 2022 taiga

#define VERSION         "1.06"

#define WIFI_SSID       "wifi"
#define WIFI_PASSWORD   "00000000"
#define WIFI_HOSTNAME   "TaiSEIA_Unknown"

#define MQTT_SERVER     "127.0.0.1"
#define MQTT_PORT       1883
#define MQTT_HVAC       0

#define NTP_SERVER      "time.google.com"
#define NTP_TIMEZONE    0

char hostname[20] = WIFI_HOSTNAME;
bool forceRestart = false;
bool forceReset = false;
char number[128] = {};
char state = 0;

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>

#define messageSerial Serial
#define taiseiaRxSerial Serial
#define taiseiaTxSerial Serial1

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_TIMEZONE * 3600);

#include "ESP-Common/BasicOTA.h"
#include "ESP-Common/MQTT_ESP8266.h"
#include "TaiSEIA-Protocol.h"

long strntol(const char *str, size_t sz, char **end, int base) {
  char temp[20];
  memcpy(temp, str, sz);
  temp[sz] = '\0';
  return strtol(temp, end, base);
}

void MQTTcallback(char* topic, char* payload, unsigned int length) {
  const char* result = "succeed";

  if (strcmp(topic, MQTTprefix("set", "Restart", 0)) == 0) {
    forceRestart = true;
  } else if (strcmp(topic, MQTTprefix("set", "Reset", 0)) == 0) {
    forceReset = true;
  } else if (strstr(topic, "/set/Service") != 0) {
    char* service = strstr(topic, "/set/Service") + sizeof("/set/Service") - 1;
    int device = strntol(service, 2, 0, 16);
    int index = strntol(service + 2, 2, 0, 16);
    int value = strntol(payload, length, 0, 10);
    sendProtocol(0x06, device, index | 0x80, (value >> 8) & 0xff, value & 0xff, 0x00);
  } else {
    result = "unknown";
  }

  if (strcmp(topic, MQTTprefix("set", "result", 0)) != 0) {
    MQTTclient.publish(MQTTprefix("set", "result", 0), result);
  }
}

void setup() {

  // MAC
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  sprintf(strchr(hostname, '_'), "_%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Serial
  messageSerial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  messageSerial.println();
  messageSerial.printf("%lu:%s starting", millis(), hostname);
  messageSerial.println();

  // WIFI
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(hostname);

  // OTA
  OTAsetup(hostname);

  // MQTT
  MQTTclient.setServer(MQTT_SERVER, MQTT_PORT);
  MQTTclient.setCallback((void(*)(char*, byte*, unsigned int))MQTTcallback);
  MQTTreconnect(hostname, false);

  // NTP
  timeClient.begin();

  // Serial
  taiseiaRxSerial.begin(9600, SERIAL_8N1, SERIAL_FULL);
  taiseiaRxSerial.swap();
  taiseiaTxSerial.begin(9600, SERIAL_8N1, SERIAL_FULL);

  // Watchdog
  ESP.wdtEnable(2000);
}

void loop() {

  // Loop
  switch (state) {
  case 0:   // Initialize
    state = 100;
  case 100: // Looping State
    MQTTclient.publish(MQTTprefix("state", 0), "looping");
    state = 101;
  case 101:
    break;
  default:
    break;
  }

  // ForceRestart
  if (forceRestart) {
    state = 0;
  }

  // ForceReset
  if (forceReset) {
    ESP.reset();
  }

  // WiFi
  static byte wifiStatus = 0;
  if (wifiStatus != WiFi.status()) {
    wifiStatus = WiFi.status();
    messageSerial.println();
    messageSerial.printf(" connected to %s, IP address: %s", WIFI_SSID, WiFi.localIP().toString().c_str());
    messageSerial.println();
  }

  // Update
  MQTTupdate();
  if (!MQTTclient.connected()) {
    MQTTreconnect(hostname, false);
  }
  MQTTclient.loop();
  ArduinoOTA.handle();
  timeClient.update();

  ESP.wdtFeed();
  delay(100);
}
