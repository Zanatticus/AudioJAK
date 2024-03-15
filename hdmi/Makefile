# Source files
SRCS = test.c include/hdmi.c
TARGET = test

FLAGS = -g -c -Wall
LIBS = 
LFLAGS = 
LZED:=#-lzed
LALSA:= -lasound -lpthread -lrt -ldl -lm -ldrm
INCLUDES = -I /usr/include/alsa/ -I /usr/include/libdrm
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
	sudo apt-get install libdrm-dev