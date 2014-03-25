/* Copyright (c) 2014, Liam Fraser <liam@liamfraser.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of Liam Fraser nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* This code makes a 49-key keyboard, organised into a scan matrix of 7 columns
 * and 8 rows act as a MIDI keyboard.
 *
 * Some parts of this code use ideas from:
 * http://www.codetinkerhack.com/2012/11/how-to-turn-piano-toy-into-midi.html
 *
 * You can find a table of MIDI commands here:
 * http://www.midi.org/techspecs/midimessages.php
 */

// The pins that the scan matrix rows are connected to
const int ROWS[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int ROW_C = sizeof(ROWS) / sizeof(int);

// Shift register (SN74HC595N) pins
const int S_CLOCK = 10;
const int S_LATCH = 11;
const int S_DATA  = 12;

// Shift register bytes that represent columns (bit 0 is unused for physical
// wiring convenience)
const byte S_BYTES[] = { B00000010,
                         B00000100,
                         B00001000,
                         B00010000,
                         B00100000,
                         B01000000,
                         B10000000 };
const int COL_C = sizeof(S_BYTES);

// The default key press / release velocity (it can range from 0 - 127)
const int ON_VELOCITY = 64;
const int OFF_VELOCITY = 64;

// Keep track of which keys are pressed and depressed
int KEY_STATE[COL_C][ROW_C];

// Map each column and row to a midi note value
int KEY_MAP[COL_C][ROW_C];

// The MIDI note number to start at when building the key map. 36 is the C
// at the beginning of Octave 3.
const int KEY_MAP_START = 36;

// The Analogue pin that the sustain pedal is connected to
const int SUSTAIN = 0;

// The voltage below which the sustain pedal is off, and above which the
// sustain pedal is on. 0-5v is represented by 0-1023. This will depend on
// pedal and how it works. I want my threshold as 2V. 2V / (5V / 1024) = 409
const int SUSTAIN_THRESH = 409;

void setup() {
    // Initialise all key states to 0
    int col;
    int row;
    for (col = 0; col < COL_C; col++) {
        for (row = 0; row < ROW_C; row++) {
            KEY_STATE[col][row] = 0;
        }
    }

    // Build the midi key map
    int key = KEY_MAP_START;
    for (col = 0; col < COL_C; col++) {
        for (row = 0; row < ROW_C; row++) {
            KEY_MAP[col][row] = key;
            key++;
        }
    }

    // Set shift register pins up
    pinMode(S_CLOCK, OUTPUT);
    pinMode(S_LATCH, OUTPUT);
    pinMode(S_DATA, OUTPUT);

    // Set the scan matrix rows up
    int pin;
    for (pin = 0; pin < ROW_C; pin++) {
        pinMode(ROWS[pin], INPUT);
    }

    // Set the serial rate to 31,250 bits per second
    Serial.begin(31250);
}

void set_col(int col) {
    // Set the latch low so the output doesn't change while we're writing data
    digitalWrite(S_LATCH, LOW);

    // Write the appropriate 8 bytes with the most significant bit first
    shiftOut(S_DATA, S_CLOCK, MSBFIRST, S_BYTES[col]);

    // Set the latch high so the new data is displayed
    digitalWrite(S_LATCH, HIGH);
}

void note_on(int col, int row) {
    KEY_STATE[col][row] = 1;
    // 0x90 turns the note on on channel 1
    Serial.write(0x90);
    Serial.write(KEY_MAP[col][row]);
    Serial.write(ON_VELOCITY);
}

void note_off(int col, int row) {
    KEY_STATE[col][row] = 0;
    // 0x80 turns the note off on channel 1
    Serial.write(0x80);
    Serial.write(KEY_MAP[col][row]);
    Serial.write(OFF_VELOCITY);
}

void sustain_on() {
    // Channel 0 controller message
    Serial.write(0xB0);
    // Controller message type == sustain
    Serial.write(0x40);
    // Sustain value (≤63 off, ≥64 on)
    Serial.write(127);
}

void sustain_off() {
    // Channel 0 controller message
    Serial.write(0xB0);
    // Controller message type == sustain
    Serial.write(0x40);
    // Sustain value (≤63 off, ≥64 on)
    Serial.write(0);
}

void loop() {
    // Go through each column
    int col;
    for (col = 0; col < COL_C; col++) {
        // Activate the appropriate column
        set_col(col);

        // Read each row and get the value
        int row;
        for (row = 0; row < ROW_C; row++) {
            int value = digitalRead(ROWS[row]);

            // If the key state is different to the one we have in memory
            if (value != KEY_STATE[col][row]) {
                if (value == 1) {
                    // Turn the note on
                    note_on(col, row);
                } else {
                    // Turn the note off
                    note_off(col, row);
                }
            }
        }
    }

    // Finally, check the sustain pedal
    int sustain_volts = analogRead(SUSTAIN);
    if (sustain_volts < SUSTAIN_THRESH) {
        //sustain_off();
    } else {
        //sustain_on();
    }
}
