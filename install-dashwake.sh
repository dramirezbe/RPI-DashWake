#!/bin/bash

VENV_NAME="core-venv"

echo "------------Core Init installation----------------"

# 1. Verify python3 & pip3

if ! command -v python3 &> /dev/null
then
    echo "python3 not found, first install it with:"
    echo "  sudo apt update"
    echo "  sudo apt install python3 python3-venv python3-pip"
    exit 1
fi

if ! command -v pip3 &> /dev/null
then
    echo "pip3 not found, first install it with:"
    echo "  sudo apt install python3-pip"
    exit 1
fi

# 2. Create venv if not exists
if [ ! -d "$VENV_NAME" ]; then
    echo "Creating venv '$VENV_NAME'..."
    python3 -m venv "$VENV_NAME"
    if [ $? -ne 0 ]; then
        echo "Error creating venv, exiting."
        exit 1
    fi
else
    echo "Venv '$VENV_NAME' already exists"
fi

source "$VENV_NAME"/bin/activate
echo "Venv ready"

if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt
    if [ $? -ne 0 ]; then
        echo "Error installing dependencies from requirements.txt."
        deactivate
        exit 1
    fi
else
    echo "requirements.txt not found, skipping installation."
fi

deactivate

echo "------------Core Installation Completed-----------"


echo "To execute Core program, use:"
echo "source $VENV_NAME/bin/activate"
echo "python3 main.py"


