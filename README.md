# Titan MjpegServer
A very fast MjpgServer library written in c++...
Created by: David Smerkous

## Intro
The Titan MjpegServer was initially written to be for Titan robotics frc 5431.
To be a fast multithreaded server to stream our custom streams from the robot to
the computer(Check out the client: Soon to come). This was built using gcc but
should be compatible cross platform with the help of boost libs

## Requirements
The Titan MjpegServer **requires**

    * C++11 standards to be built in (gcc flag... -std=c++11)
    * Boost libraries 1.54.0 and up (Built in 55)
    * OpenCv 3.10
    * libpthread (Windows might need Cygwin) POSIX threads

## Installation
Here are the steps to install the Titan MjpgServer
    1. Download libs: 


         git clone https://github.com/smerkousdavid/Titan-MjpegServer
    
    2. Copy mjpgserver.cpp and mjpgserver.h into your project:

	
	cd Titan-MjpegServer
	cp mjpgserver.cpp ~/myproject/src
	cp mjpgserver.h ~/myproject/src

    3. Add linkers:
	If building from source you must include all boost libs and all opencv libs (Windows can use world dll*)
        Example g++ build option:


        -s  /usr/lib/x86_64-linux-gnu/libpthread.so /usr/lib/x86_64-linux-gnu/libboost_math_tr1.so /usr/lib/x86_64-linux-gnu/libboost_system.so /usr/lib/x86_64-linux-gnu/libboost_iostreams.so /usr/lib/x86_64-linux-gnu/libboost_regex.so /usr/lib/x86_64-linux-gnu/libboost_signals.so /usr/lib/x86_64-linux-gnu/libboost_thread.so /usr/lib/x86_64-linux-gnu/libboost_locale.so /usr/lib/x86_64-linux-gnu/libboost_timer.so /usr/lib/x86_64-linux-gnu/libboost_atomic.so /usr/lib/x86_64-linux-gnu/libboost_chrono.so /usr/local/lib/libopencv_imgproc.so.3.1.0 /usr/local/lib/libopencv_core.so.3.1.0 /usr/local/lib/libopencv_imgcodecs.so.3.1.0 /usr/local/lib/libopencv_videoio.so.3.1.0 /usr/local/lib/libopencv_features2d.so.3.1.0 /usr/local/lib/libopencv_highgui.so.3.1.0 /usr/local/lib/libopencv_flann.so.3.1.0 /usr/local/lib/libopencv_objdetect.so.3.1.0 /usr/local/lib/libopencv_ml.so.3.1.0 /usr/local/lib/libopencv_shape.so.3.1.0 /usr/local/lib/libopencv_photo.so.3.1.0 /usr/local/lib/libopencv_calib3d.so.3.1.0 /usr/local/lib/libopencv_videostab.so.3.1.0 /usr/local/lib/libopencv_superres.so.3.1.0 /usr/local/lib/libopencv_stitching.so.3.1.0
    4. You're done:
	Just add the mjpgserver.h into your project

## Example
		
	#include "mjpgserver.h" //MjpgServer header

	int main() {
	    MjpgServer server(8081); // Start server on pot 8081
	    server.setQuality(1); // Set jpeg quality to 1 (0 - 100)
	    server.setResolution(1280, 720); // Set stream resolution to 1280x720
	    server.setFPS(15); // Set target fps to 15
	    server.setCapAttach(0); // Attach webcam id 0 to stream
	    server.run(); //Run stream forever (until fatal)
	    return 0;
	}

## License
**Look at license file and sources**
License: MIT License (MIT)
Copyright (c) 2016 David Smerkous

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, 
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 

