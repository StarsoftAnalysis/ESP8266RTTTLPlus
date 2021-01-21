/*******************************************************************

    RTTTLPlayer.ino

    Example program for the ESP8266RTTTLPlus library
    that displays a web page where the user can create 
    and play RTTTL melodies.

    Copyright 2021 Chris Dennis

    ---
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    It is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software.  If not, see <https://www.gnu.org/licenses/>.
    ---

****************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "ArduinoJson.h"
// v6
// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

#include <WiFiManager.h>
// Library used for creating the captive portal for entering WiFi Details
// Search for "Wifimanager" in the Arduino Library manager
// https://github.com/tzapu/WiFiManager

#include <ESP8266RTTTLPlus.h>

#include "debug.h"

// Declare 'webpage': 
#include "rpwebpage.h"

// -----------------------------

// --- Pin Configuration ---
#define BUZZER_PIN  D1

// ------------------------

ESP8266WebServer server(80);

// Buffer for user's melody, with an example
static char melodyBuffer[500] = "Bouree:d=4,o=6,b=180:16g#.5,8a#.5,b5,16a#.5,8g#.5,g5,16g#.5,8a#.5,d#5,16f.5,8g.5,g#5,16f#.5,8e.5,d#5,16c#.5,8b.4,a#4,16b.4,8c#.5,d#5,16c#.5,16b.4,16a#.4,g#4,16g#.5,8a#.5,b5,16a#.5,8g#.5,d#,16b.5,8d#.,d#5,16f.5,8g.5,g#5,16f#.5,8e.5,d#5,16c#.5,8b.4,8b4,32b4,32c#5,32b4,32c#5,16b4,8a#.4,2b4";

static int currentVolume = 5;   // To match default volume hard-coded in HTML

// HTTP request handlers:

void handleRoot(void) {
    server.send(200, "text/html", webpage);
}

void handleNotFound(void) {
    String message = "File Not Found\n";
    server.send(404, "text/plain", message);
}

void handleStart (void) {
    // Start playing the tune
    if (server.hasArg("melody")) {
        Serial.printf("start: melody is '%s'\n", server.arg("melody").c_str());
        strlcpy(melodyBuffer, server.arg("melody").c_str(), sizeof(melodyBuffer));
        e8rtp::setup(BUZZER_PIN, currentVolume, melodyBuffer);    
        e8rtp::start();
        server.send(200, "text/plain", String("Playing started"));
    } else {
        server.send(200, "text/plain", String("Error: no melody received"));
    }
}

void handleStop (void) {
    Serial.println("stop");
    e8rtp::stop();
    server.send(200, "text/plain", String("Playing stopped"));
}

void handlePause (void) {
    Serial.println("pause");
    e8rtp::pause();
    server.send(200, "text/plain", String("Playing paused"));
}

void handleResume (void) {
    Serial.println("resume");
    e8rtp::pause();
    server.send(200, "text/plain", String("Playing resumed"));
}

void handleGetMelody (void) {
    Serial.println("getMelody");
    server.send(200, "text/plain", String(melodyBuffer));
}

void handleSetVolume (void) {
    if (server.hasArg("volume")) {
        int volume = server.arg("volume").toInt();
        Serial.printf("setVolume: %d\n", volume);
        currentVolume = e8rtp::setVolume(volume);
        server.send(200, "text/plain", String(currentVolume));
    } else {
        server.send(200, "text/plain", String("Error: no volume received"));
    }
}

void setup(void) {
    Serial.begin(115200);
    Serial.println("\n=======================");

    WiFiManager wifiManager;
    wifiManager.autoConnect("rtttlplayer");
    IPAddress ipAddress = WiFi.localIP();

    Serial.println("\nWiFi Connected");
    Serial.print("IP address: ");
    Serial.println(ipAddress);

    // HTTP Server
    server.on("/", handleRoot);
    server.on("/start", handleStart);
    server.on("/stop", handleStop);
    server.on("/pause", handlePause);
    server.on("/resume", handleResume);
    server.on("/getmelody", handleGetMelody);
    server.on("/setvolume", handleSetVolume);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP Server Started");

}

void loop(void) {

    e8rtp::loop();
    
    server.handleClient();

}
