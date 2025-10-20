window.onload = function() {
    getClientStates();
};

function handleNavBar() {
    let x = document.getElementById("myTopnav");
    if (x.className === "topnav") {
        x.className += " responsive";
    }
    else {
        x.className = "topnav";
    }
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
                let row1 = table.insertRow();
                let row1_key = row1.insertCell(0);
                let row1_value = row1.insertCell(1);
                row1_key.className = "queenshive-ip";
                row1_value.className = "queenshive-ip";
                row1_key.innerHTML = "IP";
                row1_value.innerHTML = "<a href=\"http://" + item.ip + "\" target=\"_self\">" + item.ip + "<\a>";

                let row2 = table.insertRow();
                let row2_key = row2.insertCell(0);
                let row2_value = row2.insertCell(1);
                row2_key.innerHTML = "MAC";
                row2_value.innerHTML = item.mac;

                let row3 = table.insertRow();
                let row3_key = row3.insertCell(0);
                let row3_value = row3.insertCell(1);
                row3_key.innerHTML = "#";
                row3_value.innerHTML = item.active;

                let row4 = table.insertRow();
                let row4_key = row4.insertCell(0);
                let row4_value = row4.insertCell(1);
                row4_key.innerHTML = "WiFi config sent";
                row4_value.innerHTML = item.wifi_config_sent;

                let row5 = table.insertRow();
                let row5_key = row5.insertCell(0);
                let row5_value = row5.insertCell(1);
                row5_key.innerHTML = "Last update [s]";
                row5_value.innerHTML = item.seconds;
            });
        }
    }
    xhr.open("GET", "/getclientstates", true);
    xhr.send();
}