########################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
# DATUM: 22. 12. 2018
########################################################

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
CFLAGS = -Iinclude -Og -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -fPIC
LDFLAGS = -shared

TARGET = $(BINDIR)/liblfif.so $(BINDIR)/liblfif.a

OBJS = $(OBJDIR)/bitstream.o $(OBJDIR)/lfif_decoder.o $(OBJDIR)/lfif_encoder.o

all: $(BINDIR) $(OBJDIR) $(TARGET)

$(BINDIR)/liblfif.so: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/liblfif.a: $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/bitstream.o: $(SRCDIR)/bitstream.cc $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif_decoder.o: $(SRCDIR)/lfif_decoder.cc $(INCDIR)/lfif_decoder.h $(INCDIR)/lfif.h $(INCDIR)/dct.h $(INCDIR)/bitstream.h $(INCDIR)/constpow.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif_encoder.o: $(SRCDIR)/lfif_encoder.cc $(INCDIR)/lfif_encoder.h $(INCDIR)/lfif.h $(INCDIR)/dct.h $(INCDIR)/bitstream.h $(INCDIR)/constpow.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR)
