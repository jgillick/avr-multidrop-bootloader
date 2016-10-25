
#ifndef SHARED_H
#define SHARED_H


// Drive the signal line low
inline void signalEnable() {
  SIGNAL_DDR_REG |= (1 << SIGNAL_BIT);
  SIGNAL_PIN_REG &= ~(1 << SIGNAL_BIT);
}

// Put signal into tri-state
inline void signalDisable() {
  SIGNAL_DDR_REG &= ~(1 << SIGNAL_BIT);
}


#endif
