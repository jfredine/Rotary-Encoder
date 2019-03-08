/*
 * Demonstrate reading a rotary encoder
 *
 * Author: John Fredine
 * Original Date: 1/8/2019
 */

#include <Arduino.h>

//******************************************************************************
// User adjustable parameters
// Rotary encoder pin connections
// BTN_ENA leads BTN_ENB w/ clockwise rotation
const int BTN_ENA = 8, BTN_ENB = 9;

// number of "stopping points" in a full revolution of the encoder
const int num_detents_per_revolution = 20;

// number of samples with constant signal value to be considered "debounced"
const int debounce_stable_count = 4;

// time between interrupts for monitoring signal value
// resolution of the timer is 4us so beset to make this a multiple of 4
#define ISR_INTERVAL_us 256

//******************************************************************************
// calculated parameters which should not need user adjustment

// assume a rising and folling edge of each button happens between detents
const int num_edges_per_revolution = 2 * num_detents_per_revolution;

// Number of timer ticks between interrupts.  Each tick is 4us
#define OCR0A_INCR (ISR_INTERVAL_us / 4)

// current position of the encoder
volatile int pos;

/*
 * TIMER0_COMPA_vect
 * ISR triggered by timer0 where inputs are read and debounced.
 * In order to get interrupts at a rate greater than 1 ms (default for timer0),
 * but not require an additional timer and not disturb the timer0 overflow rate
 * (because that is used by default libraries), it makes use of the timer
 * compare interrupt and adjusts the comparison value within the ISR.
 */

ISR(TIMER0_COMPA_vect) {
    static int last_a = 1;
    static int curr_a = 1;
    static int curr_a_stable_count;

    static int initial_b;

    // set the time of the next interrupt
    OCR0A = (OCR0A + OCR0A_INCR) & 0xff;

    // get value of B the first time we detect a change on A
    // we assume B is stable by the time A begins to change
    int val = digitalRead(BTN_ENA);
    if ((curr_a_stable_count == 0) && (val != last_a)) {
        initial_b = digitalRead(BTN_ENB);
    }

    // debounce A
    if (val != curr_a) {
        curr_a = val;
        curr_a_stable_count = 1;
    } else {
        if (curr_a_stable_count > 0) {
            curr_a_stable_count++;
        }
    }

    // if values are stable, adjust rotary encoder position
    // B low on rising edge of A indicates clockwise motion while a
    // B high on rising edge of A indicates counter-clockwise motion.
    // B low on falling edge on A indicates counter clockwise motion
    // B high on falling edge of A indicates clockwise motion
    if (curr_a_stable_count == debounce_stable_count) {
        if (last_a != curr_a) {
            if (curr_a == initial_b) {
                pos--;
                if (pos < 0) {
                    pos += num_edges_per_revolution;
                }
            } else {
                pos++;
                if (pos >= num_edges_per_revolution) {
                    pos -= num_edges_per_revolution;
                }
            }

            last_a = curr_a;
        }

        curr_a_stable_count = 0;
    }
}


void setup() {
    Serial.begin(9600);

    // Add a pullup to the buttons
    pinMode(BTN_ENA, INPUT_PULLUP);
    pinMode(BTN_ENB, INPUT_PULLUP);

    // set up a recuring timer interrupt
    cli();  // disable interrupts

    // Turn off PWM modes of timer0 because such modes prevent the
    // immediate re-assignment of compare values
    TCCR0A &= ~((1 << WGM01) | (1 << WGM00));
    TCCR0B &= ~(1 << WGM02);

    OCR0A = 1;              // Set the timer0 compare value.
                            // The ISR will adjust this value to to potentially
                            // produce multiple interrupts during the normal
                            // 0 -> FF count sequence which takes 1024us
    TIMSK0 |= _BV(OCIE0A);  // Enable the timer compare interrupt

    sei();  // enable interrupts
}


void loop() {
    static int last_pos;

    if (pos != last_pos) {
        Serial.print("Pos: ");
        Serial.println(pos);
        last_pos = pos;
    }
}
