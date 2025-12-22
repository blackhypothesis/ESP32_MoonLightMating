let gateway = `ws://${window.location.hostname}/ws`;
let websocket;
let command;

window.onload = function() {
    initWebSocket();
    getDateTime();
    getVersion();
    getQueensConfigStatus();
};

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
    command = msg["command"];
    let motor_element = "motor-" + msg["motor_nr"] + "-state";
    if (command == 0) {
        document.getElementById(motor_element).innerHTML = "Motor stopped."
        document.getElementById(motor_element).style.color = "red";
    }
    else if (command == 1) {
        document.getElementById(motor_element).innerHTML = "Clockwise.";
        document.getElementById(motor_element).style.color = "green";
    }
    else if (command == -1) {
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
    const rbs = document.querySelectorAll('input[name="command"]');
    let command;
    for (const rb of rbs) {
        if (rb.checked) {
            command = rb.value;
            break;
        }
    }

    let steps = document.getElementById("steps").value;
    let msg = '{"steps":' + steps + ',"command":' + command + '}';
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

function getQueensConfigStatus() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            let dip = msg["drone_ip"];
            let hdo = msg["hour_door_open"];
            let mdo = msg["minute_door_open"];
            let hdc = msg["hour_door_close"];
            let mdc = msg["minute_door_close"];
            let sdo = msg["seconds_till_door_open"];
            let sdc = msg["seconds_till_door_close"];
            let qd = msg["queens_delay"];
            let en = msg["config_enable"];

            document.getElementById("drones-url").setAttribute("href", "http://" + dip + "/drones.html");
            document.getElementById("queens-url").setAttribute("href", "http://" + dip + "/queenshives.html");

            document.getElementById("hour-door-open").innerHTML = hdo;
            document.getElementById("minute-door-open").innerHTML = mdo;
            document.getElementById("hour-door-close").innerHTML = hdc;
            document.getElementById("minute-door-close").innerHTML = mdc;
            document.getElementById("queens-delay").innerHTML = qd;
            if (en == 1) {
                document.getElementById("config-enable-text").innerHTML = "Enabled";
                document.getElementById("config-enable-text").style.backgroundColor = "green";
            } else {
                document.getElementById("config-enable-text").innerHTML = "Disabled";
                document.getElementById("config-enable-text").style.backgroundColor = "red";
            }

            (hdo < 10) ? hdo = "0" + hdo : hdo;
            (mdo < 10) ? mdo = "0" + mdo : mdo;
            (hdc < 10) ? hdc = "0" + hdc : hdc;
            (mdc < 10) ? mdc = "0" + mdc : mdc;
            document.getElementById("datetime").innerHTML = msg["datetime"];
            document.getElementById("seconds_till_door_open").innerHTML = secondsToHMS(sdo);
            document.getElementById("seconds_till_door_close").innerHTML = secondsToHMS(sdc);
        }
    }
    xhr.open("GET", "/getconfigstatus", true);
    xhr.send();
}
