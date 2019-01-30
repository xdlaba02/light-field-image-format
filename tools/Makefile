################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
CFLAGS = -Iinclude -I../liblfif/include -I../libppm/include -O3 -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
LDFLAGS = -L../liblfif/bin -L../libppm/bin -llfif -lppm -lstdc++fs

CODEC = $(OBJDIR)/zigzag.o $(OBJDIR)/file_mask.o

ENCODER = $(OBJDIR)/compress.o $(CODEC)

DECODER = $(OBJDIR)/decompress.o $(CODEC)

all: $(BINDIR) $(OBJDIR) $(BINDIR)/lfif2d_compress $(BINDIR)/lfif2d_decompress $(BINDIR)/lfif3d_compress $(BINDIR)/lfif3d_decompress $(BINDIR)/lfif4d_compress $(BINDIR)/lfif4d_decompress

$(BINDIR)/lfif2d_compress: $(OBJDIR)/lfif2d_compress.o $(ENCODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif3d_compress: $(OBJDIR)/lfif3d_compress.o $(OBJDIR)/zigzag.o $(ENCODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif4d_compress: $(OBJDIR)/lfif4d_compress.o $(ENCODER)
	$(CC) $(LDFLAGS) $^ -o $@


$(BINDIR)/lfif2d_decompress: $(OBJDIR)/lfif2d_decompress.o $(DECODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif3d_decompress: $(OBJDIR)/lfif3d_decompress.o $(OBJDIR)/zigzag.o $(DECODER)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif4d_decompress: $(OBJDIR)/lfif4d_decompress.o $(DECODER)
	$(CC) $(LDFLAGS) $^ -o $@


$(OBJDIR)/lfif2d_compress.o: $(SRCDIR)/lfif2d_compress.cc $(INCDIR)/compress.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif3d_compress.o: $(SRCDIR)/lfif3d_compress.cc $(INCDIR)/compress.h $(INCDIR)/zigzag.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif4d_compress.o: $(SRCDIR)/lfif4d_compress.cc $(INCDIR)/compress.h
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJDIR)/lfif2d_decompress.o: $(SRCDIR)/lfif2d_decompress.cc $(INCDIR)/decompress.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif3d_decompress.o: $(SRCDIR)/lfif3d_decompress.cc $(INCDIR)/decompress.h $(INCDIR)/zigzag.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif4d_decompress.o: $(SRCDIR)/lfif4d_decompress.cc $(INCDIR)/decompress.h
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJDIR)/file_mask.o: $(SRCDIR)/file_mask.cc $(INCDIR)/file_mask.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/compress.o: $(SRCDIR)/compress.cc $(INCDIR)/compress.h $(INCDIR)/file_mask.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/decompress.o: $(SRCDIR)/decompress.cc $(INCDIR)/decompress.h $(INCDIR)/file_mask.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/zigzag.o: $(SRCDIR)/zigzag.cc $(INCDIR)/zigzag.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) vgcore* *.data callgrind*
