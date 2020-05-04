# TX2 OS
Download SDK Manager and flash Jetpack 3.3.1. It should be noted that before flashing, the TX2 must be in recovery mode.
The Os will be ubuntu 16.04 LTS and will have Cuda 9.0 and tegra 4.4.38 which is needed for this project.
At cmake ../ download only the image recognition packages, object detection packages, and FCN-Alexnet-cityscapes-HD package. Furthermore, download both python 2.7 and 3.6 when asked.

  $ sudo apt-get update
  
  $ sudo apt-get install git cmake libpython3-dev python3-numpy
  
  $ git clone --recursive https://github.com/dusty-nv/jetson-inference
  
  $ cd jetson-inference
  
  $ mkdir build
  
  $ cd build
  
  $ cmake ../
  
  $ make -j$(nproc)
  
  $ sudo make install
  
  $ sudo ldconfig

## How to Run
Inside the TX2_Node-master folder type:

  $make
  
Navigate to the build directory after running the make command, and type in the following command.

 $ sudo ./tx2_master
