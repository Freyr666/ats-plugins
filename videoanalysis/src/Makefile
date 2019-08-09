CC = gcc

CFLAGS = -Wall -O3 -std=gnu11 -fPIC -Wall -c -g
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 gl`

LDFLAGS = -shared -Wall
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 glib-2.0 gl`

PY=python3

all: error.o videoanalysis.o
	@$(CC) $(LDFLAGS) error.o videoanalysis.o -o ../../build/libvideoanalysis.so

error.o:
	@$(CC) $(CFLAGS) error.c -o error.o

videoanalysis.o: analysis.h
	@$(CC) $(CFLAGS) gstvideoanalysis.c -o videoanalysis.o
clean:
	rm *.o
