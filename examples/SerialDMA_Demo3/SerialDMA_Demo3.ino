//connect pin 0 to pin 1

#include <RP2040_SerialDMA.h>

#define BUF_SIZE 256
#define BAUD 1000000

SerialDMA serDma;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  serDma.begin(0, BAUD, 0, 1, BUF_SIZE, BUF_SIZE); //uint8_t uart_num, uint32_t baudrate, int8_t txpin, int8_t rxpin, uint16_t txbuflen, uint16_t rxbuflen
}

int iter = 0;
uint8_t buf[BUF_SIZE];
int err_cnt = 0;

void loop() {
  int wlen = 0;
  String s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678-SerialDMA-" + String(iter);

  int uart_us = 1000000 / (BAUD / 10) * s.length(); //duration to send full packet over uart (in us)
  int delay_us = iter % (uart_us/2); //delay between the two halves (in us)

  // write string in two halves
  uint32_t dt1 = micros();
  int len1 = s.length() / 2;
  int avail0 = serDma.availableForWrite();
  wlen = serDma.write((uint8_t*)s.c_str(), len1);
  delayMicroseconds(delay_us);
  int avail1 = serDma.availableForWrite();
  wlen += serDma.write(((uint8_t*)s.c_str()) + len1, s.length() - len1);
  int avail2 = serDma.availableForWrite();
  dt1 = micros() - dt1;

  //wait long enough to transmit all data incl delay_us
  delayMicroseconds(uart_us * 2);

  uint32_t dt2 = micros();
  uint16_t rlen = serDma.read(buf, sizeof(buf));
  dt2 = micros() - dt2;

  buf[rlen] = 0;
  String s2 = String((char*)buf);
  bool err = (s != s2);

  if(iter%100==0 || err) {
    if(err) {
      Serial.printf("ERROR    ");
      err_cnt++;
    }else{
      Serial.printf("OK       ");
    }
    Serial.printf("iter:%d ", (int)iter);
    Serial.printf("err_cnt:%d ", (int)err_cnt);
    Serial.printf("uart_us:%d ", (int)uart_us);
    Serial.printf("delay_us:%d ", (int)delay_us);
    Serial.printf("wlen:%d ", (int)wlen);
    Serial.printf("rlen:%d ", (int)rlen);
    //note: if txbuf1 > 0 then first dma transfer was aborted+restarted
    Serial.printf("txbuf:%d %d %d ",BUF_SIZE-1-avail0,BUF_SIZE-1-avail1,BUF_SIZE-1-avail2);
    //Serial.printf("tx:%dus ", (int)dt1);
    //Serial.printf("rx:%dus ", (int)dt2);
    Serial.print("RX:'");
    Serial.write(buf, rlen);
    Serial.printf("'");
    Serial.println();
  }

  iter++;
}
