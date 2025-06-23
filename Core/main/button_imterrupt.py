#Core/main/button_interrupt.py

import RPi.GPIO as GPIO

import global_vars

def button_callback(channel):
    
    print(f"Button interrupt in{channel})")
    
    global_vars.is_alarm_active = not global_vars.is_alarm_active


def setup_button_interrupt(pin, bouncetime=200):
    """
    Configure interruption
    """
    GPIO.setmode(GPIO.BCM)  
    GPIO.setup(pin, GPIO.IN) #Configure input

    GPIO.add_event_detect(pin, GPIO.FALLING, callback=button_callback, bouncetime=bouncetime)
    print(f"Interruption init in GPIO {pin}.")

def cleanup_gpio():
    """
    Clean GPIO
    """
    GPIO.cleanup()
    print("GPIO pin cleaned.")