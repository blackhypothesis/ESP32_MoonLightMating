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
