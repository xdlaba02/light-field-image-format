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
CFLAGS = $(INC) -O3 -g -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -fPIC
LDFLAGS = -shared
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

TARGET = $(BINDIR)/libppm.so $(BINDIR)/libppm.a

all: $(BINDIR) $(OBJDIR) $(DEPDIR) $(TARGET)

$(BINDIR)/libppm.so: $(OBJDIR)/ppm.o
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/libppm.a: $(OBJDIR)/ppm.o
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
