# xopen - simple xdg-open replacement with fallback

include config.mk

SRC = xopen.c

all: options xopen

options:
	@echo xopen build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

xopen: ${SRC} config.mk config.h
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS} $(LDLIBS)

clean:
	@echo cleaning
	@rm -f xopen

install: all
	@cp -f xopen /usr/local/bin/xopen
	@chmod 755 /usr/local/bin/xopen

uninstall:
	-rm /usr/local/bin/xopen
