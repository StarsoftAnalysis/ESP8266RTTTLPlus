# ESP8266RTTTLPlus
An RTTTL-player for ESP8266, with extra features.

## Features

### RTTTL

[RTTTL](https://en.wikipedia.org/wiki/Ring_Tone_Transfer_Language) is the Ring Tone Text Transfer Language developed by Nokia for creating ringtones on mobile phones back in the day.

It's a useful way to write simple tunes to be played by a buzzer or small speaker attached to a microcontroller.

Example: `WalkLike:d=4,o=5,b=200:p,8f#,8d#,d#,d#,d#.,c#,f#.,d#,8d#,e,8d#,b4,b4.,a4,8A4,B4`

This library is aimed at [ESP8266](https://en.wikipedia.org/wiki/ESP8266)-based boards, and uses [PWM](https://en.wikipedia.org/wiki/Pulse-width_modulation) to create the tones.
It may well work on other Arduino-esque microcontrollers that support PWM.

It interprets the RTTTL specification somewhat freely, adding some new features, but maintaining compatibility with standard RTTTL.

And, of course, the volume control goes all the way up to 11.

#### Extensions to Nokia standard:

* Allows the title section to be of any length, instead of limiting it to 10 characters.
* Covers octaves 3-8 instead of just 4-7.  The minimum frequency supported by the ESP8266's software-based PWM is 100Hz.
* Allows any tempo between x and y bpm (instead of the fixed set of tempos defined by Nokia)   TODO range? or apply minimum duration rule as below
* Note lengths are indicated by denominators, e.g. `4c` for a crotchet (or quarter note).  The library
does not force the denominator to be one of 1, 2, 4, 8, 16, or 32, so you can use any reasonable integer.  The main use of
this would be to create triplets with, e.g., `3c`.
A minimum duration (xxx ms) is applied to each note.
* Notes can be given in either upper or lower case.
* White space anywhere in the melody is ignored.
* [Dots](https://en.wikipedia.org/wiki/Dotted_note) are used to extend the duration of a note by half.  For example `2a.` (a dotted minim) specifies a duration 
equivalent to three crotchets.  Two dots multiply a note's duration by 1.75.  Note that the original RTTTL specification 
requires the dot to be placed at the end of the note, after the octave number, e.g. `2C#6.`.  Many RTTTL tunes available on the web, and
some other RTTTL-parsing libraries, put the dot /before/ the octave number, e.g. `2C#.6`.  This library accepts dots in either position.
* The title and default value sections can be empty, so the shortest possible valid melody is `::c`.
* The duration, octave, and beat in the default value section can be given in any order, or left out entirely.  'Default defaults' will be applied if necessary.
* The parser does its best to make sense of poorly formed notes.

## Example

```
#include <ESP8266RTTTLPlus.h>
#define BUZZER_PIN D1

void setup (void) {
    e8rtp::setup(BUZZER_PIN, 5, "Georgia on my mind: d=4,o=5,b=120: 8e,2g.,8p,8e,2d.,8p,p,e,a,e,2d.,8c,8d,e,g,b,a,f,f,8e,e,1c");
    e8rtp::start();
}

void loop (void) {
    e8rtp::loop();	// This must be called from your main 'loop()', otherwise nothing will happen.
}   
```

For other examples, see the `examples` directory above.

And for a complete project that uses ESP8266RTTTLPlus, See this [alarm clock software](https://github.com/StarsoftAnalysis/arduino-alarm-clock).

## Installation

Download ESP8266RTTTLPlus into your Arduino library directory.  It's not available in the Arduino IDE library manager yet, but hopefully it will be soon.

## Usage

Set the output pin that the buzzer is attached to.  You don't need to set it as an output pin -- `e8rtp::setup()` will do that.

All the functions in this library are in the `e8rtp` namespace.

### e8rtp::state()

`e8rtp::stateEnum e8rtp::state (void);`

Returns the current state of the player -- return one of:

| State | Description |
| ----- | ----------- |
| `e8rtp::Unready` | No melody has been loaded by `e8rtp::setup()`. |
| `e8rtp::Ready`   | A melody has been loaded and is ready to play, or it has finished playing. |
| `e8rtp::Playing` | The tune is playing, and can be stopped or paused A melody has been loaded and is ready to play. |
| `e8rtp::Paused`  | The tune has been paused: call `e8rtp::resume()' to continue playing it. |

### e8rtp::setup()

`void e8rtp:setup (int pin, int volume, char * melody)`

Set up the player by specifying the pin that the buzzer is connected to, the initial volume level (0..11), and the 
character string containing the melody.

Note that the character string must remain static in memory while the melody is being played, because
the player uses a pointer to it, rather than copying into its own storage.

Example:
```
e8rtp::setup(BUZZER_PIN, 5, "Georgia on my mind: d=4,o=5,b=63:8e,2g.,8p,8e,2d.,8p,p,e,a,e,2d.,8c,8d,e,g,b,a,f,f,8e,e,1c");
```

### e8rtp::start()

`void e8rtp::start (void)`

Start playing the melody from the beginning.

### e8rtp::reset() 

`void e8rtp::reset (void)`

Sets the player back to the first note.  I'm not really sure what you'd use that for.

### e8rtp::stop()

`void e8rtp::stop (void)`

Stops playing the melody, and resets back to the beginning.

### e8rtp::pause()

`void e8rtp::pause (void)`

Pause the melody, ready to be `e8rtp::resume()`d later.

### e8rtp::resume()

`void e8rtp::resume (void)`

Continue palying the melody after being `e8rtp::pause()`d, starting with the next note.

## e8rtp::setVolume(int volume) 

`int e8rtp::setVolume (int volume)`

Change the volume at any time after `e8rtp::setup()` to a value in the range 0 to 11.
Returns the valid volume used (i.e, if you try `e8rtp::setup(44)` it will set the volume to 11
and return 11 to tell you that).

