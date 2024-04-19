# Source files
SRCS = test.c include/hdmi.c include/audiovisuals.c include/visualizer.c include/spectrogram.c
TARGET = test

FLAGS = -g -c -Wall
LIBS = 
LFLAGS = 
LZED:=#-lzed
LALSA:= -lasound -lpthread -lrt -ldl -lm -lpython3.10 
INCLUDES = -I /usr/include/alsa/ -I /usr/include/libdrm -I include -I include -I python3.10 -I /home/jared/eece4534sdk/sysroots/cortexa9t2hf-neon-esl-linux-gnueabi/usr/include/libpng16
OBJS = $(SRCS:.c=.o)

include include/zed.mk
.PHONY: clean

all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LALSA) $(LZED) $(LFLAGS) $(LIBS) 

# Rule to build object files from source files
%.o: %.c
	$(CC) $(FLAGS) $(INCLUDES) -o $@ $<

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

install:
	sudo apt-get -y install libasound2-dev
	sudo apt-get install libdrm-dev
	sudo apt-get install libpng-dev

installPython:
	echo "Not implemented yet..."