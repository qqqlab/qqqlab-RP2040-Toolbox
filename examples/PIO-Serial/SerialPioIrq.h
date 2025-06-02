#pragma once

#include "UartTxPioIrq.h"
#include "UartRxPioIrq.h"

// Serial PIO IRQ class
class SerialPioIrq {
private:
  UartTxPioIrq tx;
  UartRxPioIrq rx; 
public:
  SerialPioIrq(uint8_t txpin, uint8_t rxpin, uint16_t txbuflen = 256, uint16_t rxbuflen = 256) : tx(txpin, txbuflen), rx(rxpin, rxbuflen) {}

  void begin(int baud) {
    tx.begin(baud);
    rx.begin(baud);
  }
  
  int available() {
    return rx.available();
  }

  int availableForWrite() {
    return tx.availableForWrite();
  }

  int read(uint8_t *buf, int len) {
    return rx.read(buf, len);
  }

  int write(uint8_t *buf, int len) {
    return tx.write(buf, len);
  }
};



