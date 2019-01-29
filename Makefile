################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

all: liblfif libppm tools

.PHONY: liblfif libppm tools clean

liblfif:
	cd liblfif && make

libppm:
	cd libppm && make

tools: liblfif libppm
	cd tools && make

clean:
	cd liblfif && make clean
	cd libppm && make clean
	cd tools && make clean
