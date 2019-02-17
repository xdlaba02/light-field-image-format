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

TARGET = $(BINDIR)/liblfif.a $(BINDIR)/liblfif16.a

all: $(BINDIR) $(OBJDIR) $(TARGET)

$(BINDIR)/liblfif.a: $(OBJDIR)/bitstream.o $(OBJDIR)/huffman.o $(OBJDIR)/lfiflib.o $(OBJDIR)/runlength.o
	ar rcs $@ $^

$(BINDIR)/liblfif16.a: $(OBJDIR)/lfif16_compress.o $(OBJDIR)/lfif16_decompress.o $(OBJDIR)/bitstream.o
	ar rcs $@ $^


$(OBJDIR)/%.o: $(SRCDIR)/%.cc $(INCDIR)/%.h
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
