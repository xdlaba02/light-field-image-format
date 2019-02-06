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

all: $(BINDIR) $(OBJDIR) lfiftools testtools

lfiftools: $(BINDIR)/lfif2d_compress $(BINDIR)/lfif2d_decompress $(BINDIR)/lfif3d_compress $(BINDIR)/lfif3d_decompress $(BINDIR)/lfif4d_compress $(BINDIR)/lfif4d_decompress

LFIFTOOLSLDFLAGS = -lm

COMPRESS = $(OBJDIR)/compress.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../liblfif/bin/liblfif.a ../libppm/bin/libppm.a
DECOMPRESS = $(OBJDIR)/decompress.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../liblfif/bin/liblfif.a ../libppm/bin/libppm.a

$(BINDIR)/lfif2d_compress: $(OBJDIR)/lfif2d_compress.o $(COMPRESS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif3d_compress: $(OBJDIR)/lfif3d_compress.o $(COMPRESS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif4d_compress: $(OBJDIR)/lfif4d_compress.o $(COMPRESS)
	$(CC) $(LDFLAGS) $^ -o $@


$(BINDIR)/lfif2d_decompress: $(OBJDIR)/lfif2d_decompress.o $(DECOMPRESS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif3d_decompress: $(OBJDIR)/lfif3d_decompress.o $(DECOMPRESS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/lfif4d_decompress: $(OBJDIR)/lfif4d_decompress.o $(DECOMPRESS)
	$(CC) $(LDFLAGS) $^ -o $@


$(OBJDIR)/lfif2d_compress.o: $(SRCDIR)/lfif2d_compress.cc $(INCDIR)/compress.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif3d_compress.o: $(SRCDIR)/lfif3d_compress.cc $(INCDIR)/compress.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif4d_compress.o: $(SRCDIR)/lfif4d_compress.cc $(INCDIR)/compress.h
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJDIR)/lfif2d_decompress.o: $(SRCDIR)/lfif2d_decompress.cc $(INCDIR)/decompress.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif3d_decompress.o: $(SRCDIR)/lfif3d_decompress.cc $(INCDIR)/decompress.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lfif4d_decompress.o: $(SRCDIR)/lfif4d_decompress.cc $(INCDIR)/decompress.h
	$(CC) $(CFLAGS) -c $< -o $@


testtools: $(BINDIR)/lfiftest $(BINDIR)/moztest $(BINDIR)/x265test

$(BINDIR)/lfiftest: $(OBJDIR)/lfiftest.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a ../liblfif/bin/liblfif.a
	$(CC) -lm -pthread $^ -o $@

$(BINDIR)/moztest: $(OBJDIR)/moztest.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a /opt/mozjpeg/lib64/libjpeg.a
	$(CC) -lm $^ -o $@

$(BINDIR)/x265test: $(OBJDIR)/x265test.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a
	$(CC) -lm  -lavutil -lavformat -lavcodec $^ -o $@


$(OBJDIR)/lfiftest.o: $(SRCDIR)/lfiftest.cc $(INCDIR)/lfiftest.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/moztest.o: $(SRCDIR)/moztest.cc
	$(CC) $(CFLAGS) -I/opt/mozjpeg/include -c $< -o $@

$(OBJDIR)/x265test.o: $(SRCDIR)/x265test.cc
	$(CC) $(CFLAGS) -I/usr/include/ffmpeg -c $< -o $@


$(OBJDIR)/compress.o: $(SRCDIR)/compress.cc $(INCDIR)/compress.h $(INCDIR)/file_mask.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/decompress.o: $(SRCDIR)/decompress.cc $(INCDIR)/decompress.h $(INCDIR)/file_mask.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/file_mask.o: $(SRCDIR)/file_mask.cc $(INCDIR)/file_mask.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/zigzag.o: $(SRCDIR)/zigzag.cc $(INCDIR)/zigzag.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/plenoppm.o: $(SRCDIR)/plenoppm.cc $(INCDIR)/plenoppm.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) vgcore* *.data callgrind*
