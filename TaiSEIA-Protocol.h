#pragma once

void sendProtocol(byte length, ...) {
  while (taiseiaRxSerial.available() > 0)
    taiseiaRxSerial.read();

  // Start to send protocol
  byte protocol[256];
  if (length == 0)
    return;

  va_list args;
  va_start(args, length);
  byte checksum = length;
  protocol[0] = length;
  for (int i = 2; i < length; ++i) {
    byte code = va_arg(args, int);
    protocol[i - 1] = code;
    checksum ^= code;
  }
  protocol[length - 1] = checksum;
  va_end(args);  

  // Send protocol to MQTT
  MQTTclient.beginPublish(MQTTprefix("TaiSEIA", "Send", 0), length * 3, false);
  for (int i = 0; i < length; ++i) {
    sprintf(number, "%02X.", protocol[i]);
    MQTTclient.write(number[0]);
    MQTTclient.write(number[1]);
    MQTTclient.write(number[2]);
  }
  MQTTclient.endPublish();

  // Send
  for (int i = 0; i < length; ++i) {
    taiseiaTxSerial.write(protocol[i]);
  }
}
