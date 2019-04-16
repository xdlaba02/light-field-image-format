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
INC 		 = -Iinclude -I../liblfif/include -I../libppm/include
CFLAGS   = $(INC) -O3 -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -DBLOCK_SIZE=$(BS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

LFIFTOOLS = $(BINDIR)/lfif2d_compress $(BINDIR)/lfif2d_decompress $(BINDIR)/lfif3d_compress $(BINDIR)/lfif3d_decompress $(BINDIR)/lfif4d_compress $(BINDIR)/lfif4d_decompress

all: $(BINDIR) $(OBJDIR) $(DEPDIR) rebuild_libs $(LFIFTOOLS)

rebuild_libs:
	$(MAKE) -C ../liblfif/
	$(MAKE) -C ../libppm/

$(BINDIR)/%_decompress: $(OBJDIR)/%_decompress.o $(OBJDIR)/decompress.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a ../liblfif/bin/liblfif.a
	$(CC) -lm $^ -o $@

$(BINDIR)/%_compress: $(OBJDIR)/%_compress.o $(OBJDIR)/compress.o $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a ../liblfif/bin/liblfif.a
	$(CC) -lm $^ -o $@

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

.PHONY: clean rebuild_libs

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR)
