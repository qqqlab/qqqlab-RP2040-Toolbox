#include "SerialPioIrq.h"

#define PIO_TX_PIN 16 // connect to pin 1 (Serial1 RX)
#define PIO_RX_PIN 17 // connect to pin 0 (Serial1 TX)
#define SERIAL_BAUD 115200

int cnt = 0;

SerialPioIrq SerialPio(PIO_TX_PIN, PIO_RX_PIN, 256, 256);

void setup() {
    //Console Serial
    Serial.begin(115200);
    //while(!Serial);

    //PIO Serial
    SerialPio.begin(SERIAL_BAUD);

    //Harware Serial
    Serial1.setFIFOSize(256);
    Serial1.setTimeout(0);
    Serial1.begin(SERIAL_BAUD);
}

void loop() {
    cnt++;
    String s = String(cnt) + ":Hello, world! ABCDEFGHIJKLMNOPQRSTUVWXYZ+";

    //send strings
    Serial1.write((const uint8_t*)s.c_str(), s.length());
    SerialPio.write((uint8_t*)s.c_str(), s.length());

    //wait for data transmission
    delay(500);

    //receive data
    int n;
    uint8_t buf[256];
    n = SerialPio.read(buf, sizeof(buf)-1);
    buf[n] = 0;
    Serial.print( (strcmp((char*)buf, s.c_str()) == 0 ? "ok " : "ERROR "));
    Serial.print("TX by UART, RX by PIO :");
    Serial.println((char*)buf);

    n = Serial1.readBytes(buf, sizeof(buf)-1);
    buf[n] = 0;
    Serial.print( (strcmp((char*)buf, s.c_str()) == 0 ? "ok " : "ERROR "));    
    Serial.print("TX by PIO,  RX by UART:");
    Serial.println((char*)buf);
}