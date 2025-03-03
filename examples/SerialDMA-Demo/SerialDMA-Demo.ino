//connect pin 0 to pin 1

#include <RP2040_SerialDMA.h>

SerialDMA serDma;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  serDma.begin(0, 9600, 0, 1, 64, 64); //uint8_t uart_num, uint32_t baudrate, int8_t txpin, int8_t rxpin, uint16_t txbuflen, uint16_t rxbuflen
}

int iter = 0;
uint8_t buf[64];

void loop() {
  iter++;
  String s = "SerialDMA-" + String(iter);
  uint32_t dt1 = micros();
  if(iter%2) {
    // write whole string
    serDma.write((uint8_t*)s.c_str(), s.length());
  }else{
    // write each character
    for(uint16_t i=0;i< s.length();i++) {
      serDma.write((uint8_t*)&s.c_str()[i], 1);
    }
  }
  dt1 = micros() - dt1;

  delay(1000);

  uint32_t dt2 = micros();
  uint16_t num_bytes = serDma.read(buf, sizeof(buf));
  dt2 = micros() - dt2;

  Serial.print("RX:'");
  Serial.write(buf, num_bytes);
  Serial.printf("' iter:%d dt1:%dus dt2:%dus\n", iter, (int)dt1, (int)dt2);
}
