var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);
var direction;
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
    var msg = JSON.parse(event.data);
    direction = msg["direction"];
    var motor_element = "motor-" + msg["motor_nr"] + "-state";
    if (direction == 0){ 
      document.getElementById(motor_element).innerHTML = "Motor stopped."
      document.getElementById(motor_element).style.color = "red";
    }
    else if(direction == 1) {
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
    var x = document.getElementById("myTopnav");
    if (x.className === "topnav") {
        x.className += " responsive";
    }
    else {
        x.className = "topnav";
    }
}

function runMotor(){
    const rbs = document.querySelectorAll('input[name="direction"]');
    direction;
    for (const rb of rbs) {
        if (rb.checked) {
            direction = rb.value;
            break;
        }
    }
    
    var steps = document.getElementById("steps").value;
    var msg = '{"steps":' + steps + ',"direction":' + direction + '}';
    console.log(msg);
    websocket.send(msg);
}

function getDateTime() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            console.log(myObj);
            var datetime = myObj["datetime"];
            document.getElementById("datetime").innerHTML = datetime;
        }
    }
    xhr.open("GET", "/getdatetime", true);
    xhr.send();
}

function secondsToHMS(secs)
{
    var hours = Math.floor(secs / (60 * 60));

    var divisor_for_minutes = secs % (60 * 60);
    var minutes = Math.floor(divisor_for_minutes / 60);

    var divisor_for_seconds = divisor_for_minutes % 60;
    var seconds = Math.ceil(divisor_for_seconds);

    (hours < 10) ? hours = "0" + hours : hours;
    (minutes < 10) ? minutes = "0" + minutes : minutes;
    (seconds < 10) ? seconds = "0" + seconds : seconds;

    return hours + ":" + minutes + ":" + seconds;
}

function getVersion() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var msg = JSON.parse(this.responseText);
            console.log(msg);
            document.getElementById("version").innerHTML = `Version: ${msg["version"]}`;
        }
    }
    xhr.open("GET", "/getversion", true);
    xhr.send();
}

function getConfigStatus() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var msg = JSON.parse(this.responseText);
            console.log(msg);
            dip = msg["drone_ip"];
            hdo = msg["hour_door_open"];
            mdo = msg["minute_door_open"];
            hdc = msg["hour_door_close"];
            mdc = msg["minute_door_close"];
            sdo = msg["seconds_till_door_open"];
            sdc = msg["seconds_till_door_close"];
            qd = msg["queens_delay"];
            en = msg["config_enable"];

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
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var msg = JSON.parse(this.responseText);
            console.log(msg);

            const table = document.getElementById("client-state");
            table.innerHTML = "";
            msg.forEach( item => {
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
    var datetime = new Date();
    var epochseconds = datetime.getTime()
    epochseconds = epochseconds / 1000;
    // UTC to GMT+1
    epochseconds = epochseconds + 3600;
    // Summertime
    // epochseconds = epochseconds + 3600;
    var xhr = new XMLHttpRequest();
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

function setHiveConfig() {
    var valid_params = true;
    var hour_open = document.getElementById("hour-door-open").value;
    var minute_open = document.getElementById("minute-door-open").value;
    var hour_close = document.getElementById("hour-door-close").value;
    var minute_close = document.getElementById("minute-door-close").value;
    var queens_delay = document.getElementById("queens-delay").value;
    var config_enable = 0;

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
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/sethiveconfig?hour_open=" + hour_open + "&minute_open=" + minute_open + "&hour_close=" + hour_close + "&minute_close=" + minute_close + "&queens_delay=" + queens_delay + "&config_enable=" + config_enable);
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
    var redBox = document.getElementById("save-config");
    redBox.classList.add(c);
    setTimeout(function(){
        redBox.classList.remove(c);
    }, 1000)
}
