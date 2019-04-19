################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

BS = 8

export BS

all: liblfif libppm tools tests

.PHONY: liblfif libppm tools tests clean

liblfif:
	$(MAKE) -C liblfif

libppm:
	$(MAKE) -C libppm

tools:
	$(MAKE) -C tools

clean:
	$(MAKE) clean -C liblfif
	$(MAKE) clean -C libppm
	$(MAKE) clean -C tools

bins:
