#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

WiFiUDP udp;

void setup() {
  // Turn on all the lights for a second when booting
  analogWrite(12, PWMRANGE);
  analogWrite(13, PWMRANGE);
  analogWrite(14, PWMRANGE);

  delay(1000); // very important bit for Access Point to work properly... ¯\_(ツ)_/¯

  analogWrite(12, 0);
  analogWrite(13, 0);
  analogWrite(14, 0);
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Hello OSC2Midi!");
  Serial.println();

  WiFi.softAP("Pretty Lights", "anyoneiswelcome");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  Serial.println();
  udp.begin(8000);
}

void OSCToPWM(OSCMessage &msg, int offset) {
  char address[100] = { 0 };
  uint16_t value;

  msg.getAddress(address, offset, sizeof(address));
  
  Serial.printf("Got address: %s\n", address);

  if (msg.size() != 1 || !msg.isFloat(0)) {
    Serial.printf("Unexpected message format. Should be single float value only.\n");
    return;
  }

  value = round(msg.getFloat(0) * PWMRANGE);
  Serial.printf("Got value: %i\n", value);

  if (strcmp(address, "/r") == 0) {
    analogWrite(12, value);
  } else if (strcmp(address, "/g") == 0) {
    analogWrite(13, value);
  } else if (strcmp(address, "/b") == 0) {
    analogWrite(14, value);
  } else {
    Serial.printf("Unexpected colour: %s\n", address);
  }
}


void loop() {
  OSCMessage msg;
  uint8_t buffer[1024];

  // Check if there are any OSC packets to handle
  size_t size = udp.parsePacket();
  if (size > 0) {
    udp.read(buffer, size);
    msg.fill(buffer, size);

    if (!msg.hasError()) {
      msg.route("/fade", OSCToPWM);
    } else {
      int error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}
