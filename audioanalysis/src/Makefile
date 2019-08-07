CC = gcc

CFLAGS = -Wall -O3 -std=gnu99 -fPIC -c -march=native -g
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-audio-1.0`

LDFLAGS = -shared -lm
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-audio-1.0 glib-2.0`

all: audioanalysis.o ebur.o ebur_blocklist.o error.o
	@$(CC) $(LDFLAGS) gstaudioanalysis.o ebur.o ebur_blocklist.o error.o -o ../../build/libaudioanalysis.so

audioanalysis.o:
	@$(CC) $(CFLAGS) gstaudioanalysis.c -o gstaudioanalysis.o

error.o:
	@$(CC) $(CFLAGS) error.c -o error.o

ebur.o:
	@$(CC) $(CFLAGS) ebur128.c -o ebur.o

ebur_blocklist.o:
	@$(CC) $(CFLAGS) ebur128_blocklist.c -o ebur_blocklist.o

clean:
	rm *.o
