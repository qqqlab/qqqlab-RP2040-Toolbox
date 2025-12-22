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

#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/uart.h>
#include <string.h>
#include <stdlib.h> //aligned_alloc

#include "RP2040_SerialDMA.h"


//global instances
SerialDMA* SerialDMA::_instances[NUM_UARTS] = {};

//global interrupt handler
void __not_in_flash_func(_SerialDMA_irq_handler()) {
  for(uint8_t i=0;i<NUM_UARTS;i++) {
    if(SerialDMA::_instances[i]) SerialDMA::_instances[i]->_irq_handler();
  }
}

//class instance interrupt handler
void __not_in_flash_func(SerialDMA::_irq_handler()) {
  // rx dma transfer completed
  if( dma_hw->ints0 && (1u << kUartRxChannel) ) {
    dma_hw->ints0 = 1u << kUartRxChannel;
    dma_channel_set_trans_count(kUartRxChannel, kRxBuffLength, true);
  }

  // tx dma transfer completed, just clear flag
  if( dma_hw->ints0 && (1u << kUartTxChannel) ) {
    dma_hw->ints0 = 1u << kUartTxChannel;
  }
}

void SerialDMA::begin(uint8_t uart_num, uint32_t baudrate, uint8_t txpin, uint8_t rxpin, uint16_t txbuflen, uint16_t rxbuflen) {
  uart_ = UART_INSTANCE(uart_num);
  _instances[uart_num] = this;
  gpio_set_function(txpin, GPIO_FUNC_UART);
  gpio_set_function(rxpin, GPIO_FUNC_UART);
  setBaud(baudrate);

  kRxBuffLengthPow = log_2(rxbuflen);
  kTxBuffLengthPow = log_2(txbuflen);
  kRxBuffLength = 1 << (kRxBuffLengthPow);
  kTxBuffLength = 1 << (kTxBuffLengthPow);
  rx_buffer_ = (uint8_t*)aligned_alloc(kRxBuffLength, kRxBuffLength);
  tx_buffer_ = (uint8_t*)aligned_alloc(kTxBuffLength, kTxBuffLength);

  init_dma();
}

//round up to next power of two: 255->8, 256->8, 257->9
uint8_t SerialDMA::log_2(uint16_t val) {
  uint8_t i = 0;
  val--;
  while(val>0) {
    i++;
    val>>=1;
  }
  return i;
}

void SerialDMA::setBaud(uint32_t baudrate) {
  uart_init(uart_, baudrate);
}

void SerialDMA::init_dma() {
  /// DMA uart read
  kUartRxChannel = dma_claim_unused_channel(true);
  dma_channel_config rx_config = dma_channel_get_default_config(kUartRxChannel);
  channel_config_set_transfer_data_size(&rx_config, DMA_SIZE_8);
  channel_config_set_read_increment(&rx_config, false);
  channel_config_set_write_increment(&rx_config, true);
  channel_config_set_ring(&rx_config, true, kRxBuffLengthPow);  //true = write buffer
  channel_config_set_dreq(&rx_config, DREQ_UART0_RX);
  channel_config_set_enable(&rx_config, true);
  dma_channel_configure(kUartRxChannel, &rx_config, rx_buffer_, &uart0_hw->dr, kRxBuffLength<<8, true);
  dma_channel_set_irq0_enabled(kUartRxChannel, true);

  /// DMA uart write
  kUartTxChannel = dma_claim_unused_channel(true);  
  dma_channel_config tx_config = dma_channel_get_default_config(kUartTxChannel);
  channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
  channel_config_set_read_increment(&tx_config, true);
  channel_config_set_write_increment(&tx_config, false);
  channel_config_set_ring(&tx_config, false, kTxBuffLengthPow); //false = read buffer
  channel_config_set_dreq(&tx_config, DREQ_UART0_TX);
  dma_channel_set_config(kUartTxChannel, &tx_config, false);
  dma_channel_set_write_addr(kUartTxChannel, &uart0_hw->dr, false);
  dma_channel_set_irq0_enabled(kUartTxChannel, false);

  // enable interrupt
  irq_add_shared_handler(DMA_IRQ_0, _SerialDMA_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(DMA_IRQ_0, true);
}

uint16_t SerialDMA::availableForWrite() {
  return kTxBuffLength - 1 - dma_channel_hw_addr(kUartTxChannel)->transfer_count;
}

uint16_t SerialDMA::write(const uint8_t* data, uint16_t length) {
  if (length == 0) {
    return 0;
  }

  uint16_t avail = availableForWrite();
  if (length > avail) {
    return 0; // not enough space to write
  }

  //copy to dma buffer
  if ((kTxBuffLength - 1) < tx_user_index_ + length) {
    memcpy(&tx_buffer_[tx_user_index_], data, kTxBuffLength - tx_user_index_);
    memcpy(tx_buffer_, &data[kTxBuffLength - tx_user_index_], length - (kTxBuffLength - tx_user_index_));
  } else {
    memcpy(&tx_buffer_[tx_user_index_], data, length);
  }
  tx_user_index_ = (tx_user_index_ + length) & (kTxBuffLength - 1);

  //check if busy
  dma_channel_hw_t *hw = dma_channel_hw_addr(kUartTxChannel);
  uint32_t ctrl = hw->al1_ctrl;
  bool do_abort = false;
  if(dma_channel_is_busy(kUartTxChannel)) {
    hw->al1_ctrl = 0; //EN=0 -> pause the current transfer sequence (i.e. BUSY will remain high if already high)
    do_abort = true;
  }

  //update tx_dma_size, tx_dma_index
  uint32_t bytes_remaining = dma_channel_hw_addr(kUartTxChannel)->transfer_count;
  uint32_t bytes_written = tx_dma_size - bytes_remaining;
  tx_dma_size = bytes_remaining + length;
  tx_dma_index_ = (tx_dma_index_ + bytes_written) & (kTxBuffLength - 1);

  //abort if needed
  if(do_abort) dma_channel_abort(kUartTxChannel);

  // restart tx dma
  uint8_t* start = &tx_buffer_[tx_dma_index_];
  hw->read_addr = (uintptr_t) start;
  hw->transfer_count = tx_dma_size;
  hw->ctrl_trig = ctrl;

  return length;
}

uint16_t SerialDMA::available() {
  // use current dma buffer state to calculate available bytes
  uint16_t rx_dma_index_local = (kRxBuffLength - dma_channel_hw_addr(kUartRxChannel)->transfer_count) & (kRxBuffLength - 1);
  if (rx_user_index_ <= rx_dma_index_local) {
    return rx_dma_index_local - rx_user_index_;
  }else{
    return kTxBuffLength + rx_dma_index_local - rx_user_index_;
  }
}

uint16_t SerialDMA::read(uint8_t* data, uint16_t length) {
  if (length == 0) {
    return 0;
  }

  uint16_t avail;
  uint16_t rx_dma_index_local = (kRxBuffLength - dma_channel_hw_addr(kUartRxChannel)->transfer_count) & (kRxBuffLength - 1);
  if (rx_user_index_ <= rx_dma_index_local) {
    avail = rx_dma_index_local - rx_user_index_;
  }else{
    avail = kTxBuffLength + rx_dma_index_local - rx_user_index_;
  }

  if (avail < length) {
    // read as much as we have
    length = avail;
  }

  // Update DMA index
  rx_dma_index_ = rx_dma_index_local;

  if (rx_user_index_ < rx_dma_index_) {
    memcpy(data, &rx_buffer_[rx_user_index_], length);
  } else {
    uint16_t left = kRxBuffLength - rx_user_index_;
    if (length < left) {
      left = length; // limit to target buffer size
    }
    memcpy(data, &rx_buffer_[rx_user_index_], left);
    if (left < length) {
      memcpy(&data[left], rx_buffer_, length - left);
    }
  }
  rx_user_index_ = (rx_user_index_ + length) & (kRxBuffLength - 1);

  return length;
}
