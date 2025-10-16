let gateway = `ws://${window.location.hostname}/ws`;
let websocket;
window.addEventListener('load', onload);
let direction;
getVersion();
getConfigStatus();

function onload(event) {
    initWebSocket();
    getDateTime();
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

function getDateTime() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let myObj = JSON.parse(this.responseText);
            console.log(myObj);
            let datetime = myObj["datetime"];
            document.getElementById("datetime").innerHTML = datetime;
        }
    }
    xhr.open("GET", "/getdatetime", true);
    xhr.send();
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

function getConfigStatus() {
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

            document.getElementById("hour-door-open").value = hdo;
            document.getElementById("minute-door-open").value = mdo;
            document.getElementById("hour-door-close").value = hdc;
            document.getElementById("minute-door-close").value = mdc;
            document.getElementById("queens-delay").value = qd;
            if (en == 1) {
                document.getElementById("config-enable").checked = true;
                document.getElementById("config-enable-text").innerHTML = "Enabled";
            } else {
                document.getElementById("config-enable").checked = false;
                document.getElementById("config-enable-text").innerHTML = "Disabled";
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

function getClientStates() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);

            const table = document.getElementById("client-state");
            table.innerHTML = "";
            msg.forEach(item => {
                let row = table.insertRow();
                let ip = row.insertCell(0);
                ip.innerHTML = "<a href=\"http://" + item.ip + "\" target=\"_blank\">" + item.ip + "<\a>";
                let mac = row.insertCell(1);
                mac.innerHTML = item.mac;
                let active = row.insertCell(2);
                active.innerHTML = item.active;
                let last_update = row.insertCell(3);
                last_update.innerHTML = item.seconds;
            });
        }
    }
    xhr.open("GET", "/getclientstates", true);
    xhr.send();
}

function setCurrentDateTime() {
    let datetime = new Date();
    let epochseconds = datetime.getTime()
    epochseconds = epochseconds / 1000;
    // UTC to GMT+1
    epochseconds = epochseconds + 3600;
    // Summertime
    // epochseconds = epochseconds + 3600;
    let xhr = new XMLHttpRequest();
    xhr.open("GET", "/setdatetime?epochseconds=" + epochseconds, true);
    xhr.send();
}

function toggleCheckbox(element) {
    if (element.checked) {
        document.getElementById("config-enable-text").innerHTML = "Enabled";
        console.log("config enabled");
    } else {
        document.getElementById("config-enable-text").innerHTML = "Disabled";
        console.log("config disabled");
    }
}

function setScheduleConfig() {
    let valid_params = true;
    let hour_open = document.getElementById("hour-door-open").value;
    let minute_open = document.getElementById("minute-door-open").value;
    let hour_close = document.getElementById("hour-door-close").value;
    let minute_close = document.getElementById("minute-door-close").value;
    let queens_delay = document.getElementById("queens-delay").value;
    let config_enable = 0;

    if (hour_open < 0 || hour_open > 23) {
        document.getElementById("hour-door-open").style.color = "red";
        valid_params = false;
    } else {
        document.getElementById("hour-door-open").style.color = "black";
    }
    if (minute_open < 0 || minute_open > 59) {
        document.getElementById("minute-door-open").style.color = "red";
        valid_params = false;
    } else {
        document.getElementById("minute-door-open").style.color = "black";
    }
    if (hour_close < 0 || hour_close > 23) {
        document.getElementById("hour-door-close").style.color = "red";
        valid_params = false;
    } else {
        document.getElementById("hour-door-close").style.color = "black";
    }
    if (minute_close < 0 || minute_close > 59) {
        document.getElementById("minute-door-close").style.color = "red";
        valid_params = false;
    } else {
        document.getElementById("minute-door-close").style.color = "black";
    }
    if (queens_delay < 0 || queens_delay > 300) {
        document.getElementById("queens-delay").style.color = "red";
        valid_params = false;
    } else {
        document.getElementById("queens-delay").style.color = "black";
    }
    if (document.getElementById("config-enable").checked == true) {
        config_enable = 1;
    }
    console.log("checkbox: " + document.getElementById("config-enable").checked);

    if (valid_params == true) {
        let xhr = new XMLHttpRequest();
        xhr.open("GET", "/setscheduleconfig?hour_open=" + hour_open + "&minute_open=" + minute_open + "&hour_close=" + hour_close + "&minute_close=" + minute_close + "&queens_delay=" + queens_delay + "&config_enable=" + config_enable);
        xhr.send();
        console.log("hour_open: " + hour_open + " minute_open: " + minute_open + " hour_close: " + hour_close + " minute_close: " + minute_close + " queens_delay: " + queens_delay + " config_enable: " + config_enable);
        document.getElementById("hive-config-info").innerHTML = "Config updated";
        flashColor("highlight-green")
    } else {
        document.getElementById("hive-config-info").innerHTML = "Input error";
        flashColor("highlight-red");
    }
}

function flashColor(c) {
    let redBox = document.getElementById("save-config");
    redBox.classList.add(c);
    setTimeout(function () {
        redBox.classList.remove(c);
    }, 1000)
}
