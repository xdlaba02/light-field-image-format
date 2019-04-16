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
INC 		 = -Iinclude -I../liblfif/include -I../libppm/include -I../tools/include -I/opt/mozjpeg/include -I/usr/include/ffmpeg -I/usr/local/include/
CFLAGS   = $(INC) -O3 -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -DBLOCK_SIZE=$(BS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

all: $(BINDIR) $(OBJDIR) $(DEPDIR) rebuild_tools $(BINDIR)/lfifbench $(BINDIR)/mozbench $(BINDIR)/x265bench $(BINDIR)/openjpegbench $(BINDIR)/av1bench $(BINDIR)/xvcbench

rebuild_tools:
	$(MAKE) -C ../tools/

$(BINDIR)/lfifbench: $(OBJDIR)/lfifbench.o
	$(CC) -lm -pthread $^ ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a ../liblfif/bin/liblfif.a -o $@

$(BINDIR)/mozbench: $(OBJDIR)/mozbench.o
	$(CC) -lm $^ ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a /opt/mozjpeg/lib64/libjpeg.a -o $@

$(BINDIR)/x265bench: $(OBJDIR)/x265bench.o
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a -o $@

$(BINDIR)/openjpegbench: $(OBJDIR)/openjpegbench.o
	$(CC) -lm -lpthread $^ ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a /usr/local/lib/libopenjp2.a -o $@

$(BINDIR)/av1bench: $(OBJDIR)/av1bench.o
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a -o $@

$(BINDIR)/xvcbench: $(OBJDIR)/xvcbench.o
	$(CC) -lm -lpthread $^ ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a /usr/local/lib/libxvcenc.a /usr/local/lib/libxvcdec.a -o $@

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

.PHONY: clean rebuild_tools

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR)
