# WMRPController

DIY soldering iron controller for Weller RT tips.

There are many projects of controllers for Weller tips, but most use a DC power supply and PWM power control. This one uses AC power for the heating element. The power is controlled using zero-crossing switching, so EMI is reduced to a minimum.

I used a 3D printed handle for the RT tips, which can be found here: http://kair.us/projects/weller/diy_wmrp_handle/index.html.

This project is a playground for learning C++ on microcontrollers, so don't expect perfectly clean code ;).

### What is already implemented and works well
- AC control using triac with zero-crossing switching
- User interface
- Storing settings in EEPROM
- Storing the last set temperature in EEPROM (uses wear levelling)
- Standby and auto power off


### What needs to be improved
- Temperature measurement. There is a small glitch (drop) in temperature every ~15 seconds
- Temperature control. Currently the tip temperature is controlled by a PID with 200 ms interval. It probably needs better tuning method or there is some bug in the algorithm.


### What is NOT going to be implemented
- Cold junction compensation of the thermocouple. This device is for hobby use only and small difference in the tip temperature is not really a problem. Adding this feature would require additional wire and a temperature sensor, which would make the cable for the handle less flexible.