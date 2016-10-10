
#include "DiscobusDataUart.h"
#include <avr/interrupt.h>

////////////////////////////////////////////
/// Prototypes
////////////////////////////////////////////
void uartReceive();
void uartSendNextByte();
void writeByteToRegister(uint8_t);

////////////////////////////////////////////
/// Macros
////////////////////////////////////////////
#define UART0_UBRRH UBRR0H
#define UART0_UBRRL UBRR0L
#define UART0_UCSRA UCSR0A
#define UART0_UCSRB UCSR0B
#define UART0_UCSRC UCSR0C
#define UART0_UDR   UDR0
#define UART0_UDRE  UDRE0
#define UART0_TXC   TXC0

#ifndef UART0_RX_BUFFER_SIZE
#define UART0_RX_BUFFER_SIZE 150
#endif

#ifndef UART0_TX_BUFFER_SIZE
#define UART0_TX_BUFFER_SIZE 60
#endif


#define UART_BAUD_SELECT(baudRate)  (((F_CPU) + 8UL * (baudRate)) / (16UL * (baudRate)) -1UL)

#define TX_BUFFER_EMPTY() (tx_buffer_head == tx_buffer_tail)
#define TX_NEXT_HEAD_IDX() ((tx_buffer_head + 1) % UART0_TX_BUFFER_SIZE)
#define TX_BUFFER_FULL() (TX_NEXT_HEAD_IDX() == tx_buffer_tail)

#define RX_BUFFER_EMPTY() (rx_buffer_head == rx_buffer_tail)
#define RX_BUFFER_FULL() (((rx_buffer_head + 1) % UART0_RX_BUFFER_SIZE) == rx_buffer_tail)

#define DISABLE_TX_INT() UART0_UCSRB &= ~(1 << UDRIE0);
#define ENABLE_TX_INT() UART0_UCSRB |= (1 << UDRIE0)

////////////////////////////////////////////
/// Static Globals
////////////////////////////////////////////
static volatile uint8_t rx_buffer[UART0_RX_BUFFER_SIZE];
static volatile uint8_t tx_buffer[UART0_TX_BUFFER_SIZE];

static volatile uint8_t tx_buffer_head;
static volatile uint8_t tx_buffer_tail;
static volatile uint8_t rx_buffer_head;
static volatile uint8_t rx_buffer_tail;

////////////////////////////////////////////
/// Class members
////////////////////////////////////////////
DiscobusDataUart::DiscobusDataUart() { }

// Hook into the UART and start receiving data
void DiscobusDataUart::begin(uint32_t baud) {
  UART0_UCSRA = 0;

  // Endable TX/RX
  UART0_UCSRB = (1<<TXEN0) | (1<<RXEN0);
  UART0_UCSRB |= (1<<RXCIE0);  // RX Interrupt

  // Frame format (8-bit, 1 stop bit)
  UART0_UCSRC = 1<<UCSZ01 | 1<<UCSZ00;

  // Set baud
  UART0_UBRRL =  (unsigned char)UART_BAUD_SELECT(baud);
  UART0_UBRRH =  (unsigned char)(UART_BAUD_SELECT(baud) << 8);

  // Enable interrupts
  sei();
}

// Write something to the TX line
void DiscobusDataUart::write(uint8_t c) {

  // If buffer is empty and the register is ready to be written
  // to, send it directly
  if (TX_BUFFER_EMPTY() && (UART0_UCSRA & (1<<UART0_UDRE))) {
    writeByteToRegister(c);
    return;
  }

  // If TX buffer is full, we need to flush a byte out first
  if (TX_BUFFER_FULL()) {
    uartSendNextByte();
  }

  // Add to buffer and enable interrupt
  tx_buffer[tx_buffer_head] = c;
  tx_buffer_head = TX_NEXT_HEAD_IDX();
  ENABLE_TX_INT();
}

// Read a byte from the RX buffer
uint8_t DiscobusDataUart::read() {
  // if the head isn't ahead of the tail, we don't have any characters
  if (RX_BUFFER_EMPTY()) {
    return -1;
  } else {
    uint8_t c = rx_buffer[rx_buffer_tail];
    rx_buffer_tail = (uint8_t)(rx_buffer_tail + 1) % UART0_RX_BUFFER_SIZE;
    return c;
  }
}

// How many bytes are available in the RX buffer
uint8_t DiscobusDataUart::available() {
  return ((uint8_t)(UART0_RX_BUFFER_SIZE + rx_buffer_head - rx_buffer_tail)) % UART0_RX_BUFFER_SIZE;
}

// Clears the RX buffer
void DiscobusDataUart::clear() {
  rx_buffer_head = 0;
  rx_buffer_tail = 0;
}

// Send everything in the TX buffer with blocking
void DiscobusDataUart::flush() {
  DISABLE_TX_INT();
  while (!TX_BUFFER_EMPTY()) {
    uartSendNextByte();
  }
  // Wait for the transmit to complete
  while (!(UART0_UCSRA & (1 << UART0_TXC)));
}

// Not implemented
void DiscobusDataUart::enable_write() { }
void DiscobusDataUart::enable_read() { }

////////////////////////////////////////////
/// Interrupt Controls
////////////////////////////////////////////

// Receive the byte out of the RX register
void uartReceive() {
  // RX buffer full, not taking on new bytes
  if (RX_BUFFER_FULL()) {
    return;
  }

  rx_buffer[rx_buffer_head] = UART0_UDR;
  rx_buffer_head = (rx_buffer_head + 1) % UART0_RX_BUFFER_SIZE;
}

// Send the next byte off the TX buffer
void uartSendNextByte() {
  if (TX_BUFFER_EMPTY()) return;
  DISABLE_TX_INT();

  // Wait for TX to be ready
  while(!(UART0_UCSRA & (1<<UART0_UDRE)));

  // Send from tail and move tail forward
  writeByteToRegister(tx_buffer[tx_buffer_tail]);
  tx_buffer_tail = (tx_buffer_tail + 1) % UART0_TX_BUFFER_SIZE;

  // If buffer isn't empty, enable interrupt
  if (!TX_BUFFER_EMPTY()) {
    ENABLE_TX_INT();
  }
}

// Write a single byte to the TX register
// this assumes you've made sure the register is empty
void writeByteToRegister(uint8_t b) {
  UART0_UDR = b;
  UART0_UCSRA |= (1 << UART0_TXC); // Reset transmit byte
}

// Received a byte from the RX line
ISR(USART_RX_vect){
  uartReceive();
}

// Ready to send a byte on the TX line
ISR(USART_UDRE_vect) {
  uartSendNextByte();
}
