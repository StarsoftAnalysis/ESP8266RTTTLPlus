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

#ifndef ESP8266RTTTLPlus_h
#define ESP8266RTTTLPlus_h

namespace e8rtp {

// Define 'DEBUG' to see lots of output on the serial port.
//#define DEBUG

    void setup  (int pin, int volume, const char *buffer);
    void loop   (void);
    void start  (void);
    void reset  (void);
    void stop   (void);
    void pause  (void);
    void resume (void);
    int  setVolume (int volume);

    enum stateEnum { Unready, Ready, Playing, Paused };
    stateEnum state (void);

} // namespace

#endif
