/*
 * Reads and displays air quality data from a Plantower PMS7003 sensor on a TTGO ESP32
 * Author: Jarrett Gustovich
*/

#include <SoftwareSerial.h>
#include <TFT_eSPI.h>       // Include the graphics library
#include <time.h>   // Needed for clock

TFT_eSPI tft = TFT_eSPI();  // Create object "tft"
SoftwareSerial pmsSerial(21, 22);

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  pmsSerial.begin(9600);
  Serial.begin(115200);
}

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
boolean errorState = false;
clock_t lastRead = clock();

void loop()
{
  checkForErrorState();
  
  if (checkSwSerial(&pmsSerial)) {
    saveReadTimeAndResetErrorState();
    renderSuccessState();
    renderData();
    delay(1000);
  }
}

// --- READ AND RENDER PM DATA --- //

void renderData() {
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // PM labels
  tft.drawString("PM1.0", 10, 5);
  tft.drawString("PM2.5", 90, 5);
  tft.drawString("PM10", 170, 5);
  
  // Divider
  tft.fillRect(0, 25, 240, 3, TFT_DARKGREEN);

  // Clear screen and render PM data
  tft.fillRect(0, 35, 240, 40, TFT_WHITE);
  renderPM10(data.pm10_standard);
  renderPM25(data.pm25_standard);
  renderPM100(data.pm100_standard);
}

void renderPM10(uint16_t pm10) {
  tft.drawNumber(pm10, 10, 35);
}

void renderPM25(uint16_t pm25) {
  if (pm25 <= 12) {
    tft.fillRect(85, 30, 70, 25, TFT_GREEN); 
  } else if (pm25 <= 35) {
    tft.fillRect(85, 30, 70, 25, TFT_YELLOW);
  } else if (pm25 <= 55) {
    tft.fillRect(85, 30, 70, 25, TFT_ORANGE);
  } else if (pm25 <= 150) {
    tft.fillRect(85, 30, 70, 25, TFT_RED);
  } else if (pm25 <= 250) {
    tft.fillRect(85, 30, 70, 25, TFT_PURPLE);
  } else {
    tft.fillRect(85, 30, 70, 25, TFT_MAROON);
  }

  tft.drawNumber(pm25, 90, 35);
}

void renderPM100(uint16_t pm100) {
  uint16_t color;
  if (pm100 <= 54) {
    tft.fillRect(165, 30, 70, 25, TFT_GREEN); 
  } else if (pm100 <= 154) {
    tft.fillRect(165, 30, 70, 25, TFT_YELLOW);
  } else if (pm100 <= 254) {
    tft.fillRect(165, 30, 70, 25, TFT_ORANGE);
  } else if (pm100 <= 354) {
    tft.fillRect(165, 30, 70, 25, TFT_RED);
  } else if (pm100 <= 424) {
    tft.fillRect(165, 30, 70, 25, TFT_PURPLE);
  } else {
    tft.fillRect(165, 30, 70, 25, TFT_MAROON);
  }

  tft.drawNumber(pm100, 170, 35);
}

boolean checkSwSerial(SoftwareSerial* ss) {  
  Serial.println("Waiting for data to be available...");
  if (!ss->available()) {
    return false;
  }
  Serial.println("Bytes are available...");

  // Read a byte at a time until we get to the special '0x42' start byte
  if (ss->peek() != 0x42) {
    ss->read();
    return false;
  }

  // Now read all 32 bytes of data once available
  Serial.println("Waiting for 32 bytes of data...");
  if (ss->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  Serial.println("Read data...");
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
    Serial.println("Checksum failure...");
    return false;
  }

  successfulReads++;
  Serial.printf("Successful read: %d\n", successfulReads);
  return true;
}


// --- ERROR STATE --- //

void checkForErrorState() {
  // Calculate the time since a last successful read, and if > 3 sec display an error message.
  clock_t timeSinceLastSuccess = clock() - lastRead;
  int msecSinceLastSuccess = timeSinceLastSuccess * 1000 / CLOCKS_PER_SEC;
  if (!errorState && msecSinceLastSuccess > 3000) {
    errorState = true;
    tft.fillRect(5, 100, 100, 30, TFT_WHITE);
    tft.fillRoundRect(5, 100, 80, 30, 3, TFT_RED);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("ERROR", 15, 107);
  }
}

void saveReadTimeAndResetErrorState() {
  // Save last successful read time and reset error state
  lastRead = clock();
  errorState = false;
}

void renderSuccessState() {
  tft.fillRoundRect(5, 100, 100, 30, 3, TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_GREEN);
  
  uint16_t mod = successfulReads % 3;
  if (mod == 0) {
    tft.drawString("OKAY.", 15, 107);
  } else if (mod == 1) {
    tft.drawString("OKAY..", 15, 107);
  } else if (mod == 2) {
    tft.drawString("OKAY...", 15, 107);
  }
}
