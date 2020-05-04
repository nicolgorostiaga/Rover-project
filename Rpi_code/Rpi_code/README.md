## Raspberry pi Build Instructions
At reboot all the launcher scripts will be compiled, this includes:
 
 launcher.sh
 
 launcherbracket.sh
  
 launchersensor.sh

Each one of these scripts will compile the following python scripts:
  
  control-rover.py
  
  cambracket.py
  
  humTemp.py

To connect the raspberry pi to the TX2 run the following commands:
  
  $ make controller
  
  $ ./controller
