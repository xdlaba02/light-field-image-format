################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
CFLAGS = -Iinclude -O3 -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -fPIC
LDFLAGS = -shared

TARGET = $(BINDIR)/liblfif.so $(BINDIR)/liblfif.a

OBJS = $(OBJDIR)/bitstream.o $(OBJDIR)/huffman.o

all: $(BINDIR) $(OBJDIR) $(TARGET)

$(BINDIR)/liblfif.so: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/liblfif.a: $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/bitstream.o: $(SRCDIR)/bitstream.cc $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/huffman.o: $(SRCDIR)/huffman.cc $(INCDIR)/huffman.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR)
