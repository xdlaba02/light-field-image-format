########################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
# DATUM: 19. 10. 2018
########################################################

TARGETS = lfif2d_compress

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
CFLAGS = -Iinclude -Og -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
LDFLAGS =

all: $(BINDIR) $(OBJDIR) $(BINDIR)/$(TARGETS)

$(BINDIR)/lfif2d_compress: $(OBJDIR)/lfif2d_compress.o $(OBJDIR)/bitstream.o $(OBJDIR)/endian.o $(OBJDIR)/lfif_encoder.o $(OBJDIR)/ppm.o
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR) vgcore*
