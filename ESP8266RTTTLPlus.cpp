// RTTTL Plus
// FIXME written in standard C -- should run on anything  NO -- now uses PWM, so ESP8266 only (and probably ESP32)
// -- no hardware timer, volume control
// integer arithmetic
// Extensions to Nokia standard:
// - octaves 1-8 instead of 4-7   FIXME PWM min is 100Hz  max?
// - any tempo between x and y bpm (instead of set 25, 28, 31, 35, 40, 45, 50, 56, 63, 70, 80, 90, 100, 112, 125, 140, 160, 180, 200, 225, 250, 285, 320, 355, 400, 450, 500, 565, 635, 715, 800 and 900)   TODO range? or apply minimum duration rule as below
// - note fractions 1/64 and 1/128 (instead of only 1, 2, 4, 8, 16, 32) 
//   - in fact, accepts any fraction e.g. 3 (for 1/3) to produce triplets.  
//   - TODO apply minimum length of a note, e.g. 1ms
// - 'dot' before or after the octave number (and multiple dots as in standard music notation)
// - more flexible syntax: minimal tune is "::c" -- must have two colons to separate title/settings/notes, but first two sections can be empty.
//      - dob in any order, or left out (if repeated, last one is used)
// - robust parsing -- makes the best of what it finds

// TODO
//  playing the tune: (or beginning it) -- argument to set length of inter-note gap
//   - and volume

// NOTES
// * tune buffer must remain static in memory while tune is playing

#include <Arduino.h>

#include "ESP8266RTTTLPlus.h"

namespace e8rtp {

// Constants

// standard defaults
static const int stdFraction = 4;  // note duration as fraction e.g. 4 => 1/4 i.e. quarter note
static const int stdOctave   = 5;
static const int stdBeats    = 60;  // BPM, where one beat is per 'fraction' 

static const int maxVolume = 11;

// Macro for minimal tolower() -- just for ASCII letters
#define TOLOWER(char) ((char) | 0b00100000)

#define DEBUG
#ifdef DEBUG
//#   define PRINT(...)   print(__VA_ARGS__)
//#   define PRINTLN(...) println(__VA_ARGS__)
#   define PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
//#   define PRINT(...)
//#   define PRINTLN(...)
#   define PRINTF(...)
#endif

// Details of the current tune
// defaults from tune's prefix
static int tuneFraction = stdFraction;
static int tuneOctave   = stdOctave;
static int tuneBeats    = stdBeats;
static const char *firstNote = NULL;
static const char *nextNote = NULL;
enum state { stateUnready, stateReady, statePlaying };
// or...
static bool tuneLoaded = false;
static bool tuneAtStart = true; // FIXME needed?
static bool tunePlaying = false;
static int currentPitch = 0;
static int currentDuration = 0;
static int currentVolume = maxVolume / 2; // FIXME what range? 1..11, obviously
static int buzzerPin = D1;  // reasonable default?

static void nextChar (const char **bufferPtr) {
    // Move the buffer pointer along to the next non-whitespace character,
    // stopping if the end of the buffer is reached.
    //PRINTF("nextChar skipping '%c'\n", **bufferPtr);
    //*bufferPtr += 1;
    if (**bufferPtr != '\0') {
        *bufferPtr += 1;
    }
    //while ((**bufferPtr == ' ' || **bufferPtr == '\t' || **bufferPtr == '\n' || **bufferPtr == '\r') && (**bufferPtr != '\0')) {
    while (**bufferPtr != '\0' && isspace(**bufferPtr)) {
    //while (isspace(**bufferPtr)) {
        //PRINTF("nextChar skipping '%c'\n", **bufferPtr);
        *bufferPtr += 1;
    }
}

static void setVolume (int volume) {
    if (volume < 0) {
        currentVolume = 0;
    } else if (volume > maxVolume) {
        currentVolume = maxVolume;
    } else {
        currentVolume = volume;
    } 
}

static void nextCharAfter (const char **bufferPtr, char c) {
    // Skip along the buffer to the character after the one of those specified
    //PRINTF("nextCharAfter: c='%c'  *bP='%.10s'  **bP='%c'  len=%ld\n", c, *bufferPtr, **bufferPtr, strlen(*bufferPtr));  // FIXME why is the last thing '' ?
    while (**bufferPtr != '\0' && **bufferPtr != c) {
    //while (strlen(*bufferPtr) > 0 && **bufferPtr != c) {
        //PRINTF("nextCharAfter skipping '%c'\n", **bufferPtr);
        nextChar(bufferPtr);
    }
    //PRINTF("nextCharAfter skipping '%c'\n", c); // (or \0)
    nextChar(bufferPtr);
}

static void nextCharAfterUntil (const char **bufferPtr, char after, char until) {
    // Skip along the buffer to the character after the one specified,
    // but don't go past the 'until' character (which might come first)
    //PRINTF("nextCharAfterUntil: after='%c'  until='%c'  *bP='%.10s'\n", after, until, *bufferPtr);
    while (**bufferPtr != '\0' && **bufferPtr != after && **bufferPtr != until) {
        //PRINTF("nextCharAfterUntil skipping '%c'\n", **bufferPtr);
        nextChar(bufferPtr);
    }
    if (**bufferPtr == after) {
        //PRINTF("nextCharAfterUntil skipping '%c'\n", after); // (or \0)
        nextChar(bufferPtr);
    }
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
    nextCharAfter(bufferPtr, '=');
    //PRINTF("oV: after '=' buffer='%.10s'\n", *bufferPtr);
    value = getInt(bufferPtr);
    if (value == 0) {
        value = dflt;
    }
    //PRINTF("oV: value=%i\n", value);
    //nextChar(bufferPtr);// FIXME maybe shouldn't advance if key didn't match
    // or FIXME assume key does match above
    //?buffer += 1;
    return value;
}

// Set the output pin, Parse the prefix, set d,o,b, 
void setup (int pin, int volume, const char *buffer) {
    buzzerPin = pin;    // need validation?
    pinMode(buzzerPin, OUTPUT);
    analogWrite(buzzerPin, 0);
    setVolume(volume);
    analogWriteRange(maxVolume);    // FIXME what if something else wants to set it different??

    //PRINTF("rtttl:1 buffer='%s'\n", buffer);
    //size_t len = strlen(buffer);
    //if (len < 3) {  // shortest possible tune is e.g. "::c"
    //    return false;
    //}
    // Skip past tune's name and first ':'
    nextCharAfter(&buffer, ':');
    /*
    buffer = strchr(buffer, ':');
    if (buffer == NULL) {
        return false;
    }
    */
    //PRINTF("rtttl:2a buffer='%s'\n", buffer);
    //nextChar(&buffer);
    //PRINTF("rtttl:2 buffer='%.10s'\n", buffer);
    // Default options, e.g. "d=4, o=5, b=120"
    // Allow whitespace, and the keys in any order.
    while (*buffer != '\0' && *buffer != ':') {
        //PRINTF("rtttl:3 looking for dob  buffer=%.10s\n", buffer);
        //PRINTF("\tswitching on '%c'\n", TOLOWER(*buffer));
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
        //PRINTF("rtttl:4 after switch buffer=%.10s\n", buffer);
        nextCharAfterUntil(&buffer, ',', ':');
    }
    //PRINTF("end1: buffer='%.10s'\n", buffer);
    nextCharAfter(&buffer, ':');
    //PRINTF("end2: buffer='%.10s'\n", buffer);
    
    firstNote = buffer;
    nextNote = firstNote;
    tuneLoaded = true;
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

    if (!tuneLoaded || *nextNote == '\0') {
        return;
    }

    // [d]n[a][m][o][m]
    // duration
    fraction = getInt(&nextNote);
    if (fraction == 0) {
        fraction = tuneFraction; // awkward names FIXME
    }
    // FIXME validate fraction
    // convert fraction to duration in ms, using the tune's tempo (beats per minute)
    int tempo = 60000 / tuneBeats;   // ms length of one beat
    duration = tempo * tuneFraction / fraction;
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
    // 'dot' could be here (more than one dot is musically correct, but not in original RTTTL spec)
    while (*nextNote == '.') {
        dotCount += 1;
        nextChar(&nextNote);
    }
    // octave 1..8
    octave = getInt(&nextNote);
    if (note != 'p') {
        if (octave < 1) {
            octave = tuneOctave;
        } else if (octave > 8) {
            octave = 8; // TODO constant for this
        }
        // Adjust pitch for octave -- divide by 2 for each octave below 8
        if (octave < 8) {
            pitch >>= (8 - octave);
        }
    }
    // 'dot' could be here too (more than one dot is musically correct, but not in original RTTTL spec)
    while (*nextNote == '.') {
        dotCount += 1;
        nextChar(&nextNote);
    }
    // skip past comma
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

    PRINTF("note: note=%c  sharp=%d  octave=%d  pitch=%d  fraction=%d  duration=%d\n", note, sharp, octave, pitch, fraction, duration);
    currentPitch = pitch;
    currentDuration = duration;
}

/*
void test (const char *t) {
    PRINTF(rtttl(t) ? "\nOK" : "\nfail");
    PRINTF("   d=%i o=%i b=%i rest='%s'\n", tuneFraction, tuneOctave, tuneBeats, firstNote);
    while (*nextNote != '\0') {
        getNote();
        delay(currentDuration + 100);
    }
}
*/

/*
// Start the next note
static void playNote (void) {
    if 
*/
    
static long unsigned noteStart = 0;

void start (void) {
    tunePlaying = true;
    nextNote = firstNote;
    //getNote();
    //analogWriteFreq(currentPitch);
    //analogWrite(buzzerPin, currentVolume);
    noteStart = millis() - (long unsigned)(currentDuration + 1);    // put the start time far enough into the pass to trigger the first note
    tuneAtStart = false;
    PRINTF("e8rtp: starting melody");
}

// Set tune to start -- need to call startPlaying() too
void reset (void) {
    nextNote = firstNote;
    tuneAtStart = true;
    tunePlaying = false;
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
    tunePlaying = false;
}

// Carry on after having paused
void resume (void) {
    tunePlaying = true;
    // FIXME may need to toy with noteStart here?
}

// Call this in the main loop to keep things ticking along
void loop (void) {
    if (tunePlaying && (millis() - noteStart > (long unsigned)(currentDuration))) {
        analogWrite(buzzerPin, 0);   // sound off
        PRINTF("e8rtp::loop: end of note\n");
        if (*nextNote != '\0') {
            delay(10);  // TODO properly: gap between notes
            getNote();
            if (currentPitch >= 100) {
                analogWriteFreq(currentPitch);
                // TEMP OUT for quiet testing: 
                analogWrite(buzzerPin, currentVolume);
                PRINTF("e8rtp::loop: starting to play pitch %d, duration will be %d\n", currentPitch, currentDuration);
            }
            noteStart = millis();
        } else {
            // end of tune
            tunePlaying = false;
            PRINTF("e8rtp::loop: end of tune\n");
        }
    }
}

} // namespace
