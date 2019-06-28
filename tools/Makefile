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
INC 		 = -Iinclude -I../liblfif/include -I../libppm/include -I/opt/mozjpeg/include -I/usr/include/ffmpeg -I/usr/local/include/ -I../xvc/src/xvc_enc_lib -I../xvc/src/xvc_dec_lib
CFLAGS   = $(INC) -O3 -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors -DBLOCK_SIZE=$(BS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

FOLDERS = $(BINDIR) $(OBJDIR) $(DEPDIR)
PLENO   = $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a
LFIF    = $(PLENO) ../liblfif/bin/liblfif.a

.PHONY: all clean tools benchmarks

all: tools

tools: $(FOLDERS) \
			 $(BINDIR)/lfif2d_compress \
			 $(BINDIR)/lfif2d_decompress \
			 $(BINDIR)/lfif3d_compress \
			 $(BINDIR)/lfif3d_decompress \
			 $(BINDIR)/lfif4d_compress \
			 $(BINDIR)/lfif4d_decompress \
			 $(BINDIR)/lfif5d_compress \
			 $(BINDIR)/lfif5d_decompress \
			 $(BINDIR)/center_compress \
			 $(BINDIR)/center_decompress \
			 $(BINDIR)/lf_psnr

benchmarks: $(FOLDERS) \
						$(BINDIR)/lfifbench \
						$(BINDIR)/mozbench \
						$(BINDIR)/x265bench $(BINDIR)/openjpegbench $(BINDIR)/av1bench $(BINDIR)/xvcbench

others: $(FOLDERS) \
			  $(BINDIR)/hevc_compress \
				$(BINDIR)/hevc_decompress \
				$(BINDIR)/xvc_compress \
				$(BINDIR)/xvc_decompress \


$(BINDIR)/lfif2d_compress: $(OBJDIR)/lfif2d_compress$(BS).o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif3d_compress: $(OBJDIR)/lfif3d_compress$(BS).o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif4d_compress: $(OBJDIR)/lfif4d_compress$(BS).o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif5d_compress: $(OBJDIR)/lfif5d_compress$(BS).o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/center_compress: $(OBJDIR)/center_compress$(BS).o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif2d_decompress: $(OBJDIR)/lfif2d_decompress$(BS).o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif3d_decompress: $(OBJDIR)/lfif3d_decompress$(BS).o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif4d_decompress: $(OBJDIR)/lfif4d_decompress$(BS).o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif5d_decompress: $(OBJDIR)/lfif5d_decompress$(BS).o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/center_decompress: $(OBJDIR)/center_decompress$(BS).o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@


$(BINDIR)/lf_psnr: $(OBJDIR)/lf_psnr$(BS).o $(PLENO)
	$(CC) -lm $^ -o $@


$(BINDIR)/lfifbench: $(OBJDIR)/lfifbench$(BS).o $(LFIF)
	$(CC) -lm -pthread $^ -o $@

$(BINDIR)/mozbench: $(OBJDIR)/mozbench.o $(PLENO)
	$(CC) -lm $^ /opt/mozjpeg/lib64/libjpeg.a -o $@

$(BINDIR)/openjpegbench: $(OBJDIR)/openjpegbench.o $(PLENO)
	$(CC) -lm -lpthread $^ /usr/local/lib/libopenjp2.a -o $@

$(BINDIR)/av1bench: $(OBJDIR)/av1bench.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/xvcbench: $(OBJDIR)/xvcbench.o $(PLENO)
	$(CC) -lm -lpthread $^ /usr/local/lib/libxvcenc.a /usr/local/lib/libxvcdec.a -o $@

$(BINDIR)/hevc_compress: $(OBJDIR)/hevc_compress$(BS).o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/hevc_decompress: $(OBJDIR)/hevc_decompress$(BS).o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/xvc_compress: $(OBJDIR)/xvc_compress.o $(PLENO)
	$(CC) -L../xvc/build/src/ -L/usr/local/lib/ -lm -lpthread -lswscale $^ -l:libxvcenc.a -o $@

$(BINDIR)/xvc_decompress: $(OBJDIR)/xvc_decompress.o $(PLENO)
	$(CC) -L../xvc/build/src/ -L/usr/local/lib/ -lm -lpthread -lswscale $^ -l:libxvcdec.a -o $@

../libppm/bin/libppm.a:
		$(MAKE) -C ../libppm/

../liblfif/bin/liblfif.a:
		$(MAKE) -C ../liblfif/

$(OBJDIR)/%$(BS).o: $(SRCDIR)/%.cc
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

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

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR)
