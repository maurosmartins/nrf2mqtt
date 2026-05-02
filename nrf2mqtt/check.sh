#!/bin/bash

#sleep 30

SCREEN_SESSION="nrf"
PYTHON_SCRIPT="/home/pi/nrf2mqtt/nrf2mqtt.py"

# Check if a 'screen' session named 'nrf' is running
if screen -ls | grep -q "\.$SCREEN_SESSION\s"; then
    echo "The '$SCREEN_SESSION' screen session is already running."
else
    echo "The '$SCREEN_SESSION' screen session is not running. Launching a new session..."

    # Launch a new 'screen' session named 'nrf' with the specified Python script
    screen -S $SCREEN_SESSION -dm python3 $PYTHON_SCRIPT
    
    echo "The '$SCREEN_SESSION' screen session has been launched."
fi
