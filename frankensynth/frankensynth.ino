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
 */

/* Constants: */
// The pins that the scan matrix rows are connected to
const int ROWS[] = {2, 3, 4, 5, 6, 7, 8, 9};

// Shift register (SN74HC595N) pins
const int S_CLOCK = 10;
const int S_LATCH = 11;
const int S_DATA  = 12;

// Shift register bytes (bit 0 is unused for physical wiring convenience)
const byte S_BYTES[] = { B00000010,
                         B00000100,
                         B00001000,
                         B00010000,
                         B00100000,
                         B01000000,
                         B10000000 };

void setup() {
    // Set shift register pins up
    pinMode(S_CLOCK, OUTPUT);
    pinMode(S_LATCH, OUTPUT);
    pinMode(S_DATA, OUTPUT);

    // Set the scan matrix rows up
    int pin;
    for (pin = 0; pin < (sizeof(ROWS) / sizeof(int)); pin++) {
        pinMode(ROWS[pin], INPUT);
    }

    // Set the serial rate to 31,250 bits per second
    //Serial.begin(31250);
    // Use a standard serial rate for testing
    Serial.begin(9600);
}

void set_col(int col) {
    // Set the latch low so the output doesn't change while we're writing data
    digitalWrite(S_LATCH, LOW);

    // Write the appropriate 8 bytes with the most significant bit first
    shiftOut(S_DATA, S_CLOCK, MSBFIRST, S_BYTES[col]);

    // Set the latch high so the new data is displayed
    digitalWrite(S_LATCH, HIGH);
}

void loop() {
    // Go through each column
    int col;
    for (col = 0; col < sizeof(S_BYTES); col++) {
        // Activate the appropriate column
        set_col(col);

        // Read each row and see a key has been pressed
        int row;
        for (row = 0; row < (sizeof(ROWS) / sizeof(int)); row++) {
            int value = digitalRead(ROWS[row]);

            // If the key is pressed
            if (value) {
                Serial.print("Column: ");
                Serial.print(col);
                Serial.print(" Row: ");
                Serial.print(row);
                Serial.print(" has been pressed\n");
            }
        }
    }
}
