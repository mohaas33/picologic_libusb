#! /usr/bin/bash

# check if the server is running and launch it if it is not...
SERVICE="./pa_server"
if  pgrep -x "$SERVICE" >/dev/null
then
    echo "server was already running"
else
    echo "server was stopped...starting now"
    ./pa_server &
    sleep 1
fi
