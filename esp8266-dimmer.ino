#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

WiFiUDP udp;

void setup() {
  // Turn on all the lights for a second when booting
  analogWrite(12, PWMRANGE);
  delay(100);
  analogWrite(13, PWMRANGE);
  delay(100);
  analogWrite(14, PWMRANGE);
  delay(100);
  analogWrite(16, PWMRANGE);
  delay(100);
  analogWrite(12, 0);
  delay(100);
  analogWrite(13, 0);
  delay(100);
  analogWrite(14, 0);
  delay(100);
  analogWrite(16, 0);
  delay(100);
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Hello pretty lights!");
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
  
  if (msg.size() != 1 || !msg.isFloat(0)) {
    Serial.printf("Unexpected message format. Should be single float value only.\n");
    return;
  }

  // WolframAlpha:
  // plot y(x)=1/(1+e^((-12)*(x-0.5))) from x=0 to 1
  
  value = round((1.0 / (1.0 + exp(-12.0 * (msg.getFloat(0) - 0.5)))) * PWMRANGE);

  // special values because I'm bad at math:
  if (msg.getFloat(0) == 0.0) {
    value = 0;
  } else if (msg.getFloat(0) >= 1.0) {
    value = PWMRANGE;
  }

  Serial.printf("Address: %s\tValue: %f\tTranslated to: %d\n", address, msg.getFloat(0), value);

  if (strcmp(address, "/r") == 0) {
    analogWrite(12, value);
  } else if (strcmp(address, "/g") == 0) {
    analogWrite(13, value);
  } else if (strcmp(address, "/b") == 0) {
    analogWrite(14, value);
  } else if (strcmp(address, "/w") == 0) {
    analogWrite(16, value);
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
