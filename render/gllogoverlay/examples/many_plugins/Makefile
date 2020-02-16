PLUGINS_INSTALL_DIR = `pkg-config --variable=pluginsdir gstreamer-1.0`

#CFLAGS = -Wall -O3 -fPIC -c -march=native -g
CFLAGS = -Wall -O0 -fPIC -c -g
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-audio-1.0 gstreamer-gl-1.0`

LDFLAGS = -lm
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-audio-1.0 gstreamer-gl-1.0 fontconfig` -lgstapp-1.0

all: clean build

clean: clean_build_dir

install: clean build

build: main.o
	@$(CC) $(LDFLAGS) main.o -o testfilterapp

main.o:
	@$(CC) $(CFLAGS) main.c -o main.o

clean_build_dir: 
	rm -f testfilterapp
	rm -f main.o
