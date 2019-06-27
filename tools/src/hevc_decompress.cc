/******************************************************************************\
* SOUBOR: hevc_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavutil/opt.h>
  #include <libswscale/swscale.h>
}

#include <getopt.h>
#include <cmath>

#include <iostream>
#include <fstream>
#include <functional>
#include <iostream>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-name> -o <output-file-mask>" << endl;
}

template <typename F>
void decode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt, F &&callback) {
  if (avcodec_send_packet(context, pkt) < 0) {
    cerr << "Error sending a packet for decoding" << endl;
    exit(1);
  }

  int ret = 0;
  while (ret >= 0) {
    ret = avcodec_receive_frame(context, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    }
    else if (ret < 0) {
      cerr << "Error during decoding" << endl;
      exit(1);
    }

    callback(frame);
  }
}

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_mask {};

  uint64_t width               {};
  uint64_t height              {};
  uint32_t color_depth         {};
  uint64_t image_count         {};

  size_t image_pixels          {};

  AVPacket *pkt                {};
  AVCodec *decoder             {};
  AVFrame *out_frame           {};
  AVCodecContext *out_context  {};
  SwsContext *out_convert_ctx  {};
  AVFrame *rgb_frame           {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_name) {
          input_file_name = optarg;
          continue;
        }
      break;

      case 'o':
        if (!output_file_mask) {
          output_file_mask = optarg;
          continue;
        }
      break;

      default:
        print_usage(argv[0]);
        return 1;
      break;
    }
  }

  if (!input_file_name || !output_file_mask) {
    print_usage(argv[0]);
    return 1;
  }

  pkt = av_packet_alloc();
  if (!pkt) {
    exit(1);
  }

  out_frame = av_frame_alloc();
  if (!out_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  rgb_frame = av_frame_alloc();
  if (!rgb_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  avcodec_register_all();
  decoder = avcodec_find_decoder(AV_CODEC_ID_H265);
  if (!decoder) {
    cerr << "decoder AV_CODEC_ID_H265 not found" << endl;
    exit(1);
  }

  out_context = avcodec_alloc_context3(decoder);
  if (!out_context) {
    cerr << "Could not allocate video coder context" << endl;
    exit(1);
  }

  out_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV444P, width, height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
  if (!out_convert_ctx) {
    cerr << "Could not get image conversion context" << endl;
    exit(1);
  }

  vector<uint8_t> out_rgb_data {};
  size_t compressed_size = 0;

  in_context->bit_rate = bpp * image_pixels;

  if (avcodec_open2(in_context, coder, nullptr) < 0) {
    cerr << "Could not open coder" << endl;
    exit(1);
  }

  if (avcodec_open2(out_context, decoder, nullptr) < 0) {
    cerr << "Could not open decoder" << endl;
    exit(1);
  }

  double mse = 0;
  size_t input_iterator = 0;

  auto saveFrame = [&](AVFrame *frame) {
    int outLinesize[1] = { static_cast<int>(3 * width) };
    sws_scale(out_convert_ctx, frame->data, frame->linesize, 0, height, rgb_frame->data, outLinesize);

    for (int pix = 0; pix < rgb_frame->width * rgb_frame->height * 3; pix++) {
      double tmp = rgb_data[input_iterator] - rgb_frame->data[0][pix];
      mse += tmp * tmp;
      input_iterator++;
    }
  };

  auto decodePkt = [&](AVPacket *pkt) {
    if (pkt) {
      compressed_size += pkt->size;
    }
    decode(out_context, out_frame, pkt, saveFrame);
  };

  for (size_t image = 0; image < image_count; image++) {
    uint8_t *inData[1] = { &rgb_data[image * width * height * 3] };
    int inLinesize[1] = { static_cast<int>(3 * width) };
    sws_scale(in_convert_ctx, inData, inLinesize, 0, height, in_frame->data, in_frame->linesize);

    if (intra_only) {
      in_frame->pict_type = AV_PICTURE_TYPE_I;
    }

    in_frame->pts = image;

    encode(in_context, in_frame, pkt, decodePkt);
  }

  encode(in_context, nullptr, pkt, decodePkt);

  decodePkt(nullptr);

  avcodec_close(out_context);

  avcodec_free_context(&out_context);
  av_frame_free(&out_frame);
  av_frame_free(&rgb_frame);
  av_packet_free(&pkt);
  free(out_convert_ctx);

  return 0;
}
