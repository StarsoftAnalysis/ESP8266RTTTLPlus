// ESP8266RTTTLPlus
//
// RTTTL parsing library for ESP8266 and similar microcontrollers.
//
// Copyright 2021 Chris Dennis
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    ESP8266RTTTLPlus is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this software.  If not, see <https://www.gnu.org/licenses/>.
//

#include <Arduino.h>

#include "ESP8266RTTTLPlus.h"

namespace e8rtp {

// Constants

// standard defaults
static const int stdFraction = 4;  // note duration as fraction e.g. 4 => 1/4 i.e. quarter note
static const int stdOctave   = 5;
static const int stdBeats    = 60;  // BPM, where one beat is per 'fraction' 

static const int maxVolume = 11;
static const int minOctave = 3;
static const int maxOctave = 8;

// Controlling volume via PWM duty cycle is not ideal -- the duty cycle also affects timbre and perceived pitch.
// Trial and error suggests values for duty up to about 70/1023 are usable,
// but it depends a lot on the hardware (i.e. the type of buzzer or speaker) too.
//static int volumeTable[] = {0, 20, 26, 32, 41, 51, 68, 102, 155, 256, 512, 512};
// These values assume PWM range is 1023.
static const int PWMRange = 1023;
static int volumeTable[] = {0, 4, 8, 13, 18, 24, 30, 37, 44, 52, 61, 70};

// Macro for minimal tolower() -- just for ASCII letters
#define TOLOWER(char) ((char) | 0b00100000)

//#define DEBUG
#ifdef DEBUG
#   define PRINT(...)   print(__VA_ARGS__)
#   define PRINTLN(...) println(__VA_ARGS__)
#   define PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
#   define PRINT(...)
#   define PRINTLN(...)
#   define PRINTF(...)
#endif

// Details of the current tune
// defaults from tune's prefix
static int tuneFraction = stdFraction;
static int tuneOctave   = stdOctave;
static int tuneBeats    = stdBeats;
static long unsigned tuneWholeNoteLength = stdFraction * 60000 / stdBeats; // beat length in ms
static const char *firstNote = NULL;
static const char *nextNote = NULL;
static stateEnum currentState = Unready;
static int currentPitch = 0;
static long unsigned currentDuration = 0;
static int currentVolume = maxVolume / 2;
static int buzzerPin = D1;  // reasonable default?

static void nextChar (const char **bufferPtr) {
    // Move the buffer pointer along to the next non-whitespace character,
    // stopping if the end of the buffer is reached.
    //PRINTF("nextChar skipping '%c'\n", **bufferPtr);
    if (**bufferPtr != '\0') {
        *bufferPtr += 1;
    }
    while (**bufferPtr != '\0' && isspace(**bufferPtr)) {
        //PRINTF("nextChar skipping '%c'\n", **bufferPtr);
        *bufferPtr += 1;
    }
}

int setVolume (int volume) {
    if (volume < 0) {
        currentVolume = 0;
    } else if (volume > maxVolume) {
        currentVolume = maxVolume;
    } else {
        currentVolume = volume;
    } 
    PRINTF("e8rtp: sV: volume=%d cV=%d\n", volume, currentVolume);
    return currentVolume;   // for external callers
}

static void nextCharAfter (const char **bufferPtr, char c) {
    // Skip along the buffer to the character after the one of those specified
    //PRINTF("nextCharAfter: c='%c'  *bP='%.10s'  **bP='%c'  len=%ld\n", c, *bufferPtr, **bufferPtr, strlen(*bufferPtr));
    while (**bufferPtr != '\0' && **bufferPtr != c) {
        //PRINTF("nextCharAfter skipping '%c'\n", **bufferPtr);
        nextChar(bufferPtr);
    }
    //PRINTF("nextCharAfter skipping '%c'\n", c); // (or \0)
    nextChar(bufferPtr);
}

static bool nextCharAfterUntil (const char **bufferPtr, char after, char until) {
    // Skip along the buffer to the character after the one specified,
    // but don't go past the 'until' character.
    // Return 'true' if we found the 'after' character.
    //PRINTF("nextCharAfterUntil: after='%c'  until='%c'  *bP='%.10s'\n", after, until, *bufferPtr);
    while (**bufferPtr != '\0' && **bufferPtr != after && **bufferPtr != until) {
        //PRINTF("nextCharAfterUntil skipping '%c'\n", **bufferPtr);
        nextChar(bufferPtr);
    }
    if (**bufferPtr == after) {
        //PRINTF("nextCharAfterUntil skipping '%c'\n", after); // (or \0)
        nextChar(bufferPtr);
        return true;
    }
    return false;
}

static int getInt (const char **bufferPtr) {
    // Get unsigned decimal integer value from beginning of buffer,
    // moving the pointer along to the first non-numeric character
    int value = 0;
    while (isdigit(**bufferPtr)) {
        value = (value * 10) + (**bufferPtr - '0');
        nextChar(bufferPtr);
    }
    return value;
}

static int optionValue (const char **bufferPtr, int dflt) {
    int value = 0;
    // skip past '=' (and any spurious characters before '='), but not past ':'
    if (nextCharAfterUntil(bufferPtr, '=', ':')) {
        //PRINTF("oV: after '=' buffer='%.10s'\n", *bufferPtr);
        value = getInt(bufferPtr);
        if (value == 0) {
            value = dflt;
        }
        //PRINTF("oV: value=%i\n", value);
    }
    return value;
}

stateEnum state (void) {
    return currentState;
}

// Set the output pin, Parse the prefix, set d,o,b, 
void setup (int pin, int volume, const char *buffer) {
    buzzerPin = pin;    // need validation?
    pinMode(buzzerPin, OUTPUT);
    analogWrite(buzzerPin, 0);
    setVolume(volume);
    analogWriteRange(PWMRange);

    // Skip past tune's name and first ':'
    nextCharAfter(&buffer, ':');

    // Default options, e.g. "d=4, o=5, b=120"
    // Allow whitespace, and the keys in any order.
    while (*buffer != '\0' && *buffer != ':') {
        switch (TOLOWER(*buffer)) {
            case 'd':
                tuneFraction = optionValue(&buffer, stdFraction);
                break;
            case 'o':
                tuneOctave = optionValue(&buffer, stdOctave);
                break;
            case 'b':
                tuneBeats = optionValue(&buffer, stdBeats);
                break;
            default:
                // unexpected character - ignore it
                break;
        }
        nextCharAfterUntil(&buffer, ',', ':');
    }
    nextCharAfter(&buffer, ':');
    
    tuneWholeNoteLength = tuneFraction * 60000 / tuneBeats;   // length of whole note in milliseconds

    PRINTF("e8rtp::setup done d=%d o=%d b=%d wnl=%lu cV=%d\n", tuneFraction, tuneOctave, tuneBeats, tuneWholeNoteLength, currentVolume);
    firstNote = buffer;
    nextNote = firstNote;
    currentState = Ready;
}

// pitch for octave 8 (calculation from https://en.wikipedia.org/wiki/Piano_key_frequencies)
// starting at 'A' for easier indexing
//                                 A    A#     B     C    C#     D    D#     E     F    F#     G    G#
static const int octave8[] =   {7040, 7459, 7902, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645};
//                                 0     1     2     3     4     5     6     7     8     9    10    11
static const int noteOffset[] = {  0,          2,    3,          5,          7,    8,         10      };

void getNote (void) {
    int  pitch      = 0;
    int  pitchIndex = 0;
    bool sharp      = false;
    int  octave     = 0;
    char note       = ' ';
    int  fraction   = 0;    // note length as fraction, e.g. 4 => 1/4
    int  duration   = 0;    // in ms
    int  dotCount   = 0;

    if ((currentState == Unready) || *nextNote == '\0') {
        // TODO set currentDuration and Pitch to zero??
        return;
    }

    // Note length as fraction of a whole note
    fraction = getInt(&nextNote);
    if (fraction == 0) {
        fraction = tuneFraction;
    }
    // convert fraction to duration in ms
    duration = tuneWholeNoteLength / fraction;

    // note, with optional sharp sign
    note = TOLOWER(*nextNote);
    nextChar(&nextNote);
    if (note == 'h') { // 'h' means 'b' in some parts of the world, apparently
        note = 'b';
    }
    // read invalid note letter as 'C'
    if (note != 'p' && (note < 'a' || note > 'g')) {
        note = 'c';
    }
    // For pause ('p') pitch and octave are irrelevant, 
    // but we still have to look for dots that could change the duration
    if (note != 'p') {
        pitchIndex = noteOffset[note - 'a']; 
    }

    sharp = (*nextNote == '#');
    if (sharp) {
        nextCharAfter(&nextNote, '#');
        pitchIndex += 1;
    }
    if (note != 'p') {
        pitch = octave8[pitchIndex]; // not adjusted for octave yet 
    }

    // 'dot' could be here
    while (*nextNote == '.') {
        dotCount += 1;
        nextChar(&nextNote);
    }

    // octave
    octave = getInt(&nextNote);
    if (note != 'p') {
        if (octave == 0) {
            octave = tuneOctave;
        } else if (octave < minOctave) {
            octave = minOctave;
        } else if (octave > maxOctave) {
            octave = maxOctave;
        }
        // Adjust pitch for octave -- divide by 2 for each octave below 8
        pitch >>= (maxOctave - octave);
    }

    // 'dot' could be here too
    while (*nextNote == '.') {
        dotCount += 1;
        nextChar(&nextNote);
    }

    // skip past comma (or get to end of tune)
    nextCharAfter(&nextNote, ',');

    // Apply all the accumulated dots (more than one is rare)
    {
        int extraDuration = duration;
        while (dotCount > 0) {
            extraDuration /= 2;
            duration += extraDuration;
            dotCount -= 1;
        }
    }

    //PRINTF("e8rtp: note: note=%c  sharp=%d  octave=%d  pitch=%d  fraction=%d  duration=%d\n", note, sharp, octave, pitch, fraction, duration);
    currentPitch = pitch;
    currentDuration = duration;
}

static long unsigned noteStart = 0;

void start (void) {
    nextNote = firstNote;
    noteStart = millis() - (currentDuration + 1);    // put the start time far enough into the past to trigger the first note
    currentState = Playing;
    PRINTF("e8rtp: starting melody\n");
    // (playing will start next time round the loop)
}

// Set tune to start
void reset (void) {
    nextNote = firstNote;
    currentState = Ready;
}

// Stop playing immediately
void stop (void) {
    analogWrite(buzzerPin, 0);   // sound off
    reset();
}

// Stop, but don't reset -- to nearest note:
void pause (void) {
    analogWrite(buzzerPin, 0);   // sound off
    // don't reset();
    currentState = Paused;
}

// Carry on after having paused
void resume (void) {
    currentState = Playing;
    // FIXME may need to toy with noteStart here?
}


// Call this in the main loop to keep things ticking along
void loop (void) {
    if ((currentState == Playing) && (millis() - noteStart > currentDuration)) {
        analogWrite(buzzerPin, 0);   // sound off
        PRINTF("e8rtp::loop: end of note\n");
        if (*nextNote != '\0') {
            delay(10);  // gap between notes
            getNote();
            if (currentPitch >= 100) {
                analogWriteFreq(currentPitch);
                analogWrite(buzzerPin, volumeTable[currentVolume]);
                PRINTF("e8rtp::loop: starting to play pitch %d, duration %lu, volume %d = %d\n", currentPitch, currentDuration, currentVolume, volumeTable[currentVolume]);
            }
            noteStart = millis();
        } else {
            // end of tune
            reset();
            //tunePlaying = false;
            nextNote = firstNote;
            PRINTF("e8rtp::loop: end of tune\n");
        }
    }
}

} // namespace
