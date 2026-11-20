// Basic hardware revision check (no extra Arduino libraries required).
// Waveshare 1.8 AMOLED: V1 FT3168 at 0x38; V2 CST820 at 0x15.
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(15, 14);  // Waveshare touch/expander I2C pins
  delay(2000);
  Serial.println("Scanning Waveshare I2C bus...");
  bool v1 = false, v2 = false;
  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found 0x%02X\n", address);
      if (address == 0x15) v2 = true;
      if (address == 0x38) v1 = true;
    }
  }
  if (v2) Serial.println("Likely V2: CST820 at 0x15 -> use AIWatch sketch");
  else if (v1) Serial.println("Likely V1: FT3168 at 0x38 -> V2 sketch is NOT compatible");
  else Serial.println("No touch chip found; power on board / check serial and I2C");
}
void loop() { delay(1000); }
