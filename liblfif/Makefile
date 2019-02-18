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
INC = -Iinclude
CFLAGS = $(INC) -O3 -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -fPIC
LDFLAGS = -shared
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

TARGET = $(BINDIR)/liblfif.a $(BINDIR)/liblfif16.a

all: $(BINDIR) $(OBJDIR) $(DEPDIR) $(TARGET)

$(BINDIR)/liblfif.a: $(OBJDIR)/bitstream.o $(OBJDIR)/huffman.o $(OBJDIR)/lfiflib.o $(OBJDIR)/runlength.o
	ar rcs $@ $^

$(BINDIR)/liblfif16.a: $(OBJDIR)/bitstream.o $(OBJDIR)/lfif16_compress.o $(OBJDIR)/lfif16_decompress.o
	ar rcs $@ $^

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
