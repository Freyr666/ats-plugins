CC = gcc

CFLAGS = -Wall -O3 -march=native -fPIC -Wall -c 
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-audio-1.0 gstreamer-pbutils-1.0`

LDFLAGS = -shared -Wall
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 glib-2.0 gstreamer-pbutils-1.0`

all: soundbar.o
	@$(CC) $(LDFLAGS) soundbar.o -o ../../../build/libsoundbar.so

soundbar.o: render.h
	@$(CC) $(CFLAGS) gstsoundbar.c -o soundbar.o

clean:
	rm *.o
