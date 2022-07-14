#pragma once

#include <ESP8266WiFi.h>
#include <PubSubClient.h> // see https://github.com/knolleary/pubsubclient

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

const char* MQTTprefix(const char* prefix, ...) {
  static char path[128];
  char* pointer = path;
  pointer += sprintf(pointer, "%s/%s", WiFi.hostname().c_str(), prefix);

  va_list args;
  va_start(args, prefix);
  while (const char* name = va_arg(args, char*)) {
    pointer += sprintf(pointer, "/%s", name);
  }
  va_end(args);

  return path;
}

void MQTTinformation() {
  MQTTclient.publish(MQTTprefix("ESP", "Vcc", 0), itoa(ESP.getVcc(), number, 10));
  MQTTclient.publish(MQTTprefix("ESP", "ChipId", 0), itoa(ESP.getChipId(), number, 16));

  MQTTclient.publish(MQTTprefix("ESP", "SdkVersion", 0), ESP.getSdkVersion());
  MQTTclient.publish(MQTTprefix("ESP", "CoreVersion", 0), ESP.getCoreVersion().c_str());
  MQTTclient.publish(MQTTprefix("ESP", "FullVersion", 0), ESP.getFullVersion().c_str());

  MQTTclient.publish(MQTTprefix("ESP", "BootVersion", 0), itoa(ESP.getBootVersion(), number, 10));
  MQTTclient.publish(MQTTprefix("ESP", "BootMode", 0), itoa(ESP.getBootMode(), number, 10));

  MQTTclient.publish(MQTTprefix("ESP", "CpuFreq", 0), itoa(ESP.getCpuFreqMHz(), number, 10));

  MQTTclient.publish(MQTTprefix("ESP", "FlashChipId", 0), itoa(ESP.getFlashChipId(), number, 16));
  MQTTclient.publish(MQTTprefix("ESP", "FlashChipVendorId", 0), itoa(ESP.getFlashChipVendorId(), number, 16));

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
}

void MQTTupdate() {
  static unsigned long loopHeapMillis = 0;
  if (loopHeapMillis < millis()) {
    loopHeapMillis = millis() + 1000 * 10;

    // Time
    static int now_Minutes = -1;
    int minutes = timeClient.getMinutes();
    if (now_Minutes != minutes) {
      now_Minutes = minutes;
      int hours = timeClient.getHours();
      sprintf(number, "%02d:%02d", hours, minutes);
      MQTTclient.publish(MQTTprefix("ESP", "Time", 0), number);
    }

    // Heap
    static int now_FreeHeap = -1;
    int freeHeap = ESP.getFreeHeap();
    if (now_FreeHeap != freeHeap) {
      now_FreeHeap = freeHeap;
      MQTTclient.publish(MQTTprefix("ESP", "FreeHeap", 0), itoa(now_FreeHeap, number, 10));
    }

    // RSSI
    static int now_RSSI = -1;
    int rssi = WiFi.RSSI();
    if (now_RSSI != rssi) {
      now_RSSI = rssi;
      MQTTclient.publish(MQTTprefix("ESP", "RSSI", 0), itoa(rssi, number, 10));
    } 
  }
}

// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
void MQTTreconnect(const char* hostname, bool wait) {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    messageSerial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (MQTTclient.connect(hostname, MQTTprefix("connected", 0), 0, true, "false")) {
      messageSerial.println("connected");
      MQTTclient.publish(MQTTprefix("connected", 0), "true", true);
      MQTTclient.publish(MQTTprefix("ESP", "IP", 0), WiFi.localIP().toString().c_str(), true);
      MQTTclient.subscribe(MQTTprefix("set", "#", 0));
      MQTTclient.setBufferSize(512);
      MQTTinformation();
    }
    else {
      messageSerial.print("failed, rc=");
      messageSerial.print(MQTTclient.state());
      if (wait == false) {
        messageSerial.println();
        break;
      }
      messageSerial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for (int i = 0; i < 5; ++i) {
        delay(1000);
        ArduinoOTA.handle();
      }
    }
  }
}
