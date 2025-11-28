window.onload = function() {
    updateNavBarIP();
    getHiveConfig();
    getWifiConfig();
    getDateTime();
    getUptime();
};


function getHiveConfig() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            let hive_type = msg["hive_type"];
            let wifi_mode = msg["wifi_mode"];
            let offset_open_door = msg["offset_open_door"];
            let offset_close_door = msg["offset_close_door"];
            let photoresistor_edge_delta = msg["photoresistor_edge_delta"];

            console.log("hive_type = " + hive_type + " wifi_mode = " + wifi_mode);
            if (hive_type == 0) {
                document.getElementById("cb-hive-type").checked = true;
                document.getElementById("config-hive-type-text").innerHTML = "DRONES";
                document.getElementById("hive-type").value = 0;
                console.log("drones");
            } else {
                document.getElementById("cb-hive-type").checked = false;
                document.getElementById("config-hive-type-text").innerHTML = "QUEENS";
                document.getElementById("hive-type").value = 1;
                console.log("queens");
            }
            if (wifi_mode == 2) {
                document.getElementById("cb-wifi-mode").checked = true;
                document.getElementById("config-wifi-mode-text").innerHTML = "AP";
                document.getElementById("wifi-mode").value = 2;
            } else {
                document.getElementById("cb-wifi-mode").checked = false;
                document.getElementById("config-wifi-mode-text").innerHTML = "STA";
                document.getElementById("wifi-mode").value = 1;
            }

            document.getElementById("offset-open-door").value = offset_open_door;
            document.getElementById("offset-close-door").value = offset_close_door;
            document.getElementById("photoresistor-edge-delta").value = photoresistor_edge_delta;
        }
    }
    xhr.open("GET", "/gethiveconfig", true);
    xhr.send();
}

function getWifiConfig() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            let ssid = msg["ssid"];
            let pass = msg["pass"];
            let ip = msg["ip"];
            let gateway = msg["gateway"];
            let dns = msg["dns"];

            document.getElementById("ssid").value = ssid;
            document.getElementById("pass").value = pass;
            document.getElementById("ip").value = ip;
            document.getElementById("gateway").value = gateway;
            document.getElementById("dns").value = dns;
        }
    }
    xhr.open("GET", "/getwificonfig", true);
    xhr.send();
}

function getUptime() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            let uptime = msg["seconds_since_boot"];

            document.getElementById("uptime").innerHTML = uptime;
        }
    }
    xhr.open("GET", "/secondssinceboot", true);
    xhr.send();
}

function togglehivetype(element) {
    if (element.checked) {
        document.getElementById("config-hive-type-text").innerHTML = "DRONES";
        document.getElementById("hive-type").value = 0;
        console.log("hive-type DRONES");
    } else {
        document.getElementById("config-hive-type-text").innerHTML = "QUEENS";
        document.getElementById("hive-type").value = 1;
        // force WiFi mode always to STA, when hive-type = QUEENS
        document.getElementById("cb-wifi-mode").checked = false;
        document.getElementById("config-wifi-mode-text").innerHTML = "STA";
        document.getElementById("wifi-mode").value = 1;
        console.log("hive-type QUEENS");
    }
}

function togglewifimode(element) {
    if (element.checked) {
        document.getElementById("config-wifi-mode-text").innerHTML = "AP";
        document.getElementById("wifi-mode").value = 2;
        // force hive-type always to DRONES, when WiFi mode = AP
        document.getElementById("cb-hive-type").checked = true;
        document.getElementById("config-hive-type-text").innerHTML = "DRONES";
        document.getElementById("hive-type").value = 0;
        console.log("wifi-mode AP");
    } else {
        document.getElementById("config-wifi-mode-text").innerHTML = "STA";
        document.getElementById("wifi-mode").value = 1;
        console.log("wifi-mode STA");
    }
}

function resetDefaultConfig() {
    let ok = ays();
    if (ok == true) {
        console.log("Reset to default config.")
        let xhr = new XMLHttpRequest();
        xhr.open("GET", "/resetdefaultconfig");
        xhr.send();
    }
}

function setCurrentDateTime() {
    let datetime = new Date();
    let epochseconds = datetime.getTime()
    epochseconds = epochseconds / 1000;
    let utcOffset = document.getElementById("utc-offset").value;
    // UTC offset
    epochseconds = epochseconds + 3600 * utcOffset;
    let xhr = new XMLHttpRequest();
    xhr.open("GET", "/setdatetime?epochseconds=" + epochseconds, true);
    xhr.send();
}

function ays() {
    let answer;
    if (confirm("Are you sure?") == true) {
        answer = true;
    } else {
        answer = false;
    }
    return answer;
}

function updateNavBarIP() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            let dip = msg["drone_ip"];

            document.getElementById("drones-url").setAttribute("href", "http://" + dip + "/drones.html");
            document.getElementById("queens-url").setAttribute("href", "http://" + dip + "/queenshives.html");
        }
    }
    xhr.open("GET", "/getconfigstatus", true);
    xhr.send();
}
