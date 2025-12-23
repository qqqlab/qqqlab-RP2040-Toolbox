//connect pin 0 to pin 1

#include <RP2040_SerialDMA.h>

#define BUF_SIZE 256

SerialDMA serDma;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  serDma.begin(0, 9600, 0, 1, BUF_SIZE, BUF_SIZE); //uint8_t uart_num, uint32_t baudrate, int8_t txpin, int8_t rxpin, uint16_t txbuflen, uint16_t rxbuflen
}

int iter = 0;
uint8_t buf[BUF_SIZE];

void loop() {
  int wlen = 0;
  iter++;
  String s = "123456789012345678901234567890123456789012345678901234567890-SerialDMA-" + String(iter);
  uint32_t dt1 = micros();
  if(iter%2) {
    // write whole string
    wlen = serDma.write((uint8_t*)s.c_str(), s.length());
  }else{
    // write each character
    wlen = 0;
    for(uint16_t i=0;i< s.length();i++) {
      wlen += serDma.write((uint8_t*)&s.c_str()[i], 1);
    }
  }
  dt1 = micros() - dt1;

  delay(1000);

  uint32_t dt2 = micros();
  uint16_t num_bytes = serDma.read(buf, sizeof(buf));
  dt2 = micros() - dt2;

  Serial.printf("iter:%d\t", (int)iter);
  Serial.printf("wlen:%d\t", (int)wlen);
  Serial.printf("tx:%dus\t", (int)dt1);
  Serial.printf("rx:%dus\t", (int)dt2);
  Serial.print("RX:'");
  Serial.write(buf, num_bytes);
  Serial.printf("'\t");
  Serial.println();
}
