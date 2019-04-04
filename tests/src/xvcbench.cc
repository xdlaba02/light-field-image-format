/******************************************************************************\
* SOUBOR: av1bench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <xvcenc.h>
#include <xvcdec.h>

#include <getopt.h>
#include <cmath>

#include <iostream>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-f <fist-bitrate>] [-l <last-bitrate>] [-a]" << endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file     {};
  const char *first_bitrate     {};
  const char *last_bitrate     {};

  vector<uint8_t> rgb_data  {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  size_t image_pixels  {};

  double f_b {};
  double l_b {};

  bool append     {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:f:l:a")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
      break;

      case 'o':
        if (!output_file) {
          output_file = optarg;
          continue;
        }
      break;

      case 'f':
        if (!first_bitrate) {
          first_bitrate = optarg;
          continue;
        }
      break;

      case 'l':
        if (!last_bitrate) {
          last_bitrate = optarg;
          continue;
        }
      break;

      case 'a':
        if (!append) {
          append = true;
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

  f_b = 0.1;
  if (first_bitrate) {
    stringstream(first_bitrate) >> l_b;
  }

  l_b = 15;
  if (last_bitrate) {
    stringstream(last_bitrate) >> l_b;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  rgb_data.resize(width * height * image_count * 3);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  image_pixels = width * height * image_count;

  //////////////////////////////////////////////////////////////////////////////

  ofstream output {};
  if (append) {
    output.open(output_file, std::fstream::app);
  }
  else {
    output.open(output_file, std::fstream::trunc);
    output << "'xvc' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
  }

  const xvc_encoder_api  *xvc_capi {};
  xvc_encoder_parameters *cparams  {};
  xvc_encoder            *encoder  {};

  xvc_capi = xvc_encoder_api_get();
  cparams = xvc_capi->parameters_create();;

  xvc_capi->parameters_set_default(cparams);

  cparams->width = width;
  cparams->height = height;
  cparams->chroma_format = XVC_ENC_CHROMA_FORMAT_444;

  xvc_enc_return_code cret = xvc_capi->parameters_check(cparams);
  if (cret != XVC_ENC_OK) {
    cerr << xvc_capi->xvc_enc_get_error_text(cret) << endl;
    return 0;
  }

  encoder = xvc_capi->encoder_create(cparams);

  const xvc_decoder_api *xvc_dapi {};
  xvc_decoder_parameters *dparams {};
  xvc_decoder *decoder            {};
  xvc_decoded_picture decoded_pic {};

  xvc_dapi = xvc_decoder_api_get();
  dparams = xvc_dapi->parameters_create();

  xvc_dapi->parameters_set_default(dparams);
  dparams->output_width  = width;
  dparams->output_height = height;
  dparams->output_chroma_format = XVC_DEC_CHROMA_FORMAT_444;

  xvc_dec_return_code dret = xvc_dapi->parameters_check(dparams);
  if (dret != XVC_DEC_OK) {
    cerr << xvc_dapi->xvc_dec_get_error_text(dret) << endl;
    return 0;
  }

  decoder = xvc_dapi->decoder_create(dparams);

  xvc_enc_nal_unit *nal_units;
  int num_nal_units;

  bool   loop_check {true};
  size_t img        {0};
  size_t input_ite  {0};
  double mse        {0.0};
  size_t total_size {0};

  auto saveFrame = [&](xvc_decoded_picture *pic) {
    cerr << "SIZE: " << pic->size << " " << image_pixels * 3 << "\n";
    for (size_t pix = 0; pix < pic->size; pix++) {
      double tmp = rgb_data[input_ite] - pic->bytes[pix];
      mse += tmp * tmp;
      input_ite++;
    }
  };

  auto decodePkt = [&](xvc_enc_nal_unit *unit) {
    if (unit) {
      total_size += unit->size;
      dret = xvc_dapi->decoder_decode_nal(decoder, unit->bytes, unit->size, 0);
    }
    else {
      dret = xvc_dapi->decoder_flush(decoder);
    }

    if (xvc_dapi->decoder_get_picture(decoder, &decoded_pic) == XVC_DEC_OK) {
      saveFrame(&decoded_pic);
    }
  };

  while (loop_check) {
    if (img < image_count) {
      cerr << "IMG: " << img << "\n";
      xvc_enc_return_code ret = xvc_capi->encoder_encode(encoder, &rgb_data[img * width * height * 3], &nal_units, &num_nal_units, nullptr);
      assert(ret == XVC_ENC_OK || ret == XVC_ENC_NO_MORE_OUTPUT);
      img++;
    }
    else {
      xvc_enc_return_code ret = xvc_capi->encoder_flush(encoder, &nal_units, &num_nal_units, nullptr);
      loop_check = (ret == XVC_ENC_OK);
    }

    for (int i = 0; i < num_nal_units; i++) {
      decodePkt(&nal_units[i]);
    }

    decodePkt(nullptr);
  }

  mse /= image_pixels * 3;

  double bpp = total_size * 8.0 / image_pixels;
  double psnr = 10 * log10((255 * 255) / mse);

  cerr   << psnr << " " << bpp << endl;
  output << psnr << " " << bpp << endl;

  if (encoder) {
    xvc_capi->encoder_destroy(encoder);
  }
  if (cparams) {
    xvc_capi->parameters_destroy(cparams);
  }

  return 0;
}
