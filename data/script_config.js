window.onload = function() {
    updateNavBarIP();
    getHiveConfig();
    getWifiConfig();
    getDateTime();
};


function getHiveConfig() {
    let xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            let msg = JSON.parse(this.responseText);
            console.log(msg);
            let hive_type = msg["hivetype"];
            let wifi_mode = msg["wifimode"];

            console.log("hive_type = " + hive_type + " wifi_mode = " + wifi_mode);
            if (hive_type == 0) {
                document.getElementById("cb-hivetype").checked = true;
                document.getElementById("config-hivetype-text").innerHTML = "DRONES";
                document.getElementById("hivetype").value = 0;
                console.log("drones");
            } else {
                document.getElementById("cb-hivetype").checked = false;
                document.getElementById("config-hivetype-text").innerHTML = "QUEENS";
                document.getElementById("hivetype").value = 1;
                console.log("queens");
            }
            if (wifi_mode == 2) {
                document.getElementById("cb-wifimode").checked = true;
                document.getElementById("config-wifimode-text").innerHTML = "AP";
                document.getElementById("wifimode").value = 2;
            } else {
                document.getElementById("cb-wifimode").checked = false;
                document.getElementById("config-wifimode-text").innerHTML = "STA";
                document.getElementById("wifimode").value = 1;
            }
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

function toggleHiveType(element) {
    if (element.checked) {
        document.getElementById("config-hivetype-text").innerHTML = "DRONES";
        document.getElementById("hivetype").value = 0;
        console.log("Hivetype DRONES");
    } else {
        document.getElementById("config-hivetype-text").innerHTML = "QUEENS";
        document.getElementById("hivetype").value = 1;
        // force WiFi mode always to STA, when hivetype = QUEENS
        document.getElementById("cb-wifimode").checked = false;
        document.getElementById("config-wifimode-text").innerHTML = "STA";
        document.getElementById("wifimode").value = 1;
        console.log("HiveType QUEENS");
    }
}

function toggleWifiMode(element) {
    if (element.checked) {
        document.getElementById("config-wifimode-text").innerHTML = "AP";
        document.getElementById("wifimode").value = 2;
        // force hivetype always to DRONES, when WiFi mode = AP
        document.getElementById("cb-hivetype").checked = true;
        document.getElementById("config-hivetype-text").innerHTML = "DRONES";
        document.getElementById("hivetype").value = 0;
        console.log("WiFiMode AP");
    } else {
        document.getElementById("config-wifimode-text").innerHTML = "STA";
        document.getElementById("wifimode").value = 1;
        console.log("WiFiMode STA");
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
