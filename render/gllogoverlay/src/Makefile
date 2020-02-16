PLUGINS_INSTALL_DIR = `pkg-config --variable=pluginsdir gstreamer-1.0`

#CFLAGS = -Wall -O3 -fPIC -c -march=native -g
CFLAGS = -Wall -O0 -fPIC -c -g
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-audio-1.0 gstreamer-gl-1.0`

LDFLAGS = -shared -lm
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-audio-1.0 gstreamer-gl-1.0 fontconfig`

all: clean build

clean: create_build_dir clean_build_dir

install: clean build copy_lib_to_pluginsdir


build: gstgllogoverlay.o gldrawing.o errorshandler.o
	@$(CC) $(LDFLAGS) ../build/gstgllogoverlay.o ../build/gldrawing.o ../build/errorshandler.o -o ../build/libgstgllogoverlay.so

gstgllogoverlay.o:
	@$(CC) $(CFLAGS) gstgllogoverlay.c -o ../build/gstgllogoverlay.o

gldrawing.o:
	@$(CC) $(CFLAGS) gldrawing.c -o ../build/gldrawing.o

errorshandler.o:
	@$(CC) $(CFLAGS) errorshandler.c -o ../build/errorshandler.o

create_build_dir: 
	mkdir -p ../build
clean_build_dir:
	rm -f ../build/gstgllogoverlay.o
	rm -f ../build/gldrawing.o
	rm -f ../build/errorshandler.o
	rm -f ../build/libgstgllogoverlay.so

copy_lib_to_pluginsdir:
	yes | cp -rf ../build/libgstgllogoverlay.so $(PLUGINS_INSTALL_DIR)


