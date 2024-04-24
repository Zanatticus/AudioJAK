# Source files
SRCS = player.c audio_player.c ../hdmi/include/hdmi.c ../hdmi/include/audiovisuals.c ../hdmi/include/visualizer.c ../hdmi/include/spectrogram.c
TARGET = player

PYTHON_VERSION := $(shell python3 --version | cut -d " " -f2 | cut -d "." -f1-2)

ifeq ($(PYTHON_VERSION),3.11)
    PYTHON_INCLUDE := -I python3.11
	PYFLAG := -lpython3.11
else ifeq ($(PYTHON_VERSION),3.10)
    PYTHON_INCLUDE := -I python3.10
	PYFLAG := -lpython3.10
else
    $(error Unsupported Python version: $(PYTHON_VERSION))
endif

FLAGS = -g -c -Wall
LIBS = 
LFLAGS = 
LZED:=#-lzed
LALSA:= -lasound -lpthread -lrt -ldl -lm
INCLUDES = -I /usr/include/alsa/ -I /usr/include/libdrm -I include -I include -I /home/jared/eece4534sdk/sysroots/cortexa9t2hf-neon-esl-linux-gnueabi/usr/include/libpng16
OBJS = $(SRCS:.c=.o)

include ../hdmi/include/zed.mk
.PHONY: clean

all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LALSA) $(LZED) $(LFLAGS) $(PYFLAG) $(LIBS) 

# Rule to build object files from source files
%.o: %.c
	$(CC) $(FLAGS) $(INCLUDES) $(PYTHON_INCLUDE) -o $@ $<

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

install:
	sudo apt-get -y install libasound2-dev
	sudo apt-get install libdrm-dev
	sudo apt-get install libpng-dev

installPython:
	echo "Not implemented yet..."