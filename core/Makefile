LIB=psyc_core
SRCS=psyc-core.c psyc-servers.c psyc-queries.c psyc-channels.c
OBJS=$(SRCS:.c=.o)
IRSSI_INCLUDE=../irssi-0.8.12

CFLAGS+=	-fPIC \
		-std=c99 -Wall -W \
		-I${IRSSI_INCLUDE} \
		-I${IRSSI_INCLUDE}/src \
		-I${IRSSI_INCLUDE}/src/core \
		-I/usr/include/glib-2.0 \
		-I/usr/lib/glib-2.0/include \
		-DUOFF_T_LONG -ggdb -DDEBUG
LDFLAGS+=	-shared
all: $(LIB)

$(LIB): $(OBJS)
	$(CC) -shared -o lib$@.so $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean user-install

user-install:
	install -d ~/.irssi/modules
	install lib$(LIB).so ~/.irssi/modules

install: user-install

clean:
	rm -rf *.o *.so *.core
