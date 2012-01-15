/*
 * RTCTimerCounter2.ino
 * Author: Steve Marple
 * License: Gnu General Public License version 2 
 *
 * Example sketch to demonstrate the real-time clock outputting a 1Hz
 * square wave which is used as an input by timer/counter2 on the
 * ATmega1284P. An interrupt service routine is called when
 * timer/counter2 has reached its top value; the ISR sets isrRan
 * to true. Every run of loop() checks if the isrRan is true; if
 * so it is set to false and the current time is printed to the
 * console. The state of D13 is also toggled, to flash the on-board
 * LED.
 *
 * To run this sketch on a standard Arduino (ATmega168 or ATmega328)
 * you will need to use timer/counter 1; define USE_CT to be 1. Use D5
 * for the input. Using timer/counter2 might be possible if your
 * ATmega328 uses the internal oscillator and the crystal is not
 * fitted.
 * 
 * This sketch will not work with an Arduino Mega(2560) since the
 * timer inputs are not connected to the shield headers; see
 * RTCInterrupts sketch for an alternative method.
 *
 * Requirements:
 *   Arduino >= 1.0
 *   RealTimeClockDS1307, see
 *     https://github.com/davidhbrown/RealTimeClockDS1307
 *
 *   Calunium configuration:
 *     RTC (DS1307 or similar) fitted, with battery
 *     RTC square wave routed to D15 (JP1 fitted)
 */

#include <Wire.h>
#include <RealTimeClockDS1307.h>

bool ledValue = false;

// Use an 8-bit variable to obtain atomic read/write access. Disabling
// interrupts is therefore not needed.
volatile bool isrRan = false;

// Select which timer/counter to use
#define USE_CT 2

#if USE_CT == 1
ISR(TIMER1_COMPA_vect)
{
  isrRan = true;
}

// Configure counter/timer1. Count the falling edges to have the ticks
// occuring on the second boundary. 
void setupTimer1(void)
{
  noInterrupts();
  ASSR = _BV(AS2);
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 1;
  TCCR1B |= _BV(WGM12); // CTC mode
  TCCR1B |= (_BV(CS12) | _BV(CS11)); // External clock source, falling edge
  TIMSK1 |= _BV(OCIE1A);
  interrupts();
}

#elif USE_CT == 2
ISR(TIMER2_COMPA_vect)
{
  isrRan = true;
}

// Configure counter/timer2 in asynchronous mode, counting the rising
// edge of signals on TOSC1. This always triggers on the rising edge;
// if connecting SQW from a DS1307 then this means the ticks occur
// halfway through the second.
void setupTimer2(void)
{
  noInterrupts();
  ASSR = (_BV(EXCLK) | _BV(AS2));
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  OCR2A = 3; // Have the ISR called every 4th (n+1) tick
  TCCR2A |= _BV(WGM21); // CTC mode
  TCCR2B |= _BV(CS20);
  TIMSK2 |= _BV(OCIE2A);
  interrupts();
}

#else
#error "Not configured for this timer"
#endif

void setup(void)
{
  pinMode(SS, OUTPUT); // Prevent SPI slave mode from happening
  Wire.begin();
  Serial.begin(9600);

  // For calunium: enable inputs on pin 6 in case JP4 is fitted and
  // routing SQW there also
  pinMode(6, INPUT); 

    
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, ledValue);

#if USE_CT == 1
  // Use counter/timer1. On Calunium digital pin 5 is T1, adjust for
  // if necessary your board.
  pinMode(5, INPUT); // Make T1 pin an input
  setupTimer1();

#elif USE_CT == 2
  pinMode(15, INPUT);
  setupTimer2();
#else
#error "Not configured for this timer"
#endif

  // Set up the RTC output
  RTC.sqwEnable(RealTimeClockDS1307::SQW_1Hz);
}


unsigned long last = 0;
void loop(void) 
{
  // Print if the interrupt service routine was called, and the
  // current number of seconds
  if (isrRan) {
    isrRan = false;
    ledValue = !ledValue;
    digitalWrite(LED_BUILTIN, ledValue);
    RTC.readClock();
    Serial.print("ISR called: ");
    Serial.println(RTC.getSeconds());
  }

  // Every 200ms print the current number of seconds 
  unsigned long now = millis();
  if (now > last + 200) {
    last = now;
    RTC.readClock();
    Serial.println(RTC.getSeconds());
  }
}
