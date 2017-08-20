#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <HT1621.h>

#define ONE_DAY_WD_COUNT 10800 // 1j
#define PIN_BUTTON 0
#define PCIE_BUTTON PCINT0

/*
0: 0b10111110
1: 0b00000110
2: 0b01111100
3: 0b01011110
4: 0b11000110
5: 0b11011010
6: 0b11111010
7: 0b00001110
8: 0b11111110
9: 0b11011110
.: &0x01
-: 0b01000000
*/
static unsigned char NUMBERS[] = {
  0b10111110, 0b00000110, 0b01111100, 0b01011110, 0b11000110,
  0b11011010, 0b11111010, 0b00001110, 0b11111110, 0b11011110
};

HT1621 ht(2 /*CS*/, 3 /*CLK*/, 4 /*Data*/);

int g_dayCounter = 0;
unsigned long g_wdCounter = 0;
volatile bool g_bttnPressed = true;
volatile bool g_demoMode = false;

ISR(WDT_vect) {
  if(g_bttnPressed) {
    return;
  }

  g_wdCounter++;
  if(g_demoMode || g_wdCounter >= ONE_DAY_WD_COUNT) {
    g_dayCounter++;
    g_wdCounter = 0;
  }
}

ISR(PCINT0_vect) {
  if ( (PINB & _BV(PCIE_BUTTON)) == 0 ) {
    g_bttnPressed = true;
  }
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
//From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {
  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  byte bb = timerPrescaler & 7;
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~_BV(WDRF); //Clear the watch dog reset
  WDTCR |= _BV(WDCE) | _BV(WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}

void sleep() {
  GIMSK |= _BV(PCIE); // - Turns on pin change interrupts
  PCMSK |= _BV(PIN_BUTTON); // - Turns on interrupts on pins
  ADCSRA &= ~_BV(ADEN); // - Disable ADC, saves ~230uA
  setup_watchdog(g_demoMode ? 6 /* 1s */ : 9 /* 8s */); // - Set watchdog interval
  sleep_mode(); // - Go to sleep

  //ADCSRA |= _BV(ADEN); // - Enable ADC
}

void displayNumber(int number) {
  if(number >= 1000) {
    ht.write(0, NUMBERS[number / 1000]);
  } else {
    ht.write(0, 0);
  }
  if(number >= 100) {
    ht.write(1, NUMBERS[ (number - number / 1000 * 1000) / 100]);
  } else {
    ht.write(1, 0);
  }
  if(number >= 10) {
    ht.write(2, NUMBERS[ (number - number / 100 * 100) / 10 ]);
  } else {
    ht.write(2, 0);
  }
  ht.write(3, NUMBERS[ (number - number / 10 * 10) ]);
}

void setup() {
  register uint8_t i;

  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_BUTTON, HIGH); // Enable pullup resistor

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  ht.begin();
  ht.sendCommand(HT1621::RC256K);
  ht.sendCommand(HT1621::BIAS_THIRD_4_COM);
  ht.sendCommand(HT1621::SYS_EN);
  ht.sendCommand(HT1621::LCD_ON);

  // clear memory
  for(i = 0; i < 4; i++) {
    ht.write(i, 0);
  }

  displayNumber(g_dayCounter);
}

void loop() {
  sleep();

  if(g_bttnPressed) {
    g_wdCounter = 0;
    g_dayCounter = 0;
  }

  if(g_dayCounter > 999) {
    ht.write(0, 0);
    ht.write(1, 0);
    ht.write(2, 0);
    ht.write(3, 0b01000000);
  }
  displayNumber(g_dayCounter);

  if(g_bttnPressed) {
    delay(5000);
    if(digitalRead(PIN_BUTTON) == LOW) {
      g_demoMode = !g_demoMode;
    }
    if(g_demoMode) {
      g_dayCounter = 0;
    }
    g_bttnPressed = false;
  }
}
