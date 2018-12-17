########################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
# DATUM: 19. 10. 2018
########################################################

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
CFLAGS = -Iinclude -Og -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
LDFLAGS =

all: $(BINDIR) $(OBJDIR) $(BINDIR)/lfif2d_compress $(BINDIR)/lfif2d_decompress

$(BINDIR)/lfif2d_compress: $(OBJDIR)/lfif2d_compress.o $(OBJDIR)/bitstream.o $(OBJDIR)/endian.o $(OBJDIR)/lfif_encoder.o $(OBJDIR)/ppm.o
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif2d_decompress: $(OBJDIR)/lfif2d_decompress.o $(OBJDIR)/bitstream.o $(OBJDIR)/endian.o $(OBJDIR)/lfif_decoder.o $(OBJDIR)/ppm.o
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR) vgcore* *.data
