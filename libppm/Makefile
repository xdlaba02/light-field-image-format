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

TARGET = $(BINDIR)/libppm.so $(BINDIR)/libppm.a

OBJS = $(OBJDIR)/ppm.o

all: $(BINDIR) $(OBJDIR) $(TARGET)

$(BINDIR)/libppm.so: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BINDIR)/libppm.a: $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/ppm.o: $(SRCDIR)/ppm.cc $(INCDIR)/ppm.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR)
