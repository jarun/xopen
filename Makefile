# xopen - simple xdg-open replacement with fallback

include config.mk

SRC = xopen.c

O_MAGIC := 0  # do not link with libmagic by default

ifeq ($(strip $(O_MAGIC)),1)
	CPPFLAGS += "-DMAGIC=1"
	LDLIBS += "-lmagic"
endif

all: options xopen

options:
	@echo xopen build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

xopen: ${SRC} config.mk config.h
	${CC} ${CPPFLAGS} ${CFLAGS} -o $@ $< ${LDFLAGS} $(LDLIBS)

clean:
	@echo cleaning
	@rm -f xopen

install: all
	@cp -f xopen /usr/local/bin/xopen
	@chmod 755 /usr/local/bin/xopen

uninstall:
	-rm /usr/local/bin/xopen

.PHONY: all xopen install uninstall clean
