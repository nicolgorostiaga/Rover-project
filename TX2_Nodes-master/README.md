# TX2 OS
Download SDK Manager and flash Jetpack 3.3.1. This is the Cuda 9.0 and tegra 4.4.38 needed for this project.
At cmake ../ download only the image recognition packages, object detection packages, and FCN-Alexnet-cityscapes-HD package.

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
On the TX2, or whatever you are running the TX2 nodes on, navigate to the build directory after 
running the make command, and type in the following command.

$ sudo ./tx2_master
