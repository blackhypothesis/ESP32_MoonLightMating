#!/usr/bin/env bash

function wait_seconds () {
    echo $((1 + $RANDOM % 5))
}

DRONES_IP="172.23.1.222"

while :
do
    echo "setdatetime"
    curl -X GET http://$DRONES_IP/setdatetime?epochseconds=$(date +%s)
    echo
    sleep 2

    echo "getdatetime"
    curl -X GET http://$DRONES_IP/getdatetime
    echo
    sleep 2

    echo "getversion"
    curl -X GET http://$DRONES_IP/getversion
    echo
    sleep 2

    echo "gethiveconfig"
    curl -X GET http://$DRONES_IP/gethiveconfig
    echo
    sleep 2

    echo "getwificonfig"
    curl -X GET http://$DRONES_IP/getwificonfig
    echo
    sleep 2

    echo "getclientstates"
    curl -X GET http://$DRONES_IP/getclientstates
    echo
    sleep 2

    echo "getconfigstatus"
    curl -X GET http://$DRONES_IP/getconfigstatus
    echo
    sleep 2

    echo "getconfigstatusclient"
    curl -X GET http://$DRONES_IP/getconfigstatusclient
    echo
    sleep 2

    echo "setscheduleconfig"
    curl -X GET "http://$DRONES_IP/setscheduleconfig?hour_open=15&minute_open=30&hour_close=15&minute_close=45&queens_delay=35&config_enable=1"
    echo
    sleep 2

    echo "secondssinceboot"
    curl -X GET http://$DRONES_IP/secondssinceboot
    echo
    sleep $(wait_seconds)
done

exit 0
