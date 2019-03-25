################################################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
################################################################################

DEPDIR = .dep
SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC 			 = g++
INC 		 = -Iinclude -I../liblfif/include -I../libppm/include -I../tools/include -I/opt/mozjpeg/include -I/usr/include/ffmpeg
CFLAGS   = $(INC) -O3 -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

TESTS = $(BINDIR)/lfifbench $(BINDIR)/mozbench $(BINDIR)/x265bench

all: $(BINDIR) $(OBJDIR) $(DEPDIR) $(TESTS)

MULTIPPM = ../tools/build/file_mask.o ../tools/build/plenoppm.o ../libppm/bin/libppm.a
LFIF 		 = ../liblfif/bin/liblfif.a

$(BINDIR)/lfifbench: $(OBJDIR)/lfifbench.o $(MULTIPPM) $(LFIF)
	$(CC) -lm -pthread $^ -o $@

$(BINDIR)/mozbench: $(OBJDIR)/mozbench.o $(MULTIPPM) /opt/mozjpeg/lib64/libjpeg.a
	$(CC) -lm $^ -o $@

$(BINDIR)/x265bench: $(OBJDIR)/x265bench.o $(MULTIPPM)
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
