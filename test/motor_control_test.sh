#!/usr/bin/env bash

function wait_seconds () {
    echo $((1 + $RANDOM % 5))
}

DRONES_IP="172.23.1.222"

# motor init
curl -X GET "http://$DRONES_IP/mctrl?mcmd=4&steps=300"
sleep 20
# anticlockwise
curl -X GET "http://$DRONES_IP/mctrl?mcmd=-1&steps=600"

while :
do
    # door open
    curl -X GET "http://$DRONES_IP/mctrl?mcmd=2&steps=300"
    sleep 10
    # door close
    curl -X GET "http://$DRONES_IP/mctrl?mcmd=3&steps=300"
    sleep 10
done

exit 0
