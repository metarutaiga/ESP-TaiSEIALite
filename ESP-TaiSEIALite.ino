// ESP-TaiSEIALite 1.00
// Copyright 2022 taiga

#define VERSION         "1.00"

#define WIFI_SSID       "wifi"
#define WIFI_PASSWORD   "00000000"
#define WIFI_HOSTNAME   "TaiSEIA_Unknown"

#define MQTT_SERVER     "127.0.0.1"
#define MQTT_PORT       1883
#define MQTT_HVAC       0

#define NTP_SERVER      "time.google.com"
#define NTP_TIMEZONE    0

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <WiFiUdp.h>

#define messageSerial Serial1
#define taiseiaSerial Serial

#include "BasicOTA.h"
#include "MQTT_ESP8266.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_TIMEZONE * 3600);

bool forceRestart = false;
bool forceReset = false;
char number[128] = {};
char state = 0;

#include "TaiSEIA-Protocol.h"

int ascii_interger(char* payload, unsigned int length, bool hex = false) {
  char string[20];
  memcpy(string, payload, length);
  string[length] = '\0';
  return hex ? strtol(string, 0, 16) : atoi(string);
}

void MQTTcallback(char* topic, char* payload, unsigned int length) {
  const char* result = "succeed";

  if (strcmp(topic, MQTTprefix("set", "Restart", 0)) == 0) {
    forceRestart = true;
  } else if (strcmp(topic, MQTTprefix("set", "Reset", 0)) == 0) {
    forceReset = true;
  } else if (strstr(topic, "/set/Service") != 0) {
    char* service = strstr(topic, "/set/Service") + sizeof("/set/Service") - 1;
    int device = ascii_interger(service, 2, true);
    int index = ascii_interger(service + 2, 2, true);
    int value = ascii_interger(payload, length);
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
  char hostname[32] = WIFI_HOSTNAME;
  WiFi.macAddress(mac);
  sprintf(strchr(hostname, '_'), "_%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Serial
  messageSerial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  messageSerial.println();
  messageSerial.printf("%lu:%s starting", millis(), hostname);
  messageSerial.println();

  // WIFI
  WiFi.hostname(hostname);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    messageSerial.print(".");
  }
  WiFi.hostname(hostname);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  messageSerial.println();
  messageSerial.printf(" connected to %s, IP address: %s", WIFI_SSID, WiFi.localIP().toString().c_str());
  messageSerial.println();

  // OTA
  OTAsetup();

  // MQTT
  MQTTclient.setServer(MQTT_SERVER, MQTT_PORT);
  MQTTclient.setCallback((void(*)(char*, byte*, unsigned int))MQTTcallback);
  MQTTreconnect(false);

  // NTP
  timeClient.begin();

  // Serial
  taiseiaSerial.begin(9600, SERIAL_8N1, SERIAL_FULL);

  // Watchdog
  ESP.wdtEnable(2000);
}

void loop() {
  switch (state) {
  case 0:   // Initialize

    MQTTclient.publish(MQTTprefix("ESP", "Vcc", 0), itoa(ESP.getVcc(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "ChipId", 0), itoa(ESP.getChipId(), number, 10));

    MQTTclient.publish(MQTTprefix("ESP", "SdkVersion", 0), ESP.getSdkVersion());
    MQTTclient.publish(MQTTprefix("ESP", "CoreVersion", 0), ESP.getCoreVersion().c_str());
    MQTTclient.publish(MQTTprefix("ESP", "FullVersion", 0), ESP.getFullVersion().c_str());

    MQTTclient.publish(MQTTprefix("ESP", "BootVersion", 0), itoa(ESP.getBootVersion(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "BootMode", 0), itoa(ESP.getBootMode(), number, 10));

    MQTTclient.publish(MQTTprefix("ESP", "CpuFreq", 0), itoa(ESP.getCpuFreqMHz(), number, 10));

    MQTTclient.publish(MQTTprefix("ESP", "FlashChipId", 0), itoa(ESP.getFlashChipId(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "FlashChipVendorId", 0), itoa(ESP.getFlashChipVendorId(), number, 10));

    MQTTclient.publish(MQTTprefix("ESP", "FlashChipRealSize", 0), itoa(ESP.getFlashChipRealSize(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "FlashChipSize", 0), itoa(ESP.getFlashChipSize(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "FlashChipSpeed", 0), itoa(ESP.getFlashChipSpeed(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "FlashChipMode", 0), itoa(ESP.getFlashChipMode(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "FlashChipSizeByChipId", 0), itoa(ESP.getFlashChipSizeByChipId(), number, 10));

    MQTTclient.publish(MQTTprefix("ESP", "SketchSize", 0), itoa(ESP.getSketchSize(), number, 10));
    MQTTclient.publish(MQTTprefix("ESP", "SketchMD5", 0), ESP.getSketchMD5().c_str());
    MQTTclient.publish(MQTTprefix("ESP", "FreeSketchSpace", 0), itoa(ESP.getFreeSketchSpace(), number, 10));

    MQTTclient.publish(MQTTprefix("ESP", "ResetReason", 0), ESP.getResetReason().c_str());
    MQTTclient.publish(MQTTprefix("ESP", "ResetInfo", 0), ESP.getResetInfo().c_str());

    MQTTclient.publish(MQTTprefix("ESP", "Build", 0), __DATE__ " " __TIME__, true);
    MQTTclient.publish(MQTTprefix("ESP", "Version", 0), VERSION, true);

    state = 100;
  case 100: // Looping State
    MQTTclient.publish(MQTTprefix("state", 0), "looping");
    state = 101;
  case 101:

    static unsigned long loopHeapMillis = 0;
    if (loopHeapMillis < millis()) {
      loopHeapMillis = millis() + 1000 * 10;
 
      // Time
      int hours = timeClient.getHours();
      int minutes = timeClient.getMinutes();
      sprintf(number, "%02d:%02d", hours, minutes);
      MQTTclient.publish(MQTTprefix("ESP", "Time", 0), number);

      // Heap
      static int now_FreeHeap = 0;
      int freeHeap = ESP.getFreeHeap();
      if (now_FreeHeap != freeHeap) {
        now_FreeHeap = freeHeap;
        MQTTclient.publish(MQTTprefix("ESP", "FreeHeap", 0), itoa(now_FreeHeap, number, 10));
      }

      // RSSI
      static int now_RSSI = 0;
      int rssi = WiFi.RSSI();
      if (now_RSSI != rssi) {
        now_RSSI = rssi;
        MQTTclient.publish(MQTTprefix("ESP", "RSSI", 0), itoa(rssi, number, 10));
      } 
    }

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

  if (!MQTTclient.connected())
    MQTTreconnect(false);
  MQTTclient.loop();
  ArduinoOTA.handle();
  timeClient.update();

  delay(16);
  ESP.wdtFeed();
}
