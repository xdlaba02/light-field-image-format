/******************************************************************************\
* SOUBOR: av1bench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

extern "C" {
  #include <libavcodec/avcodec.h>
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
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-f <fist-bitrate>] [-l <last-bitrate>] [-a] [-n]" << endl;
}

void encode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt, const function<void(AVPacket *)> &callback) {
  if (avcodec_send_frame(context, frame) < 0) {
    cerr << "Error sending a frame for encoding" << endl;
    exit(1);
  }

  int ret = 0;
  while (ret >= 0) {
    ret = avcodec_receive_packet(context, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    }
    else if (ret < 0) {
      cerr << "Error during encoding" << endl;
      exit(1);
    }

    callback(pkt);

    av_packet_unref(pkt);
  }
}

void decode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt, const function<void(AVFrame *)> &callback) {
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
  bool intra_only {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:f:l:an")) >= 0) {
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

      case 'n':
        if (!intra_only) {
          intra_only = true;
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
    f_b = stod(first_bitrate);
  }

  l_b = 15;
  if (last_bitrate) {
    l_b = stod(last_bitrate);
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  rgb_data.resize(width * height * image_count * 3);

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  image_pixels = width * height * image_count;

  AVPacket *pkt               {};

  AVCodec *coder              {};
  AVFrame *in_frame           {};
  AVCodecContext *in_context  {};
  SwsContext *in_convert_ctx  {};

  AVCodec *decoder            {};
  AVFrame *out_frame          {};
  AVCodecContext *out_context {};
  SwsContext *out_convert_ctx {};

  AVFrame *rgb_frame          {};

  pkt = av_packet_alloc();
  if (!pkt) {
    exit(1);
  }

  out_frame = av_frame_alloc();
  if (!out_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  in_frame = av_frame_alloc();
  if (!in_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  in_frame->format = AV_PIX_FMT_YUV444P;
  in_frame->width  = width;
  in_frame->height = height;

  if (av_frame_get_buffer(in_frame, 32) < 0) {
    cerr << "Could not allocate the video frame data" << endl;
    exit(1);
  }

  rgb_frame = av_frame_alloc();
  if (!rgb_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  rgb_frame->format = AV_PIX_FMT_RGB24;
  rgb_frame->width  = width;
  rgb_frame->height = height;

  if (av_frame_get_buffer(rgb_frame, 32) < 0) {
    cerr << "Could not allocate the video frame data" << endl;
    exit(1);
  }

  decoder = avcodec_find_decoder(AV_CODEC_ID_AV1);
  if (!decoder) {
    cerr << "decoder AV_CODEC_ID_AV1 not found" << endl;
    exit(1);
  }

  coder = avcodec_find_encoder(AV_CODEC_ID_AV1);
  if (!coder) {
    cerr << "coder AV_CODEC_ID_AV1 not found" << endl;
    exit(1);
  }

  out_context = avcodec_alloc_context3(decoder);
  if (!out_context) {
    cerr << "Could not allocate video coder context" << endl;
    exit(1);
  }

  in_context = avcodec_alloc_context3(coder);
  if (!in_context) {
    cerr << "Could not allocate video coder context" << endl;
    exit(1);
  }

  in_context->width = width;
  in_context->height = height;

  in_context->time_base = {1, int(image_count)};
  in_context->framerate = {int(image_count), 1};

  in_context->pix_fmt = AV_PIX_FMT_YUV444P;

  in_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUV444P, 0, 0, 0, 0);
  if (!in_convert_ctx) {
    cerr << "Could not get image conversion context" << endl;
    exit(1);
  }

  out_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV444P, width, height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
  if (!out_convert_ctx) {
    cerr << "Could not get image conversion context" << endl;
    exit(1);
  }

  ofstream output {};
  if (append) {
    output.open(output_file, std::fstream::app);
  }
  else {
    output.open(output_file, std::fstream::trunc);
    output << "'av1' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
  }

  for (double bpp = f_b; bpp <= l_b; bpp *= 1.25893) {
    cerr << "BPP: " << bpp << "\n";

    vector<uint8_t> out_rgb_data {};
    size_t compressed_size = 0;

    in_context->bit_rate = bpp * image_pixels;
    in_context->strict_std_compliance = -2;

    if (avcodec_open2(in_context, coder, nullptr) < 0) {
      cerr << "Could not open coder" << endl;
      exit(1);
    }

    out_context->strict_std_compliance = -2;

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
      cerr << "IMG: " << image << "\n";

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

    avcodec_close(in_context);
    avcodec_close(out_context);


    mse /= image_count * width * height * 3;

    double out_bpp = compressed_size * 8.0 / image_pixels;
    double psnr = 10 * log10((255 * 255) / mse);

    cerr << bpp << " " << psnr << " " << out_bpp << endl;
    output << bpp << " " << psnr << " " << out_bpp << endl;

    if (abs(bpp - out_bpp) > 2) {
      break;
    }
  }

  avcodec_free_context(&in_context);
  avcodec_free_context(&out_context);
  av_frame_free(&in_frame);
  av_frame_free(&out_frame);
  av_frame_free(&rgb_frame);
  av_packet_free(&pkt);
  free(in_convert_ctx);
  free(out_convert_ctx);

  return 0;
}
