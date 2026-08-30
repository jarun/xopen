# chndlr - simple xdg-open replacement with fallback

include config.mk

SRC = chndlr.c

all: options chndlr

options:
	@echo chndlr build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

chndlr: ${SRC} config.mk config.h
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f chndlr

install: all
	@cp -f chndlr /usr/bin/xdg-open
	@chmod 755 /usr/bin/xdg-open

uninstall:
	-rm /usr/bin/xdg-open
