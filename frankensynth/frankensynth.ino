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
#define ROW_C 8

// Shift register (SN74HC595N) pins
#define S_CLOCK 10
#define S_LATCH 11
#define S_DATA  12

// Shift register bytes that represent columns (bit 0 is unused for physical
// wiring convenience)
const byte S_BYTES[] = { B00000010,
                         B00000100,
                         B00001000,
                         B00010000,
                         B00100000,
                         B01000000,
                         B10000000 };
#define COL_C 7

// The MIDI ON/OFF velocity. Set by a knob
byte VELOCITY = 0;

// The pin that the velocity knob is connected to
#define VELOCITY_P 5

// Keep track of which keys are pressed and depressed
bool KEY_STATE[COL_C][ROW_C];

// Map each column and row to a midi note value
byte KEY_MAP[COL_C][ROW_C];

// The MIDI note number to start at when building the key map. 36 is the C
// at the beginning of Octave 3.
#define KEY_MAP_START 36

// The Analogue pin that the sustain pedal is connected to
#define SUSTAIN_P 0

// The sustain state
bool SUSTAIN = false;

// The voltage below which the sustain pedal is off, and above which the
// sustain pedal is on. 0-5v is represented by 0-1023. This will depend on
// pedal and how it works. I want my threshold as 2V. 2V / (5V / 1024) = 409
#define SUSTAIN_THRESH 409

// Pins for the two effect knobs followed by variables to track their state
#define EFFECT_1_P 4
#define EFFECT_2_P 3
byte EFFECT_1 = 0;
byte EFFECT_2 = 0;
byte EFFECT_1_PREV = 0;
byte EFFECT_2_PREV = 0;

// The analogue pins that the octave rotary switches are connected to via a
// resistor ladder
#define LEFT_OCTAVE_P 1
#define RIGHT_OCTAVE_P 2

// The voltage thresholds for the rotary switch positions. Will be compared in
// a if volts > RST_1, else if volts > RST_2 and so on. Resistors are 1 = 1k,
// to 6 = 47k
#define RST_6 102 // 0.5v
#define RST_5 204 // 1v
#define RST_4 409 // 2v
#define RST_3 614 // 3v
#define RST_2 798 // 3.9v
#define RST_1 880 // 4.3v

// Rotary switch pitch state (-3, 0, +2) will be -3 octaves, normal, +2 octaves
short LEFT_OCTAVE = 0;
short RIGHT_OCTAVE = 0;
short LEFT_OCTAVE_PREV = 0;
short RIGHT_OCTAVE_PREV = 0;

// The keyboard will be split into a left and right for octave purposes.
// Define the key that this happens at in terms of rows and columns. I'm using
// middle C and including that in the right octave. Middle C is key 25 which is
// column 4 and row 1, but we use zero based indexes in the loop
#define OCTAVE_SPLIT_ROW 0
#define OCTAVE_SPLIT_COL 3

void reset_key_state() {
    byte col;
    byte row;
    for (col = 0; col < COL_C; col++) {
        for (row = 0; row < ROW_C; row++) {
            KEY_STATE[col][row] = false;
        }
    }
}

void setup() {
    // Initialise all key states to 0
    reset_key_state();

    // Build the midi key map
    byte key = KEY_MAP_START;
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
    byte pin;
    for (pin = 0; pin < ROW_C; pin++) {
        pinMode(ROWS[pin], INPUT);
    }

    // Set the serial rate to 31,250 bits per second
    Serial.begin(31250);
}

void set_col(byte col) {
    // Set the latch low so the output doesn't change while we're writing data
    digitalWrite(S_LATCH, LOW);

    // Write the appropriate 8 bytes with the most significant bit first
    shiftOut(S_DATA, S_CLOCK, MSBFIRST, S_BYTES[col]);

    // Set the latch high so the new data is displayed
    digitalWrite(S_LATCH, HIGH);
}

void note_on(short note) {
    KEY_STATE[col][row] = true;
    // 0x90 turns the note on on channel 1
    Serial.write(0x90);
    Serial.write(note);
    Serial.write(VELOCITY);
}

void note_off(short note) {
    KEY_STATE[col][row] = false;
    // 0x80 turns the note off on channel 1
    Serial.write(0x80);
    Serial.write(note);
    Serial.write(VELOCITY);
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

void effect_change(byte effect_num) {
    // Channel 0 controller message
    Serial.write(0xB0);

    if (effect_num == 1) {
        // Message type == Effect Control 1
        Serial.write(0x0C);
        // Effect value
        Serial.write(EFFECT_1);
    } else {
        // Message type == Effect Control 2
        Serial.write(0x0D);
        // Effect value
        Serial.write(EFFECT_2);
    }
}

byte volt_to_midi(short volts) {
    // Convert a voltage from 0-1023 (0-5V) to a MIDI value of 0-127
    // 1024 / 128 = 8, so divide voltage by 8 and return
    return volts / 8;
}

short rotary_to_octave(short volts) {
    // Returns the number of midi notes to take away or add on depending on
    // the position of a rotary switch
    if (volts > RST_1) {
        return -36;
    } else if (volts > RST_2) {
        return -24;
    } else if (volts > RST_3) {
        return -12;
    } else if (volts > RST_4) {
        return 0;
    } else if (volts > RST_5) {
        return 12;
    } else {
        return 24;
    }
}

void loop() {
    // Set the velocity according to the velocity knob
    VELOCITY = volt_to_midi(analogRead(VELOCITY_P));

    // Deal with the two effect knobs
    EFFECT_1 = volt_to_midi(analogRead(EFFECT_1_P));
    EFFECT_2 = volt_to_midi(analogRead(EFFECT_2_P));

    if (EFFECT_1 != EFFECT_1_PREV) {
        EFFECT_1_PREV = EFFECT_1;
        effect_change(1);
    }
    if (EFFECT_2 != EFFECT_2_PREV) {
        EFFECT_2_PREV = EFFECT_2;
        effect_change(2);
    }

    // Read in the rotary switches to put out notes at the desired octaves
    LEFT_OCTAVE = rotary_to_octave(analogRead(LEFT_OCTAVE_P));
    RIGHT_OCTAVE = rotary_to_octave(analogRead(RIGHT_OCTAVE_P));

    // If either of the octaves have changed, turn off every note to avoid
    // the possibility of never turning a note off
    if ((LEFT_OCTAVE != LEFT_OCTAVE_PREV) ||
        (RIGHT_OCTAVE != RIGHT_OCTAVE_PREV)) {
        byte note;
        for (note = 0; note < 128; note++) {
            note_off(note);
        }

        // Reset the key states too
        reset_key_state();
    }


    // We've dealt with the octaves now, so update the prev values
    LEFT_OCTAVE_PREV = LEFT_OCTAVE;
    RIGHT_OCTAVE_PREV = RIGHT_OCTAVE;

    // Go through each column
    byte col;
    for (col = 0; col < COL_C; col++) {
        // Activate the appropriate column
        set_col(col);

        // Read each row and get the value
        byte row;
        for (row = 0; row < ROW_C; row++) {
            bool value = digitalRead(ROWS[row]);

            // If the key state is different to the one we have in memory
            if (value != KEY_STATE[col][row]) {
                // Set the note according to the octave knobs
                short note;

                if ((col >= OCTAVE_SPLIT_COL) && (row >= OCTAVE_SPLIT_ROW)) {
                    // Right side
                    note = KEY_MAP[col][row] + RIGHT_OCTAVE;
                } else {
                    // Left side
                    note = KEY_MAP[col][row] + LEFT_OCTAVE;
                }

                if (value) {
                    // Turn the note on
                    note_on(note);
                } else {
                    // Turn the note off
                    note_off(note);
                }
            }
        }
    }

    // Check the sustain pedal
    short sustain_volts = analogRead(SUSTAIN_P);
    if (sustain_volts < SUSTAIN_THRESH) {
        if (SUSTAIN) {
            SUSTAIN = false;
            sustain_off();
        }
    } else {
        if (!SUSTAIN) {
            SUSTAIN = true;
            sustain_on();
        }
    }

}
