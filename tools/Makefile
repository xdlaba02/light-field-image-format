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
INCLUDE  = -Iinclude -I../liblfif/include -I../libppm/include
OTHERINC = -I/opt/mozjpeg/include -I/usr/include/ffmpeg -I/usr/local/include/ -I../xvc/src/xvc_enc_lib -I../xvc/src/xvc_dec_lib
CFLAGS   = $(INCLUDE) -O3 -march=native -std=c++17 -Wall -Wextra -pedantic -Wfatal-errors
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

SRCS = $(wildcard $(SRCDIR)/*.cc)

PLENO   = $(OBJDIR)/file_mask.o $(OBJDIR)/plenoppm.o ../libppm/bin/libppm.a
LFIF    = $(PLENO) ../liblfif/bin/liblfif.a

$(shell mkdir -p $(BINDIR) > /dev/null)
$(shell mkdir -p $(OBJDIR) > /dev/null)
$(shell mkdir -p $(DEPDIR) > /dev/null)

.PHONY: all clean tools benchmarks

all: tools

tools: $(BINDIR)/ctu2d_compress \
			 $(BINDIR)/ctu4d_compress \
			 $(BINDIR)/lfif2d_compress \
			 $(BINDIR)/lfif2d_decompress \
			 $(BINDIR)/lfif3d_compress \
			 $(BINDIR)/lfif3d_decompress \
			 $(BINDIR)/lfif4d_compress \
			 $(BINDIR)/lfif4d_decompress \
			 $(BINDIR)/lfif5d_compress \
			 $(BINDIR)/lfif5d_decompress \
			 $(BINDIR)/lfifbench \
			 $(BINDIR)/lf_stats

benchmarks: $(BINDIR)/mozbench
						$(BINDIR)/x265bench \
						$(BINDIR)/openjpegbench \
						$(BINDIR)/av1bench \
						$(BINDIR)/xvcbench

others: $(BINDIR)/hevc_compress \
				$(BINDIR)/hevc_decompress \
				$(BINDIR)/xvc_compress \
				$(BINDIR)/xvc_decompress \
				$(BINDIR)/cabac_test \
				$(BINDIR)/predict_test \


$(BINDIR)/ctu4d_compress: $(OBJDIR)/ctu4d_compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/ctu2d_compress: $(OBJDIR)/ctu2d_compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif2d_compress: $(OBJDIR)/lfif2d_compress.o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif3d_compress: $(OBJDIR)/lfif3d_compress.o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif4d_compress: $(OBJDIR)/lfif4d_compress.o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif5d_compress: $(OBJDIR)/lfif5d_compress.o $(OBJDIR)/compress.o $(LFIF)
	$(CC) -lm $^ -o $@


$(BINDIR)/lfif2d_decompress: $(OBJDIR)/lfif2d_decompress.o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif3d_decompress: $(OBJDIR)/lfif3d_decompress.o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif4d_decompress: $(OBJDIR)/lfif4d_decompress.o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@

$(BINDIR)/lfif5d_decompress: $(OBJDIR)/lfif5d_decompress.o $(OBJDIR)/decompress.o $(LFIF)
	$(CC) -lm $^ -o $@


$(BINDIR)/lf_stats: $(OBJDIR)/lf_stats.o $(PLENO)
	$(CC) -lm $^ -o $@


$(BINDIR)/lfifbench: $(OBJDIR)/lfifbench.o $(LFIF)
	$(CC) -lm -pthread $^ -o $@

$(BINDIR)/mozbench: $(OBJDIR)/mozbench.o $(PLENO)
	$(CC) -lm $^ /opt/mozjpeg/lib64/libjpeg.a -o $@

$(BINDIR)/openjpegbench: $(OBJDIR)/openjpegbench.o $(PLENO)
	$(CC) -lm -lpthread $^ /usr/local/lib/libopenjp2.a -o $@

$(BINDIR)/av1bench: $(OBJDIR)/av1bench.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/x265bench: $(OBJDIR)/x265bench.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/xvcbench: $(OBJDIR)/xvcbench.o $(PLENO)
	$(CC) -lm -lpthread $^ /usr/local/lib/libxvcenc.a /usr/local/lib/libxvcdec.a -o $@

$(BINDIR)/hevc_compress: $(OBJDIR)/hevc_compress.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/hevc_decompress: $(OBJDIR)/hevc_decompress.o $(PLENO)
	$(CC) -lm -lavutil -lavformat -lavcodec -lswscale $^ -o $@

$(BINDIR)/xvc_compress: $(OBJDIR)/xvc_compress.o $(PLENO)
	$(CC) -L../xvc/build/src/ -L/usr/local/lib/ -lm -lpthread -lswscale $^ -l:libxvcenc.a -o $@

$(BINDIR)/xvc_decompress: $(OBJDIR)/xvc_decompress.o $(PLENO)
	$(CC) -L../xvc/build/src/ -L/usr/local/lib/ -lm -lpthread -lswscale $^ -l:libxvcdec.a -o $@



$(BINDIR)/cabac_test: $(OBJDIR)/cabac_test.o ../liblfif/bin/liblfif.a
	$(CC) -lm $^ -o $@

$(BINDIR)/predict_test: $(OBJDIR)/predict_test.o ../liblfif/bin/liblfif.a
	$(CC) -lm $^ -o $@

$(BINDIR)/dynamic_test: $(OBJDIR)/dynamic_test.o ../liblfif/bin/liblfif.a
	$(CC) -lm $^ -o $@

$(BINDIR)/ppm_test: $(OBJDIR)/ppm_test.o $(OBJDIR)/plenoppm.o $(OBJDIR)/file_mask.o ../libppm/bin/libppm.a
	$(CC) -lm $^ -o $@

$(BINDIR)/tiler: $(OBJDIR)/tiler.o $(OBJDIR)/plenoppm.o $(OBJDIR)/file_mask.o ../libppm/bin/libppm.a
	$(CC) -lm $^ -o $@

$(BINDIR)/dct_fiddle: $(OBJDIR)/dct_fiddle.o ../libppm/bin/libppm.a ../liblfif/build/bwt.o
	$(CC) -lm $^ -o $@

$(BINDIR)/zigzag_test: $(OBJDIR)/zigzag_test.o
	$(CC) -lm $^ -o $@

../libppm/bin/libppm.a:
		$(MAKE) -C ../libppm/

../liblfif/bin/liblfif.a:
		$(MAKE) -C ../liblfif/

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(notdir $(SRCS))))

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DEPDIR)
