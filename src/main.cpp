#include <Arduino.h>

// Rotary encoder
// BTN_ENA leads BTN_ENB w/ clockwise rotation
// BTN_ENC is low when button is pressed
const int BTN_ENA = 8, BTN_ENB = 9;

volatile int pos;
const int debounce_stable_count = 4;
#define OCR0A_INCR 64


/*
 * TIMER0_COMPA_vect
 * ISR triggered by timer0 where inputs are read and debounced.
 */

ISR(TIMER0_COMPA_vect) {
    static int last_a = 1;
    static int curr_a = 1;
    static int curr_a_stable_count;

    static int initial_b;

    OCR0A = (OCR0A + OCR0A_INCR) & 0xff;

    // Check the value of button B when a rising edge on button A is found.
    //  A rising edge on A when B is low indicates clockwise motion while a
    // rising edge on A when B is high indicates counter-clockwise motion

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
        if ((curr_a_stable_count > 0) &&
            (curr_a_stable_count < debounce_stable_count)) {
            curr_a_stable_count++;
        }
    }

    // if values are stable and rising edge on A, update position
    if (curr_a_stable_count == debounce_stable_count) {
        if ((last_a == 0) && (curr_a == 1)) {
            if (initial_b == 0) {
                pos++;
            } else {
                pos--;
            }
            if (pos < 0) {
                pos += 20;
            } else if (pos > 19) {
                pos -= 20;
            }
        }

        curr_a_stable_count = 0;
        last_a = curr_a;
    }
}


void setup() {
    Serial.begin(9600);

    // Add a pullup to the buttons
    pinMode(BTN_ENA, INPUT_PULLUP);
    pinMode(BTN_ENB, INPUT_PULLUP);

    // set up a recuring timer interrupt
    cli();  // disable interrupts

    TCCR0A &= ~((1 << WGM01) | (1 << WGM00));  // Turn off PWM modes
    TCCR0B &= ~(1 << WGM02);

    OCR0A = 1;              // set timer0/A to generate output when it
                            // hits this value.  The ISR will adjust this
                            // value to to potentially produce multiple
                            // interrupts during the normal 0 -> FF count
                            // sequence which takes 1024us
    TIMSK0 |= _BV(OCIE0A);  // Set the compare A output to cause an interrupt

    sei();  // enable interrupts
}

void loop() {
    static int last_pos;

    if (pos != last_pos) {
        Serial.print("Pos: ");
        Serial.print(pos);
        Serial.print("\n");
        last_pos = pos;
    }
}
