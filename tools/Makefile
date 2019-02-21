################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

DEPDIR = .dep
SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
INC = -Iinclude -I../liblfif/include -I../libppm/include -I/opt/mozjpeg/include -I/usr/include/ffmpeg
CFLAGS = $(INC) -O3 -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

LFIFTOOLS = $(BINDIR)/lfif2d_compress $(BINDIR)/lfif2d_decompress $(BINDIR)/lfif3d_compress $(BINDIR)/lfif3d_decompress $(BINDIR)/lfif4d_compress $(BINDIR)/lfif4d_decompress $(BINDIR)/lfif16_compress $(BINDIR)/lfif16_decompress
BENCHTOOLS = $(BINDIR)/lfifbench $(BINDIR)/mozbench $(BINDIR)/x265bench

all: $(BINDIR) $(OBJDIR) $(DEPDIR) $(LFIFTOOLS) $(BENCHTOOLS)

COMPRESS = $(OBJDIR)/compress.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../liblfif/bin/liblfif.a ../libppm/bin/libppm.a
DECOMPRESS = $(OBJDIR)/decompress.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../liblfif/bin/liblfif.a ../libppm/bin/libppm.a
BENCH = $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a

$(BINDIR)/%_compress: $(OBJDIR)/%_compress.o $(COMPRESS)
	$(CC) -lm $^ -o $@

$(BINDIR)/%_decompress: $(OBJDIR)/lfif_decompress.o $(DECOMPRESS)
	$(CC) -lm $^ -o $@


$(BINDIR)/lfifbench: $(OBJDIR)/lfifbench.o $(BENCH) ../liblfif/bin/liblfif.a
	$(CC) -lm -pthread $^ -o $@

$(BINDIR)/mozbench: $(OBJDIR)/mozbench.o $(BENCH) /opt/mozjpeg/lib64/libjpeg.a
	$(CC) -lm $^ -o $@

$(BINDIR)/x265bench: $(OBJDIR)/x265bench.o $(BENCH)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@


$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(notdir $(SRCS))))


$(DEPDIR):
	mkdir -p $(DEPDIR)
$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR)
