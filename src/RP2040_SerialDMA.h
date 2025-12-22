/*==========================================================================================
MIT License

Copyright (c) 2025 https://github.com/qqqlab

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==========================================================================================*/

//based on: https://github.com/rossihwang/pico_dma_uart

#pragma once

class SerialDMA {
  friend void _SerialDMA_irq_handler(); // allow C-language IRQ handler access to private SerialDMA::_irq_handler()

public:
  //buffer size has to be a power of two, will be rounded up to next greater value: i.e. 128->128, 129->256
  void begin(uint8_t uart_num, uint32_t baudrate, uint8_t txpin, uint8_t rxpin, uint16_t txbuflen, uint16_t rxbuflen);
  void setBaud(uint32_t baudrate); //call this after begin to change baud rate
  uint16_t write(const uint8_t* data, uint16_t length);
  uint16_t read(uint8_t* data, uint16_t length);
  uint16_t available();
  uint16_t availableForWrite();

private:
  static SerialDMA* _instances[NUM_UARTS];
  void _irq_handler();

  uart_inst_t* uart_ = nullptr;
  uint8_t kUartRxChannel;
  uint8_t kUartTxChannel;

  uint8_t kRxBuffLengthPow; // = 8; //2^8 = 256 bytes
  uint8_t kTxBuffLengthPow; // = 8; //2^8 = 256 bytes
  uint16_t kRxBuffLength; // = 1 << (kRxBuffLengthPow);
  uint16_t kTxBuffLength; // = 1 << (kTxBuffLengthPow);

  uint8_t * rx_buffer_ = nullptr; //needs to be aligned! ... static version: __attribute__((aligned(256))) uint8_t rx_buffer_[256];  
  uint16_t rx_user_index_ = 0; // next index to read
  uint16_t rx_dma_index_ = 0;  // next index dma will write

  uint8_t * tx_buffer_ = nullptr; //needs to be aligned! ... static version: //__attribute__((aligned(256))) uint8_t tx_buffer_[256];
  uint16_t tx_user_index_ = 0;  // next index to write
  uint16_t tx_dma_index_ = 0;  // next index dma will read
  uint16_t tx_dma_size = 0; // size of current dma transfer

  void init_dma();
  uint8_t log_2(uint16_t val);
};
