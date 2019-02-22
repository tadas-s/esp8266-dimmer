#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <CRC32.h>
#include <EEPROM.h>

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
}

void SettingsInit() {
  uint32_t checksum;

  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, settings);

  checksum = CRC32::calculate(&settings + sizeof(uint32_t), sizeof(Settings) - sizeof(uint32_t));

  if (checksum != settings.crc) {
    Serial.printf("\nSettings not initialized: %du != %du\n", checksum, settings.crc);
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

void OSCToPWM(OSCMessage &msg, int offset) {
  char address[100] = { 0 };

  msg.getAddress(address, offset, sizeof(address));

  if (msg.size() != 1 || !msg.isFloat(0)) {
    Serial.printf("Unexpected message format. Should be single float value only.\n");
    return;
  }

  // WolframAlpha:
  // y(x)=(1/(1+e^((-11)*(x-0.5))) - 0.00407)*1.0078 from x=0 to 1
  // What WA solves for y(0)=0 and y(1)=1 needs to be adjusted slightly. Probs
  // because of floats precision ¯\_(ツ)_/¯
  float interm = 1.0 / (1.0 + exp(-12.0 * (msg.getFloat(0) - 0.5))) - 0.002;
  uint16_t pwm_value = round((interm) * 1.0043 * PWMRANGE);

  Serial.printf(
    "Address: %s\tValue: %f\t%f\tTranslated to: %d\n",
    address, msg.getFloat(0), interm, pwm_value
  );

  if (strcmp(address, "/r") == 0) {
    analogWrite(12, pwm_value);
  } else if (strcmp(address, "/g") == 0) {
    analogWrite(13, pwm_value);
  } else if (strcmp(address, "/b") == 0) {
    analogWrite(14, pwm_value);
  } else if (strcmp(address, "/w") == 0) {
    analogWrite(16, pwm_value);
  } else {
    Serial.printf("Unexpected colour: %s\n", address);
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
      msg.route("/settings", OSCToDeviceSettings);
    } else {
      int error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}
