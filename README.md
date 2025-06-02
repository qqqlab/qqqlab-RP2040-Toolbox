# qqqlab-RP2040-Toolbox

A set of tools for RP2040 and RP2350.

## Directional DSHOT

Bi-Directional DSHOT for 1 to 8 ESCs 

Used Resources (for all channels):
- 1 PIO state machine running a 8 instruction PIO program
- 1 DMA channel with 256 byte buffer

Limitation: The channel pins need to be consecutive.

## DSHOT

Uni-Directional DSHOT for 1 to 8 ESCs

Used Resources (for all channels):
- 1 PIO state machine running a 4 instruction PIO program

Limitation: The channel pins need to be consecutive.

## SerialPioIrq

PIO library for Serial Port. TX and RX are non-blocking (buffered) and IRQ driven.

## SerialDMA

A minimal replacement for Arduino HardwareSerial class, uses DMA buffers for both RX and TX to enable non-blocking reads and writes. 

Note: Attempting to write more than fits in the buffer will fail, in order to keep the driver non-blocking in all cases. The regular [pico-arduino](https://github.com/earlephilhower/arduino-pico) SerialUART and SerialPIO libraries have small write buffers and block when writing more than fits in this buffer.

## SerialIRQ

A minimal replacement for Arduino HardwareSerial class, uses IRQ and ring buffers for both RX and TX to enable non-blocking reads and writes. 

Note: Attempting to write more than fits in the buffer will fail, in order to keep the driver non-blocking in all cases. The regular [pico-arduino](https://github.com/earlephilhower/arduino-pico) SerialUART and SerialPIO libraries have small write buffers and block when writing more than fits in this buffer.

## PWM

Minimal Servo / PWM library.
