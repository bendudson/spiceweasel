
OPENJPEG=/hwdisks/home/bd512/local/
#OPENJPEG = /home/bdudson/mast_video/openjpeg/
#OPENJPEG = ../openjpeg/
#OPENJPEG = ../openjpeg/dist/

CC = gcc -c
LD = gcc -o
CFLAGS = -Wall -g
INCLUDES = -I$(OPENJPEG)/include
LIBS = -lpthread -lpng -L$(OPENJPEG)/lib -lopenjpeg

OBJS = spiceweasel.o io_png.o io_bmp.o process_frames.o read_main.o io_ipx.o \
       process_script.o parse_nextline.o run_script.o
TARGET = spiceweasel

.PHONY: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(TARGET) $(OBJS) $(LIBS) 

$(OBJS): spiceweasel.h
io_bmp.o: io_bmp.h
io_ipx.o: io_ipx.h
process_script.o run_script.o: script.h

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

.PHONY: clean

clean: 
	rm $(OBJS) $(TARGET)
