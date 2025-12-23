// SerialDMA high speed test

// Connect pin 0 to pin 1

// max baud rates:
// RP2040 @ 133MHz: 3,000,000
// RP2040 @ 200MHz: 3,000,000
// RP2350 @ 150MHz: 9,375,000 (=150000000/16)

#include <RP2040_SerialDMA.h>

#define BUF_SIZE 256

SerialDMA serDma;

uint8_t b_last = 0;
uint8_t b = 1;
int wlen = 0;
int rlen = 0;
int err = 0;
uint32_t actual_baud = 0;

uint32_t ts = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  actual_baud = serDma.begin(0, 100000000, 0, 1, BUF_SIZE, BUF_SIZE); //uint8_t uart_num, uint32_t baudrate, int8_t txpin, int8_t rxpin, uint16_t txbuflen, uint16_t rxbuflen

  ts = micros();
}

void loop() {
  uint8_t buf[32];
  for(uint i = 0; i < sizeof(buf); i++) {
    buf[i] = b++;
  }
  wlen += serDma.write(buf, sizeof(buf));

  int n = serDma.read(buf, sizeof(buf));
  rlen += n;

  if(micros() - ts >= 1000000) {
    Serial.printf("write:%d bytes/sec\tread:%d b/s\t", wlen, rlen);
    Serial.printf("errors:%d\t", err);
    Serial.printf("actual_baud:%d\t", actual_baud);
    Serial.println();
    ts += 1000000;
    wlen = 0;
    rlen = 0;
  }
}
