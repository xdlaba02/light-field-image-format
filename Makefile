################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

all: liblfif libppm tools tests

.PHONY: liblfif libppm tools tests clean

liblfif:
	cd liblfif && make

libppm:
	cd libppm && make

tools: liblfif libppm
	cd tools && make

tests: tools liblfif libppm
	cd tests && make


clean:
	cd liblfif && make clean
	cd libppm && make clean
	cd tools && make clean
	cd tests && make clean
