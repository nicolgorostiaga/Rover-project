library_includes = -I/usr/include/gstreamer-1.0\
		   -I/usr/lib/aarch64-linux-gnu/gstreamer-1.0/include\
		   -I/usr/include/glib-2.0\
		   -I/usr/include/libxml2\
		   -I/usr/lib/aarch64-linux-gnu/glib-2.0/include\
		   -I/usr/local/include/jetson-utils\
		   -I/usr/local/include/jetson-inference

jetson_libs = -rdynamic /usr/local/cuda-9.0/lib64/libcudart_static.a -lpthread -ldl -lrt /usr/local/lib/libjetson-inference.so /usr/local/lib/libjetson-utils.so /usr/local/cuda-9.0/lib64/libcudart_static.a -lpthread -ldl -lrt -lGL -lGLEW -lgstreamer-1.0 -lgstapp-1.0 -lnvinfer -lnvinfer_plugin -lnvcaffe_parser -Wl,-rpath,/usr/local/lib

common_obj = objects/Messages.o objects/SharedMem.o
common_inc = include/Messages.h include/SharedMem.h

all : tx2_master\
      tx2_can_node\
      tx2_comm_node\
      tx2_cam_node\
      tx2_nav_node\
      tx2_gps_node\
      tx2_gyro_node\
      controller\
      logWriter

tx2_master : objects/tx2_master.o\
	     objects/Messages.o\
	     objects/Command.o
	gcc -o build/tx2_master\
	       objects/tx2_master.o\
	       objects/Messages.o\
	       objects/Command.o

objects/tx2_master.o : src/tx2_master.c\
	               include/Messages.h\
		       include/Command.h
	gcc -c -o objects/tx2_master.o\
		  src/tx2_master.c

objects/Command.o: src/Command.c\
	           include/Command.h\
		   include/Messages.h
	gcc -c -o objects/Command.o\
		  src/Command.c

tx2_can_node : objects/tx2_can_node.o\
	       objects/CanController.o\
	       objects/Messages.o
	gcc -o build/tx2_can_node\
	       objects/tx2_can_node.o\
	       objects/CanController.o\
	       objects/Messages.o

objects/tx2_can_node.o : src/tx2_can_node.c\
	                 include/CanController.h\
			 include/Messages.h
	gcc -c -o objects/tx2_can_node.o\
		  src/tx2_can_node.c

objects/CanController.o : src/CanController.c\
	                  include/CanController.h\
			  include/Messages.h
	gcc -c -o objects/CanController.o\
		src/CanController.c

tx2_comm_node : objects/tx2_comm_node.o\
	        objects/CommController.o\
		objects/Messages.o
	gcc -o build/tx2_comm_node\
	       objects/tx2_comm_node.o\
	       objects/CommController.o\
	       objects/Messages.o

objects/tx2_comm_node.o : src/tx2_comm_node.c\
	                  include/CommController.h\
			  include/Messages.h
	gcc -c -o objects/tx2_comm_node.o\
		  src/tx2_comm_node.c

objects/CommController.o : src/CommController.c\
	                   include/CommController.h\
			   include/Messages.h
	gcc -c -o objects/CommController.o\
		  src/CommController.c

tx2_cam_node : objects/tx2_cam_node.o\
	       objects/Messages.o\
	       objects/SharedMem.o
	g++ -o build/tx2_cam_node\
	       objects/tx2_cam_node.o\
	       objects/Messages.o\
	       objects/SharedMem.o\
	       ${jetson_libs}\
	       -lrt

objects/tx2_cam_node.o : src/tx2_cam_node.cpp\
	                 include/Messages.h
	nvcc -c ${library_includes}\
		-std=c++11\
		-o objects/tx2_cam_node.o\
		src/tx2_cam_node.cpp

objects/tx2_nav_node.o : src/tx2_nav_node.c\
			 include/Messages.h\
			 include/SharedMem.h\
			 include/protocol.h
	gcc -c -o objects/tx2_nav_node.o\
		  src/tx2_nav_node.c

tx2_nav_node : objects/tx2_nav_node.o\
	       objects/Messages.o\
	       objects/SharedMem.o\
	       objects/LatLonTrig.o\
	       objects/FilterGen.o\
	       objects/Parameters.o
	gcc -o build/tx2_nav_node\
		objects/tx2_nav_node.o\
		objects/Messages.o\
		objects/SharedMem.o\
		objects/LatLonTrig.o\
		objects/FilterGen.o\
		objects/Parameters.o -lrt -lm

objects/tx2_gps_node.o : src/tx2_gps_node.c\
	                 include/Messages.h\
			 include/I2CGPS.h\
			 include/SharedMem.h
	gcc -c -o objects/tx2_gps_node.o\
		src/tx2_gps_node.c

tx2_gps_node : objects/tx2_gps_node.o\
	       objects/Messages.o\
	       objects/I2CGPS.o\
	       objects/SharedMem.o
	gcc -o build/tx2_gps_node\
	       objects/tx2_gps_node.o\
	       objects/Messages.o\
	       objects/I2CGPS.o\
	       objects/SharedMem.o -lrt

objects/Messages.o : src/Messages.c\
	             include/Messages.h
	gcc -c -o objects/Messages.o\
		  src/Messages.c

objects/SharedMem.o : src/SharedMem.c\
	              include/SharedMem.h
	gcc -c -o objects/SharedMem.o\
		  src/SharedMem.c

objects/I2CGPS.o : src/I2CGPS.c\
	           include/Messages.h\
		   include/I2CGPS.h
	gcc -c -o objects/I2CGPS.o\
		  src/I2CGPS.c

objects/LatLonTrig.o : src/LatLonTrig.c\
	               include/LatLonTrig.h
	gcc -c -o objects/LatLonTrig.o\
		  src/LatLonTrig.c

objects/FilterGen.o : src/FilterGen.c\
	              include/FilterGen.h
	gcc -c -o objects/FilterGen.o\
		  src/FilterGen.c

objects/Parameters.o : src/Parameters.c\
	               include/Parameters.h
	gcc -c -o objects/Parameters.o\
		  src/Parameters.c

objects/I2CGyro.o : src/I2CGyro.c\
	            include/Messages.h\
		    include/I2CGyro.h
	gcc -c -o objects/I2CGyro.o\
		  src/I2CGyro.c

objects/tx2_gyro_node.o : src/tx2_gyro_node.c\
	                  include/Messages.h\
			  include/I2CGyro.h\
			  include/SharedMem.h
	gcc -c -o objects/tx2_gyro_node.o\
		  src/tx2_gyro_node.c

tx2_gyro_node : objects/tx2_gyro_node.o\
	        objects/Messages.o\
		objects/I2CGyro.o\
		objects/SharedMem.o
	gcc -o build/tx2_gyro_node\
	       objects/tx2_gyro_node.o\
	       objects/Messages.o\
	       objects/I2CGyro.o\
	       objects/SharedMem.o -lrt

controller : controller.c\
	     logWriter.c\
	     include/Messages.h
	gcc -o controller controller.c
	$(MAKE) logWriter

logWriter : logWriter.c\
	objects/Messages.o
	gcc -o logWriter\
	       logWriter.c\
	       objects/Messages.o

clean :
	rm objects/* controller logWriter
