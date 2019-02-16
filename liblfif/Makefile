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

TARGET = $(BINDIR)/liblfif.so $(BINDIR)/liblfif.a $(BINDIR)/liblfif16.a

OBJS = $(OBJDIR)/bitstream.o $(OBJDIR)/huffman.o $(OBJDIR)/lfiflib.o

all: $(BINDIR) $(OBJDIR) $(TARGET)

$(BINDIR)/liblfif.so: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/liblfif.a: $(OBJS)
	ar rcs $@ $^

$(BINDIR)/liblfif16.a: $(OBJDIR)/lfif16_compress.o $(OBJDIR)/lfif16_decompress.o $(OBJDIR)/bitstream.o
	ar rcs $@ $^


$(OBJDIR)/bitstream.o: $(SRCDIR)/bitstream.cc $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/huffman.o: $(SRCDIR)/huffman.cc $(INCDIR)/huffman.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfiflib.o: $(SRCDIR)/lfiflib.cc $(INCDIR)/lfiflib.h
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJDIR)/lfif16_compress.o: $(SRCDIR)/lfif16_compress.cc
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif16_decompress.o: $(SRCDIR)/lfif16_decompress.cc
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR)
