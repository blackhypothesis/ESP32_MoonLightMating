let gateway = `ws://${window.location.hostname}/ws`;
let websocket;
window.addEventListener('load', onload);
let direction;
getVersion();
getConfigStatus();

function onload(event) {
    initWebSocket();
}

// Websocket
// ---------------------------------------------------------
function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    document.getElementById("motor-0-state").innerHTML = "Motor stopped."
    document.getElementById("motor-1-state").innerHTML = "Motor stopped."
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log(event.data);
    let msg = JSON.parse(event.data);
    direction = msg["direction"];
    let motor_element = "motor-" + msg["motor_nr"] + "-state";
    if (direction == 0) {
        document.getElementById(motor_element).innerHTML = "Motor stopped."
        document.getElementById(motor_element).style.color = "red";
    }
    else if (direction == 1) {
        document.getElementById(motor_element).innerHTML = "Clockwise.";
        document.getElementById(motor_element).style.color = "green";
    }
    else if (direction == -1) {
        document.getElementById(motor_element).innerHTML = "Anticlockwise.";
        document.getElementById(motor_element).style.color = "blue";
    }
}

// ---------------------------------------------------------
function handleNavBar() {
    let x = document.getElementById("myTopnav");
    if (x.className === "topnav") {
        x.className += " responsive";
    }
    else {
        x.className = "topnav";
    }
}

function runMotor() {
    const rbs = document.querySelectorAll('input[name="direction"]');
    let direction;
    for (const rb of rbs) {
        if (rb.checked) {
            direction = rb.value;
            break;
        }
    }

    let steps = document.getElementById("steps").value;
    let msg = '{"steps":' + steps + ',"direction":' + direction + '}';
    console.log(msg);
    websocket.send(msg);
}

function secondsToHMS(secs) {
    let hours = Math.floor(secs / (60 * 60));

    let divisor_for_minutes = secs % (60 * 60);
    let minutes = Math.floor(divisor_for_minutes / 60);

    let divisor_for_seconds = divisor_for_minutes % 60;
    let seconds = Math.ceil(divisor_for_seconds);

    (hours < 10) ? hours = "0" + hours : hours;
    (minutes < 10) ? minutes = "0" + minutes : minutes;
    (seconds < 10) ? seconds = "0" + seconds : seconds;

    return hours + ":" + minutes + ":" + seconds;
}

function getVersion() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            document.getElementById("version").innerHTML = `Version: ${msg["version"]}`;
        }
    }
    xhr.open("GET", "/getversion", true);
    xhr.send();
}

