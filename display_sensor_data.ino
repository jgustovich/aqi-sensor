
#include <SoftwareSerial.h>
#include <TFT_eSPI.h>       // Include the graphics library

TFT_eSPI tft = TFT_eSPI();  // Create object "tft" for text display

// Create a SoftwareSerial to read data from the sensor. The first parameter is the "receiver (rx) pin", the TTGO pin you connected to the Plantower's TXD pin.
// The second parameter is pin used to transmit data. This pin is unused, but required, so I just set it to the next pin on the TTGO. 
SoftwareSerial pmsSerial(21, 22);

// This struct is based on the Plantower's transport protocol. As the sensor runs, this data is transmitted from the Plantower via it's TXD (transmit) pin.
// You can find this information in Appendix I of http://m.eleparts.co.kr/data/goods_old/data/PMSA003.pdf. 
struct pms7003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms7003data data;
int successfulReads = 0;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  // Sets the speed of data transmission
  pmsSerial.begin(9600);
}

void loop() {
  if (checkSwSerial(&pmsSerial)) {
    tft.setTextSize(2);
    tft.setCursor(0,0);

    // PM labels
    tft.drawString("PM1.0", 10, 5);
    tft.drawString("PM2.5", 90, 5);
    tft.drawString("PM10", 170, 5);

    // Clear screen and render PM data
    tft.fillRect(0, 35, 240, 40, TFT_WHITE);
    tft.drawNumber(data.pm10_standard, 10, 35);
    tft.drawNumber(data.pm25_standard, 90, 35);
    tft.drawNumber(data.pm100_standard, 170, 35);
  }
}

boolean checkSwSerial(SoftwareSerial* ss) {
  setUpCursorForDebug();
  
  tft.println("Waiting for data to be available...");
  if (!ss->available()) {
    return false;
  }
  tft.println("Bytes are available...");

  // Read a byte at a time until we get to the special '0x42' start byte
  if (ss->peek() != 0x42) {
    ss->read();
    return false;
  }

  // Now read all 32 bytes of data once available
  tft.println("Waiting for 32 bytes of data...");
  if (ss->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  tft.println("Read data...");
  ss->readBytes(buffer, 32);

  // Compute checksum
  for (uint8_t i=0; i < 30; i++) {
    sum += buffer[i];
  }

  // The data comes in endian'd, so let's convert.
  // The +2 offset is to shave off the unneeded start bytes.
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += buffer[2 + i*2] << 8;
  }
  // Copy buffer into our struct
  memcpy((void *)&data, (void *)buffer_u16, 30);

  // Verify checksum
  if (sum != data.checksum) {
    tft.println("Checksum failure...");
    return false;
  }

  successfulReads++;
  tft.print("Successful read: ");
  tft.print(successfulReads);
  return true;
}

void setUpCursorForDebug() {
  tft.setTextSize(1);
  tft.setCursor(0,75);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
}
