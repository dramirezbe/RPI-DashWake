#Core/main/main.py

import RPi.GPIO as GPIO
import time
import global_vars

from main import button_interrupt

BTN_PIN = 17
DEBOUNCE_TIME_MS = 250

def main():
    
    try:
        button_interrupt.setup_button_interrupt(pin=BTN_PIN, bouncetime=DEBOUNCE_TIME_MS) #Configure button interrupt
        print("-------------System Configured-----------------")

        print("--------------Firmware Ready------------------")
        while True:
        
            # Here other tasks
            if (global_vars.is_alarm_active):
                # alarm_callback()
                pass
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n(Ctrl+C)Leaving program...")
    finally:
        
        button_interrupt.cleanup_gpio()
        print("---------Program finished---------------")

if __name__ == "__main__":
    main()