#!/usr/bin/env bash

function wait_seconds () {
    echo $((1 + $RANDOM % 5))
}

SLEEP_SECONDS=60
IP="172.23.1.222"

if [ -n "$1" ]
then
    IP="$1"
fi

# motor init
curl -X GET "http://$IP/mctrl?mcmd=4&steps=300"
sleep $SLEEP_SECONDS
# anticlockwise
curl -X GET "http://$IP/mctrl?mcmd=-1&steps=600"

while :
do
    # door open
    curl -X GET "http://$IP/mctrl?mcmd=2&steps=300"
    sleep $SLEEP_SECONDS
    # door close
    curl -X GET "http://$IP/mctrl?mcmd=3&steps=300"
    sleep $SLEEP_SECONDS
done

exit 0
