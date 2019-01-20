################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

all: liblfif libppm tools

.PHONY: liblfif libppm tools clean doc prez

liblfif:
	cd liblfif && make

libppm:
	cd libppm && make

tools:
	cd tools && make

clean:
	cd liblfif && make clean
	cd libppm && make clean
	cd tools && make clean
	cd doc/dokumentace && make clean
	cd doc/prezentace && make clean

doc:
	cd doc/dokumentace && make

prez:
	cd doc/prezentace && make xdlaba02.pdf
