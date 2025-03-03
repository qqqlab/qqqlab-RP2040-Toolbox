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

  // restart tx dma if tx buffer is not empty and dma is not busy 
  if(tx_dma_index_ != tx_user_index_ && !dma_channel_is_busy(kUartTxChannel)) {
    uint size = (tx_dma_index_ <= tx_user_index_)
                    ? (tx_user_index_ - tx_dma_index_)
                    : (kTxBuffLength + tx_user_index_ - tx_dma_index_);
    
    uint8_t* start = &tx_buffer_[tx_dma_index_];
    dma_channel_transfer_from_buffer_now(kUartTxChannel, start, size);
    tx_dma_index_ = (tx_dma_index_ + size) & (kTxBuffLength - 1);
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
  channel_config_set_ring(&rx_config, true, kRxBuffLengthPow);
  channel_config_set_dreq(&rx_config, DREQ_UART0_RX);
  channel_config_set_enable(&rx_config, true);
  dma_channel_configure(kUartRxChannel, &rx_config, rx_buffer_, &uart0_hw->dr, kRxBuffLength, true);
  dma_channel_set_irq0_enabled(kUartRxChannel, true);

  /// DMA uart write
  kUartTxChannel = dma_claim_unused_channel(true);  
  dma_channel_config tx_config = dma_channel_get_default_config(kUartTxChannel);
  channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
  channel_config_set_read_increment(&tx_config, true);
  channel_config_set_write_increment(&tx_config, false);
  channel_config_set_ring(&tx_config, false, kTxBuffLengthPow);
  channel_config_set_dreq(&tx_config, DREQ_UART0_TX);
  dma_channel_set_config(kUartTxChannel, &tx_config, false);
  dma_channel_set_write_addr(kUartTxChannel, &uart0_hw->dr, false);
  dma_channel_set_irq0_enabled(kUartTxChannel, true);

  // enable interrupt
  irq_add_shared_handler(DMA_IRQ_0, _SerialDMA_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(DMA_IRQ_0, true);
}

uint16_t SerialDMA::availableForWrite() {
  if(tx_dma_index_ <= tx_user_index_) {
    return kTxBuffLength - 1 + tx_dma_index_ - tx_user_index_;
  }else{
    return tx_dma_index_ - tx_user_index_;
  }
}

uint16_t SerialDMA::write(const uint8_t* data, uint16_t length) {
  if (length == 0) {
    return 0;
  }
  uint16_t avail = availableForWrite();

  if (length > avail) {
    return 0; // not enough space to write
  }

  if (tx_dma_index_ < tx_user_index_) {
    if ((kTxBuffLength - 1) < tx_user_index_ + length) {
      memcpy(&tx_buffer_[tx_user_index_], data, kTxBuffLength - tx_user_index_);
      memcpy(tx_buffer_, &data[kTxBuffLength - tx_user_index_], length - (kTxBuffLength - tx_user_index_));
    } else {
      memcpy(&tx_buffer_[tx_user_index_], data, length);
    }

  } else {
    if ((kTxBuffLength - 1) < tx_user_index_ + length) {
      memcpy(&tx_buffer_[tx_user_index_], data, kTxBuffLength - tx_user_index_);
      memcpy(tx_buffer_, &data[kTxBuffLength - tx_user_index_], length - (kTxBuffLength - tx_user_index_));
    } else {
      memcpy(&tx_buffer_[tx_user_index_], data, length);
    }
  }
  tx_user_index_ = (tx_user_index_ + length) & (kTxBuffLength - 1);

  //start tx dma if not started yet
  irq_set_pending(DMA_IRQ_0);

  return length;
}

uint16_t SerialDMA::available() {
  if (rx_user_index_ <= rx_dma_index_) {
    return rx_dma_index_ - rx_user_index_;
  }else{
    return kTxBuffLength + rx_dma_index_ - rx_user_index_;
  }
}

uint16_t SerialDMA::read(uint8_t* data, uint16_t length) {
  // Update DMA index
  rx_dma_index_ = kRxBuffLength - dma_channel_hw_addr(kUartRxChannel)->transfer_count;

  uint16_t avail = available();
  if (avail < length) {
    // read as much as we have
    length = avail;
  }
  
  if (length == 0) {
    return 0;
  }

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
