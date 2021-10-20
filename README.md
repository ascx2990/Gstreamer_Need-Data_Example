# Gstreamer_Need-Data_Example
An example that how to use appsrc push image data.


## Why do I do this project?
I have a problem at work. I need to put the image into appsrc through an open source project.Because the camera needs to be set through the open source.
## Introduction
The preject use "cv.imread(image)" data  put the image into appsrc. 
There are two sink in this project. The sink of "xvimagesink" show image view. The sink of "appsink" can get image and save. 

You can change the fps through 'gst_util_uint64_scale_int'.
Example below:

''' sh 
GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 1); // 1 fps
'''

''' sh 
GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // 30 fps
'''



Put buffer in 'cb_need_data'. This project use cvLoadImage get image buffer ,you can alse change cv::videocapture to get image buffer.


## RUN 
Compile : make VER=1.0.0 CPU=NVIDIA BITS=64

RUN     : ./run.sh
