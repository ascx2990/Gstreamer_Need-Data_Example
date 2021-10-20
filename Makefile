MAKEFILE = Makefile

####### Compiler, tools and options
PKGS:= gstreamer-1.0
LIBS:= `pkg-config --libs $(PKGS)`


ifeq ($(BITS), 64)
	# ifeq ($(CPU),X86)
	# 	CXX = g++
	# 	LINK = g++
	# 	TARGET = test_x86
	# 	LIBS += -L./3rdparty/opencv/TX2/lib/ -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_video -lopencv_videoio
	# endif
	ifeq ($(CPU), NVIDIA)
		CXX = g++
		LINK = g++
		TARGET = nvidia_tx2
		LIBS += -L./3rdparty/opencv/TX2/lib/ -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_video -lopencv_videoio
	endif
	# ifeq ($(CPU), ARMHF)
	# 	CXX =  aarch64-linux-gnu-g++
        #  LINK =  aarch64-linux-gnu-g++
        #  TARGET = test_arm64
        #  LIBS = -L../eSPDI -leSPDI_ARMHF_64_tiny
	# endif
endif


CFLAGS = -Wall -W -std=c++11
LFLAGS = -Wall -W 
LIBS += -lpthread

OBJECTS = main.o
INCPATH = -I./3rdparty -I./3rdparty/opencv/TX2/include

#INCPATH+= `pkg-config --cflags $(PKGS)`
INCPATH+= $(shell pkg-config --cflags $(PKGS))
####### Implicit rules

.SUFFIXES: .o .cpp

.cpp.o:
	@echo $(CXX) -c $(CFLAGS) $(INCPATH) -g -o $@ $<
	$(CXX) -c $(CFLAGS) $(INCPATH) -g -o $@ $<
	

all: $(TARGET)

####### Build rules
$(TARGET): $(OBJECTS)
	$(LINK) $(LFLAGS) -g -o $@ $^ $(LIBS) 
	

####### clean rules
clean: 
	rm $(OBJECTS) test_*
