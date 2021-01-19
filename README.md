# ESP8266RTTTLPlus
An RTTTL-player for ESP8266, with extra features.

## Features

### RTTTL

[RTTTL](https://en.wikipedia.org/wiki/Ring_Tone_Transfer_Language) is the Ring Tone Text Transfer Language developed by Nokia for creating ringtones on mobile phones back in the day.

It's a useful way to write simple tunes to be played by a buzzer or small speaker attached to a microcontroller.

Example: `WalkLike:d=4,o=5,b=200:p,8f#,8d#,d#,d#,d#.,c#,f#.,d#,8d#,e,8d#,b4,b4.,a4,8A4,B4`

This library is aimed at [ESP8266](https://en.wikipedia.org/wiki/ESP8266)-based boards, and uses [PWM](https://en.wikipedia.org/wiki/Pulse-width_modulation) to create the tones.

It interprets the RTTTL specification somewhat freely, adding some new features, but maintaining compatibility with standard RTTTL.

#### Extensions to Nokia standard:

* Allows the title section to be of any length, instead of limiting it to 10 characters.
* Covers octaves 1-8 instead of 4-7   FIXME PWM min is 100Hz  max?
* Allows any tempo between x and y bpm (instead of the fixed set of tempos defined by Nokia)   TODO range? or apply minimum duration rule as below
* Note lengths are indicated by denominators, e.g. '4c' for a crotchet (or quarter note).  The library
does not force the denominator to be one of 1, 2, 4, 8, 16, or 32, so you can use any reasonable integer.  The main use of
this would be to create triplets with, e.g., '3c'.
A minimum duration (xxx ms) is applied to each note.
* Notes can be given in either upper or lower case.
* White space anywhere in the melody is ignored.
* [Dots](https://en.wikipedia.org/wiki/Dotted_note) are used to extend the duration of a note by half.  For example '2a.' (a dotted minim) specifies a duration 
equivalent to three crotchets.  Two dots multiply a note's duration by 1.75.  Note that the original RTTTL specification 
requires the dot to be placed at the end of the note, after the octave number, e.g. '2C#6.'.  Many RTTTL tunes available on the web, and
some other RTTTL-parsing libraries, put the dot /before/ the octave number, e.g. '2C#.6'.  This library accepts dots in either position.
* The title and default value sections can be empty, so the shortest possible valid melody is '::c'.
* The duration, octave, and beat in the default value section can be given in any order, or left out entirely.  'Default defaults' will be applied if necessary.
* The parser does its best to make sense of poorly formed notes.

## Usage

* Set the output pin that the buzzer is attached to.  Don't need to set it as output -- setup() will do that.



## Functions

## setup(...)

## start()

## reset() -- sets the player back to the first note.  Not sure if that's useful.

## stop()

## pause()

## resume() -- will continue with the melody, starting with the next note.


