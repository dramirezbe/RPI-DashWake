#!/bin/bash

# Detect the script's own directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

CAPTIVE_DIR = $SCRIPT_DIR/captive_portal

FRONT_DIR = $SCRIPT_DIR/frontend

echo "------------Core Init installation----------------"

echo "---Installing WiringPi---"

sudo apt install wiringpi libcjson1 libcjson-dev libcurl4-openssl-dev -y


echo "---Done---"

echo "------------Core Installation Completed-----------"



