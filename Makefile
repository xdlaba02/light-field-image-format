TARGET = codec

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

.PHONY: clean run encode decode valgrind

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR) vgcore*

run: encode decode

encode: $(BINDIR)/$(TARGET)
	./$< --encode 50 ../foto.ppm 2>cerr1.txt

decode: $(BINDIR)/$(TARGET) encode
	./$< --decode ../foto.ppm.jpeg2d 2>cerr2.txt

valgrind: $(BINDIR)/$(TARGET)
	valgrind --track-origins=yes ./$^ --encode 50 ../foto.ppm
	valgrind --track-origins=yes ./$^ --decode ../foto.ppm.jpeg2d
