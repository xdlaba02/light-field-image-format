/******************************************************************************\
* SOUBOR: mozbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <jpeglib.h>

#include <getopt.h>

#include <cmath>

#include <iostream>
#include <fstream>

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-s <quality-step>]" << endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file  {};

  const char *quality_step    {};

  uint8_t q_step  {};

  vector<uint8_t> original_rgb_data {};

  uint64_t original_width       {};
  uint64_t original_height      {};
  uint32_t original_color_depth {};
  uint64_t original_image_count {};

  jpeg_compress_struct cinfo   {};
  jpeg_decompress_struct dinfo {};
  jpeg_error_mgr jerr          {};
  FILE *outfile                {};
  FILE *infile                 {};
  JSAMPROW row_pointer[1]      {};
  int row_stride               {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:s:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
      break;

      case 's':
        if (!quality_step) {
          quality_step = optarg;
          continue;
        }
      break;

      case 'o':
        if (!output_file) {
          output_file = optarg;;
          continue;
        }
      break;

      default:
        print_usage(argv[0]);
        return 1;
      break;
    }
  }

  if (!input_file_mask || !output_file) {
    print_usage(argv[0]);
    return 1;
  }

  q_step = 1;

  if (quality_step) {
    int tmp = atoi(quality_step);
    if ((tmp < 1) || (tmp > 100)) {
      print_usage(argv[0]);
      return 1;
    }
    q_step = tmp;
  }

  if (!loadPPMs(input_file_mask, original_rgb_data, original_width, original_height, original_color_depth, original_image_count)) {
    return 2;
  }

  if (original_color_depth != 255) {
    cerr << "ERROR: UNSUPPORTED COLOR DEPTH. YET." << endl;
    return 2;
  }

  cinfo.err = jpeg_std_error(&jerr);
  dinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);
  jpeg_create_decompress(&dinfo);

  cinfo.image_width = original_width;
  cinfo.image_height = original_height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults(&cinfo);

  row_stride = original_width * 3;

  ofstream output(output_file);
  output << "'mozjpeg' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

  size_t image_pixels = original_width * original_height * original_image_count;

  for (size_t quality = q_step; quality <= 100; quality += q_step) {
    cerr << "Q" << quality << " STARTED" << endl;

    size_t compressed_size = 0;
    double mse = 0;

    jpeg_set_quality(&cinfo, quality, TRUE);

    cinfo.comp_info[0].h_samp_factor = 1;
    cinfo.comp_info[0].v_samp_factor = 1;
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;
    cinfo.comp_info[2].v_samp_factor = 1;

    for (size_t i = 0; i < original_image_count; i++) {
      uint8_t *original = &original_rgb_data[i * original_width * original_height * 3];
      vector<uint8_t>  decompressed_rgb_data(original_width * original_height * 3);

      if ((outfile = fopen("/tmp/mozbench.jpeg", "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "/tmp/mozbench.jpeg");
        exit(1);
      }

      jpeg_stdio_dest(&cinfo, outfile);

      jpeg_start_compress(&cinfo, TRUE);

      while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &original[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
      }

      jpeg_finish_compress(&cinfo);

      compressed_size += ftell(outfile);
      fclose(outfile);

      /**/

      if ((infile = fopen("/tmp/mozbench.jpeg", "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "/tmp/mozbench.jpeg");
        return 0;
      }

      jpeg_stdio_src(&dinfo, infile);

      jpeg_read_header(&dinfo, TRUE);

      jpeg_start_decompress(&dinfo);

      while (dinfo.output_scanline < dinfo.output_height) {
        row_pointer[0] = &decompressed_rgb_data[dinfo.output_scanline * row_stride];
        jpeg_read_scanlines(&dinfo, row_pointer, 1);
      }

      jpeg_finish_decompress(&dinfo);
      fclose(infile);

      for (size_t pix = 0; pix < decompressed_rgb_data.size(); pix++) {
        double tmp = original[pix] - decompressed_rgb_data[pix];
        mse += tmp * tmp;
      }

    }

    mse /= original_image_count * original_width * original_height * 3;

    double bpp = compressed_size * 8.0 / image_pixels;
    double psnr = 10 * log10((255 * 255) / mse);

    cerr << quality  << " " << psnr << " " << bpp << endl;
    output << quality  << " " << psnr << " " << bpp << endl;
  }

  jpeg_destroy_compress(&cinfo);
  jpeg_destroy_decompress(&dinfo);

  return 0;
}
