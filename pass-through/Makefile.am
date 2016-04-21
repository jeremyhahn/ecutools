AUTOMAKE_OPTIONS = foreign

CWEBSOCKET_SUBPROTOCOL_SRC = src/cwebsocket/subprotocol/echo/echo_client.h src/cwebsocket/subprotocol/echo/echo_client.c
CWEBSOCKET_SRC = src/cwebsocket/utf8.h src/cwebsocket/utf8.c src/cwebsocket/common.h src/cwebsocket/common.c src/cwebsocket/client.h src/cwebsocket/client.c

bin_PROGRAMS = ecutune
ecutune_SOURCES = $(CWEBSOCKET_SUBPROTOCOL_SRC) $(CWEBSOCKET_SRC) src/uds.h src/uds.c src/canbus.h src/canbus.c src/wcbridge.h src/wcbridge.c src/ecutune.c

AM_CFLAGS =
AM_LDFLAGS = 
#AM_LDFLAGS = -lwsclient

if DEBUG
  AM_CFLAGS += -Wall -g3
else
  AM_CFLAGS += -Wall -O3
endif

if THREADED
  AM_CFLAGS += -pthread -DTHREADED
  AM_LDFLAGS += -lpthread
endif

if USESSL
  AM_CFLAGS += -DUSESSL
  AM_LDFLAGS += -lssl
endif

clean:
	rm -rf config.h.in config.h config.cache configure install-sh aclocal.m4 autom4te.cache/ config.log config.status Debug/ depcomp .deps/ m4/ Makefile Makefile.in missing stamp-h1 *.o src/*.o *~ ecutune
