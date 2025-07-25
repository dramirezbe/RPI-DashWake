#!/bin/bash

# Detect the script's own directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

SERVER_DIR=$SCRIPT_DIR/server # Corrected
CORE_DIR=$SCRIPT_DIR/Core   # Corrected

echo "------------Init installation----------------"

echo "---Installing Dependencies---"

sudo apt install wiringpi libcjson1 libcjson-dev libcurl4-openssl-dev python3 python3.12-venv -y

cd $SERVER_DIR

echo "---Installing Server---"

sudo python3 -m venv venv

source venv/bin/activate

pip install -r requirements.txt

deactivate

echo "---Done---"

echo "------------Installation Completed-----------"



