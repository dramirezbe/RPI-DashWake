#!/bin/bash

# Detect the script's own directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

CAPTIVE_DIR = $SCRIPT_DIR/captive_portal

FRONT_DIR = $SCRIPT_DIR/frontend

echo "------------Core Init installation----------------"

echo "---Installing WiringPi---"

sudo apt install wiringpi

echo "---Done---"


echo "---Installing Node.js & pnpm---"

# Download and install nvm:
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | bash

# in lieu of restarting the shell
\. "$HOME/.nvm/nvm.sh"

# Download and install Node.js:
nvm install 22.13.1

# Download and install pnpm:
corepack enable pnpm


echo "---Done---"

echo "---Installing captive portal---"

cd $CAPTIVE_DIR

pnpm i
pnpm run build

cd $FRONT_DIR

pnpm i
pnpm run build


echo "------------Core Installation Completed-----------"



