function getConfigStatus() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
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
