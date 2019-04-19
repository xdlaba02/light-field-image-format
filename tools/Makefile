################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

DEPDIR = .dep
SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

BS			 = 8
CC 			 = g++
INC 		 = -Iinclude -I../liblfif/include -I../libppm/include -I/opt/mozjpeg/include -I/usr/include/ffmpeg -I/usr/local/include/
CFLAGS   = $(INC) -O3 -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -DBLOCK_SIZE=$(BS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

PLENO = $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a
LFIF  = $(PLENO) ../liblfif/bin/liblfif.a

all: tools benchmarks
tools: $(BINDIR) $(OBJDIR) $(DEPDIR) $(BINDIR)/lfif2d_compress $(BINDIR)/lfif2d_decompress $(BINDIR)/lfif3d_compress $(BINDIR)/lfif3d_decompress $(BINDIR)/lfif4d_compress $(BINDIR)/lfif4d_decompress
benchmarks: $(BINDIR) $(OBJDIR) $(DEPDIR) $(BINDIR)/lfifbench $(BINDIR)/mozbench $(BINDIR)/x265bench $(BINDIR)/openjpegbench $(BINDIR)/av1bench $(BINDIR)/xvcbench
lfif: $(BINDIR) $(OBJDIR) $(DEPDIR) tools $(BINDIR)/lfifbench

$(BINDIR)/%_decompress: $(OBJDIR)/%_decompress.o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/%_compress: $(OBJDIR)/%_compress.o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfifbench: $(OBJDIR)/lfifbench.o $(LFIF)
	$(CC) -lm -pthread $^ -o $@

$(BINDIR)/mozbench: $(OBJDIR)/mozbench.o $(PLENO)
	$(CC) -lm $^ /opt/mozjpeg/lib64/libjpeg.a -o $@

$(BINDIR)/x265bench: $(OBJDIR)/x265bench.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/openjpegbench: $(OBJDIR)/openjpegbench.o $(PLENO)
	$(CC) -lm -lpthread $^ /usr/local/lib/libopenjp2.a -o $@

$(BINDIR)/av1bench: $(OBJDIR)/av1bench.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/xvcbench: $(OBJDIR)/xvcbench.o $(PLENO)
	$(CC) -lm -lpthread $^ /usr/local/lib/libxvcenc.a /usr/local/lib/libxvcdec.a -o $@

../libppm/bin/libppm.a:
		$(MAKE) -C ../libppm/

../liblfif/bin/liblfif.a:
		$(MAKE) -C ../liblfif/

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

.PHONY: clean tools benchmarks lfif

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR)
