
#ifndef COMMON_H
#define COMMON_H


// Drive the signal line low
#define signalEnable() { \
  SIGNAL_DDR_REG |= (1 << SIGNAL_PIN); \
  SIGNAL_PORT_REG &= ~(1 << SIGNAL_PIN); \
}

// Put signal into tri-state
#define signalDisable() SIGNAL_DDR_REG &= ~(1 << SIGNAL_PIN);


#endif