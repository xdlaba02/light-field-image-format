/******************************************************************************\
* SOUBOR: xvc_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <xvcenc.h>

extern "C" {
  #include <libswscale/swscale.h>
}

#include <getopt.h>
#include <cmath>

#include <iostream>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <cassert>
#include <bitset>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> -q <qp>" << endl;
}

template <typename F>
void encode(const xvc_encoder_api  *xvc_api, xvc_encoder *encoder, const uint8_t *buffer, F &&callback) {
  xvc_enc_pic_buffer *rec_pic_ptr { nullptr };
  xvc_enc_nal_unit *nal_units;
  int num_nal_units;

  if (buffer) {
    xvc_enc_return_code ret = xvc_api->encoder_encode(encoder, buffer, &nal_units, &num_nal_units, rec_pic_ptr);
    assert(ret == XVC_ENC_OK || ret == XVC_ENC_NO_MORE_OUTPUT);
  }
  else {
    xvc_enc_return_code ret = xvc_api->encoder_flush(encoder, &nal_units, &num_nal_units, rec_pic_ptr);
    assert(ret == XVC_ENC_OK);
  }

  for (int i = 0; i < num_nal_units; i++) {
    callback(nal_units[i]);
  }
}

int main(int argc, char *argv[]) {
  const char *input_file_mask     {};
  const char *output_file_name    {};
  const char *s_qp                {};

  vector<uint8_t> rgb_data        {};
  uint64_t width                  {};
  uint64_t height                 {};
  uint32_t color_depth            {};
  uint64_t image_count            {};

  double qp                       {};

  const xvc_encoder_api  *xvc_api {};
  xvc_encoder_parameters *params  {};
  xvc_encoder            *encoder {};

  SwsContext *in_convert_ctx      {};

  vector<uint8_t> yuv_frame       {};

  ofstream output                 {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:b:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
      break;

      case 'o':
        if (!output_file_name) {
          output_file_name = optarg;
          continue;
        }
      break;

      case 'q':
        if (!s_qp) {
          s_qp = optarg;
          continue;
        }
      break;

      default:
      break;
    }

    print_usage(argv[0]);
    return 1;
  }

  if (!input_file_mask || !output_file_name) {
    print_usage(argv[0]);
    return 1;
  }

  qp = 30;
  if (s_qp) {
    stringstream(s_qp) >> qp;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  rgb_data.resize(width * height * image_count * 3);
  yuv_frame.resize(width * height * 3 * 2);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  xvc_api = xvc_encoder_api_get();
  params = xvc_api->parameters_create();

  xvc_api->parameters_set_default(params);

  params->width             = width;
  params->height            = height;
  params->qp                = qp;
  params->chroma_format     = XVC_ENC_CHROMA_FORMAT_444;
  params->input_bitdepth    = 8;
  params->internal_bitdepth = 8;
  params->framerate         = 1;
  params->tune_mode         = 1;
  params->threads           = -1;

  xvc_enc_return_code ret = xvc_api->parameters_check(params);
  if (ret != XVC_ENC_OK) {
    cerr << xvc_api->xvc_enc_get_error_text(ret) << endl;
    return 1;
  }

  encoder = xvc_api->encoder_create(params);

  in_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUV444P, 0, 0, 0, 0);
  if (!in_convert_ctx) {
    cerr << "Could not get image conversion context" << endl;
    exit(1);
  }

  auto saveNal = [&](xvc_enc_nal_unit &nal) {
    char nal_size[4] {};

    nal_size[0] = (nal.size >>  0) & 0xFF;
    nal_size[1] = (nal.size >>  8) & 0xFF;
    nal_size[2] = (nal.size >> 16) & 0xFF;
    nal_size[3] = (nal.size >> 24) & 0xFF;

    output.write(nal_size, 4);
    output.write(reinterpret_cast<const char *>(nal.bytes), nal.size);
  };

  size_t last_slash_pos = string(output_file_name).find_last_of('/');
  if (last_slash_pos != string::npos) {
    string command = "mkdir -p " + string(output_file_name).substr(0, last_slash_pos);
    system(command.c_str());
  }

  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "Could not open " << output_file_name << " for writing\n";
    exit(1);
  }

  for (size_t image = 0; image < image_count; image++) {
    uint8_t *inData[1]  = { &rgb_data[image * width * height * 3] };
    uint8_t *outData[1] = { &yuv_frame[0] };
    int lineSize[1]     = { static_cast<int>(3 * width) };

    sws_scale(in_convert_ctx, inData, lineSize, 0, height, outData, lineSize); //FIXME, otestuj lineSize u HEVC

    encode(xvc_api, encoder, yuv_frame.data(), saveNal);
  }

  encode(xvc_api, encoder, nullptr, saveNal);

  sws_freeContext(in_convert_ctx);

  xvc_api->encoder_destroy(encoder);
  xvc_api->parameters_destroy(params);

  return 0;
}
