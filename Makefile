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

CODEC = $(OBJDIR)/bitstream.o $(OBJDIR)/ppm.o $(OBJDIR)/zigzag.o $(OBJDIR)/file_mask.o

ENCODER = $(OBJDIR)/lfif_encoder.o $(OBJDIR)/compress.o $(CODEC)

DECODER = $(OBJDIR)/lfif_decoder.o $(OBJDIR)/decompress.o $(CODEC)

all: $(BINDIR) $(OBJDIR) $(BINDIR)/ppm2lfif2d $(BINDIR)/lfif2d2ppm $(BINDIR)/ppm2lfif3d $(BINDIR)/lfif3d2ppm $(BINDIR)/ppm2lfif4d $(BINDIR)/lfif4d2ppm

$(BINDIR)/ppm2lfif2d: $(OBJDIR)/ppm2lfif2d.o $(ENCODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif2d2ppm: $(OBJDIR)/lfif2d2ppm.o $(DECODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/ppm2lfif3d: $(OBJDIR)/ppm2lfif3d.o $(ENCODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif3d2ppm: $(OBJDIR)/lfif3d2ppm.o $(DECODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/ppm2lfif4d: $(OBJDIR)/ppm2lfif4d.o $(ENCODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif4d2ppm: $(OBJDIR)/lfif4d2ppm.o $(DECODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR) vgcore* *.data callgrind*
