########################################################
# SOUBOR: Makefile
# AUTOR: Drahomir Dlabaja (xdlaba02)
# DATUM: 19. 10. 2018
########################################################

TARGET = lfif

DEPDIR = .dep
SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = bin

CC = g++
CFLAGS = -Iinclude -O3 -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
LDFLAGS =
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)
OBJS = $(addsuffix .o, $(addprefix $(OBJDIR)/, $(basename $(notdir $(SRCS)))))

all: $(DEPDIR) $(BINDIR) $(OBJDIR) $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

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
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR) vgcore*
