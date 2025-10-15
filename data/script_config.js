getHiveConfig()
getWifiConfig()
getConfigStatus();

function getHiveConfig() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var msg = JSON.parse(this.responseText);
            console.log(msg);
            hive_type = msg["hivetype"];
            wifi_mode = msg["wifimode"];

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
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var msg = JSON.parse(this.responseText);
            console.log(msg);
            ssid = msg["ssid"];
            pass = msg["pass"];
            ip = msg["ip"];
            gateway = msg["gateway"];
            dns = msg["dns"];

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
    ok = ays();
    if (ok == true) {
        console.log("Reset to default config.")
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/resetdefaultconfig");
        xhr.send();
    }
}

function ays() {
    var answer;
    if (confirm("Are you sure?") == true) {
        answer = true;
    } else {
        answer = false;
    }
    return answer;
}