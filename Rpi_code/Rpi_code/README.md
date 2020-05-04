## Raspberry pi OS
 Flash ubuntu mate 64 bit.
 
 Use docker and FFMPEG for accessing the camera.

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
  
Since at the end of the semester the LTE stop working we started to work with WIFI.
We assigned a hostname to the raspberry pi called Rover.local which can be used for ssh as
 
  $ssh rover@Rover.local

This hostname is used for the raspberry to connect to the iphone app.
It should be noted that the iphone app just needs an IP address to connect to the device.
