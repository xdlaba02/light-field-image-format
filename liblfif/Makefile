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


$(INCDIR)/block_compress_chain.h: $(INCDIR)/lfiftypes.h $(INCDIR)/quant_table.h $(INCDIR)/traversal_table.h $(INCDIR)/huffman.h $(INCDIR)/dct.h $(INCDIR)/runlength.h $(INCDIR)/block.h

$(INCDIR)/block_decompress_chain.h: $(INCDIR)/lfiftypes.h $(INCDIR)/huffman.h $(INCDIR)/runlength.h $(INCDIR)/traversal_table.h $(INCDIR)/quant_table.h $(INCDIR)/dct.h $(INCDIR)/block.h

$(INCDIR)/block.h: $(INCDIR)/lfiftypes.h

$(INCDIR)/colorspace.h: $(INCDIR)/lfiftypes.h

$(INCDIR)/dct.h: $(INCDIR)/constpow.h

$(INCDIR)/lfif_decoder.h: $(INCDIR)/block_decompress_chain.h $(INCDIR)/bitstream.h $(INCDIR)/colorspace.h

$(INCDIR)/lfif_encoder.h: $(INCDIR)/colorspace.h $(INCDIR)/bitstream.h $(INCDIR)/block_compress_chain.h

$(INCDIR)/lfiftypes.h: $(INCDIR)/constpow.h

$(INCDIR)/runlength.h: $(INCDIR)/huffman.h


$(OBJDIR)/bitstream.o: $(SRCDIR)/bitstream.cc $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/huffman.o: $(SRCDIR)/huffman.cc $(INCDIR)/huffman.h $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif16_compress.o: $(SRCDIR)/lfif16_compress.cc $(INCDIR)/lfif16.h $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif16_decompress.o: $(SRCDIR)/lfif16_decompress.cc $(INCDIR)/lfif16.h $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfiflib.o: $(SRCDIR)/lfiflib.cc $(INCDIR)/lfiflib.h $(INCDIR)/lfif_encoder.h $(INCDIR)/colorspace.h $(INCDIR)/bitstream.h $(INCDIR)/block_compress_chain.h $(INCDIR)/lfiftypes.h $(INCDIR)/quant_table.h $(INCDIR)/traversal_table.h $(INCDIR)/huffman.h $(INCDIR)/dct.h $(INCDIR)/runlength.h $(INCDIR)/block.h $(INCDIR)/constpow.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/runlength.o: $(SRCDIR)/runlength.cc $(INCDIR)/runlength.h $(INCDIR)/huffman.h $(INCDIR)/bitstream.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cc $(INCDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR)
