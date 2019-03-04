#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <CRC32.h>
#include <EEPROM.h>
#include <PacketSerial.h>

struct Settings {
  /*
    CRC for settings. So it's possible to detect un-initialized settings.
  */
  uint32_t crc;

  /*
    Deviced ID from 0 to 255. In reality there will be less IDs usable since
    devices won't fit into the subnet. Unless it's possible to have UDP broadcasts
    in wider than 8 bit subnets?
  */
  uint8_t id;
};

WiFiUDP udp;
WiFiManager wifiManager;
Settings settings;

PacketSerial packetSerial;
void onPacketReceived(const uint8_t* buffer, size_t size);

void setup() {
  analogWrite(12, 0);
  analogWrite(13, 0);
  analogWrite(14, 0);
  analogWrite(16, 0);

  Serial.begin(115200);
  Serial.printf("\nHello pretty lights!\n");

  WiFi.hostname("prettylights");
  wifiManager.autoConnect("Pretty Lights");

  udp.begin(8000);

  // Turn on all the lights for a second when booting
  analogWrite(12, PWMRANGE);
  delay(250);
  analogWrite(12, 0);

  analogWrite(13, PWMRANGE);
  delay(250);
  analogWrite(13, 0);

  analogWrite(14, PWMRANGE);
  delay(250);
  analogWrite(14, 0);

  analogWrite(16, PWMRANGE);
  delay(250);
  analogWrite(16, 0);

  SettingsInit();

  packetSerial.begin(115200);
  packetSerial.setPacketHandler(&onPacketReceived);
}

void SettingsInit() {
  uint32_t checksum;

  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, settings);

  checksum = CRC32::calculate(&settings + sizeof(uint32_t), sizeof(Settings) - sizeof(uint32_t));

  if (checksum != settings.crc) {
    Serial.printf("\nSettings not initialized. Must be first boot after new firmware, eh?\n");
    memset(&settings, 0, sizeof(Settings));
    SettingsUpdate();
  }

  Serial.printf("my id: %i\n", settings.id);
}

void SettingsUpdate() {
  settings.crc = CRC32::calculate(&settings + sizeof(uint32_t), sizeof(Settings) - sizeof(uint32_t));
  EEPROM.put(0, settings);
  EEPROM.commit();
}

uint16_t floatToCorrectedPWM(float value) {
  // WolframAlpha:
  // y(x)=(1/(1+e^((-11)*(x-0.5))) - 0.00407)*1.0078 from x=0 to 1
  // What WA solves for y(0)=0 and y(1)=1 needs to be adjusted slightly. Probs
  // because of floats precision ¯\_(ツ)_/¯
  float interm = 1.0 / (1.0 + exp(-12.0 * (value - 0.5))) - 0.002;
  return round((interm) * 1.0043 * PWMRANGE);
}

void OSCToPWM(OSCMessage &msg, int offset) {
  char address[100] = { 0 };

  msg.getAddress(address, offset, sizeof(address));

  if (msg.size() == 1 && msg.isFloat(0)) {
    if (strcmp(address, "/r") == 0) {
      analogWrite(12, floatToCorrectedPWM(msg.getFloat(0)));
    } else if (strcmp(address, "/g") == 0) {
      analogWrite(13, floatToCorrectedPWM(msg.getFloat(0)));
    } else if (strcmp(address, "/b") == 0) {
      analogWrite(14, floatToCorrectedPWM(msg.getFloat(0)));
    } else if (strcmp(address, "/w") == 0) {
      analogWrite(16, floatToCorrectedPWM(msg.getFloat(0)));
    } else {
      Serial.printf("Unexpected colour: %s\n", address);
    }
  } else if (msg.size() == 4 && msg.isFloat(0) && msg.isFloat(1) && msg.isFloat(2) && msg.isFloat(3)) {
    if (strcmp(address, "/rgbw") == 0) {
      analogWrite(12, floatToCorrectedPWM(msg.getFloat(0)));
      analogWrite(13, floatToCorrectedPWM(msg.getFloat(1)));
      analogWrite(14, floatToCorrectedPWM(msg.getFloat(2)));
      analogWrite(16, floatToCorrectedPWM(msg.getFloat(3)));
    }
  }
}

void OSCToDeviceSettings(OSCMessage &msg, int offset) {
  char address[100] = { 0 };

  msg.getAddress(address, offset, sizeof(address));

  if (msg.size() == 1 && msg.isInt(0) && strcmp(address, "/id") == 0) {
    uint32_t id = msg.getInt(0);

    if (id < 255) {
      settings.id = (uint8_t)id;
      SettingsUpdate();
      Serial.printf("Setting device id to: %i\n", id);
    } else {
      Serial.printf("Deviced id out of range: %i\n", id);
    }
  } else {
    Serial.printf("Unexpected settings message address / format\n");
    return;
  }
}

void onPacketReceived(const uint8_t* buffer, size_t size)
{
    if (size < 1) {
      return;
    }

    // 3 -> request to get device id
    if (size == 1 && buffer[0] == 3) {
      uint8_t response[2];
      response[0] = 5;
      response[1] = settings.id;
      packetSerial.send(response, 2);
    }

    // 4 -> request to set new device id
    if (size == 2 && buffer[0] == 4 && settings.id != buffer[1]) {
      uint8_t response[1];
      response[0] = 1; // acknowledge message
      settings.id = buffer[1];
      SettingsUpdate();
      packetSerial.send(response, 1);
    }

    // 4 -> request to set decvice id, but it's already set to the same id
    if (size == 2 && buffer[0] == 4 && settings.id == buffer[1]) {
      uint8_t response[1];
      response[0] = 1; // acknowledge message
      packetSerial.send(response, 1);
    }
}

void loop() {
  OSCMessage msg;
  uint8_t buffer[1024];
  char selfAddress[10];

  sprintf(selfAddress, "/%d", settings.id);

  // Check if there are any OSC packets to handle
  size_t size = udp.parsePacket();
  if (size > 0) {
    udp.read(buffer, size);
    msg.fill(buffer, size);

    if (!msg.hasError()) {
      msg.route(selfAddress, OSCToPWM);
      msg.route("/all", OSCToPWM);
      msg.route("/settings", OSCToDeviceSettings);
    } else {
      int error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }

  packetSerial.update();
}
