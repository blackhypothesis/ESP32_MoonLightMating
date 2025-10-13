getHiveConfig()
getWifiConfig()

function getHiveConfig() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var msg = JSON.parse(this.responseText);
            console.log(msg);
            hive_type = msg["hivetype"];
            wifi_mode = msg["wifimode"];

            document.getElementById("hivetype").value = hive_type;
            document.getElementById("wifimode").value = wifi_mode;
        }
    }
    xhr.open("GET", "/gethiveconfig", true);
    xhr.send();
}

function getWifiConfig() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
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