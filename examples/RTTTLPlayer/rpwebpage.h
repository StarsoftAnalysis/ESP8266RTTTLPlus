const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en" class="h-100">
<head>
<!--

    Copyright 2021 Chris Dennis

    This file is part of ESP8266RTTTLPlus.

    ESP8266RTTTLPlus is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ESP8266RTTTLPlus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ESP8266RTTTLPlus.  If not, see <https://www.gnu.org/licenses/>.

-->
    <title>RTTTL Player</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">

    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css" integrity="sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T" crossorigin="anonymous">

    <script src="https://code.jquery.com/jquery-3.5.1.min.js" integrity="sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0=" crossorigin="anonymous"></script>
    <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.min.js" integrity="sha384-JjSmVgyd0p3pXB1rRibZUAYoIIy6OrQ6VrjIEaFf/nJGzIxFDsf4x0xIM+B07jRM" crossorigin="anonymous"></script>

</head>

<body class="d-flex flex-column h-100">
<nav class="navbar navbar-expand-md navbar-dark bg-dark">
  <div class="container">
      <h1 class="text-white mb-0">RTTTL Player</h1>
  </div>
</nav>
<main role="main">
  <div class="container">
    <div class=form-group>
      <div class="form-row">
        <div class="col-auto">
          <label for=melody class="h3 mt-3">RTTTL Melody</label>
          <textarea id=melody class="form-control" rows="5" cols="60" maxlength=500></textarea>
          <button type=button class="btn btn-primary mt-3" id=start onclick='start()'>Start</button>
          <button type=button class="btn btn-primary mt-3" id=pause onclick='pause()'>Pause</button>
          <button type=button class="btn btn-primary mt-3" id=resume onclick='resume()'>Resume</button>
          <button type=button class="btn btn-primary mt-3" id=stop onclick='stop()'>Stop</button>
          <p class="h4 mt-3"><label for="volume">Volume:</label> <output for="volume" id="volumeOutput"></output>
          <input class="form-control" type="range" name="volume" id="volume" min="0" max="11" step="1" value="5">
          <p class="h5 text-secondary" id=result></p>
        </div>
    </div>
  </div>
</main>

<script>

const volumeInput  = document.querySelector('#volume');
const volumeOutput = document.querySelector('#volumeOutput');
volumeOutput.textContent = volumeInput.value;

function ajaxSuccess (data, status) {
    $("#result").textContent = data;
}

function ajaxError (data, status) {
    $("#result").html("Error: " + status + ": " + data);
}

function showVolume (data, status) {
    const volume = parseInt(data);
    if (isNaN(volume)) {
        $("#result").textContent = data;
    } else {
        volumeOutput.textContent = volume.toString();
        volumeInput.value = volume;
    }
}

// Play the tune in the textarea
function start () {
    const parms = "melody=" + encodeURIComponent($("#melody").val());
    $.ajax({"url": "start?" + parms, "success": ajaxSuccess, "error": ajaxError});
}

function pause () {
    $.ajax({"url": "pause", "success": ajaxSuccess, "error": ajaxError});
}

function resume () {
    $.ajax({"url": "resume", "success": ajaxSuccess, "error": ajaxError});
}

function stop () {
    $.ajax({"url": "stop", "success": ajaxSuccess, "error": ajaxError});
}

function showMelody (data, status) {
    console.log(data, status);
    $("#melody").val(data);
}

function getMelody () {
    $.ajax({"url": "getmelody", "success": showMelody, "error": ajaxError});
}

function setVolume () {
    const parms = "volume=" + volume.value.toString();
    $.ajax({"url": "setvolume?" + parms, "success": showVolume, "error": ajaxError});
}

$(document).ready(function() {
    getMelody();
    volumeInput.addEventListener('input', setVolume);
});

</script>
  </body>
</html>
)=====";
