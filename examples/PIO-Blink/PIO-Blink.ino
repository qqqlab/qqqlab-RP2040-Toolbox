/* Blink LED with PIO 

compile blink.pio with: 

pioasm blink.pio blink.pio.h

or use online version:

https://wokwi.com/tools/pioasm

*/

#include "blink.pio.h"

#define BLINK_PIN 25

//this would normally be in blink.pio, inserted here for readability
static inline void blink_program_init(PIO pio, uint sm, uint offset, uint pin) {
  // set the pin's GPIO function to PIO
  pio_gpio_init(pio, pin);

  // set pin as output
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

  // Define a config object
  pio_sm_config config = blink_program_get_default_config(offset);

  // set clock
  sm_config_set_clkdiv(&config, rp2040.f_cpu() / 1000000.0); // set clock to 1 MHz (NOTE: max divider is 65536, so can't reach 1 kHz)

  // Set and initialize the set pins
  sm_config_set_set_pins(&config, pin, 1);

  // set configuration, and set next instruction to execute to start of program (i.e. offset)
  pio_sm_init(pio, sm, offset, &config);

  // start the sm
  pio_sm_set_enabled(pio, sm, true);
}

void setup() {
  /*
  //equivalent arduino blink progam
  pinMode(BLINK_PIN, OUTPUT);
  while(1) {
    digitalWrite(BLINK_PIN, LOW);
    delay(500);
    digitalWrite(BLINK_PIN, HIGH);
    delay(500);
  }
  */
  PIO pio = pio0;
  uint sm = 0; //state machine id
  uint offset = pio_add_program(pio, &blink_program);  //load the program to pio memory, returns offset to start of program
  blink_program_init(pio, sm, offset, BLINK_PIN); //start the program (waits for delay time)
  pio->txf[sm] = 500 * 1000; //set the delay
}

void loop() {
  // do nothing
}
