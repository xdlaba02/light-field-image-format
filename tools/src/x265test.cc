/******************************************************************************\
* SOUBOR: x265test.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

extern "C" {
  #include <libavcodec/avcodec.h>
}

#include <getopt.h>
#include <cmath>

#include <iostream>
#include <fstream>
#include <functional>

#define INBUF_SIZE 4096

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-s <quality-step>]" << endl;
}

double PSNR(const vector<uint8_t> &original, const vector<uint8_t> &compared) {
  double mse  {};
  size_t size {};

  size = original.size() < compared.size() ? original.size() : compared.size();

  for (size_t i = 0; i < size; i++) {
    mse += (original[i] - compared[i]) * (original[i] - compared[i]);
  }

  mse /= size;

  if (!mse) {
    return 0;
  }

  return 10 * log10((255.0 * 255.0) / mse);
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

  vector<uint8_t> in_rgb_data  {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  size_t image_pixels  {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
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

  if (!input_file_mask) {
    print_usage(argv[0]);
    return 1;
  }

  if (!output_file) {
    cerr << "Please specify -o <output-file-name>." << endl;
    print_usage(argv[0]);
    return 1;
  }

  if (!loadPPMs(input_file_mask, in_rgb_data, width, height, color_depth, image_count)) {
    return 2;
  }

  if (color_depth != 255) {
    cerr << "ERROR: UNSUPPORTED COLOR DEPTH. YET." << endl;
    return 2;
  }

  image_pixels = width * height * image_count;

  AVPacket *pkt               {};

  AVCodec *coder              {};
  AVFrame *in_frame           {};
  AVCodecContext *in_context  {};

  AVCodec *decoder            {};
  AVFrame *out_frame          {};
  AVCodecContext *out_context {};

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

  decoder = avcodec_find_decoder(AV_CODEC_ID_HEVC);
  if (!decoder) {
    cerr << "decoder AV_CODEC_ID_HEVC not found" << endl;
    exit(1);
  }

  coder = avcodec_find_encoder_by_name("libx265");
  if (!coder) {
    cerr << "coder 'libx265' not found" << endl;
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

  in_context->gop_size = image_count;
  in_context->pix_fmt = AV_PIX_FMT_YUV444P;

  for (double bpp = 0.5; bpp < 5; bpp += .5) {
    vector<uint8_t> out_rgb_data {};

    in_context->bit_rate = bpp * image_pixels;

    if (avcodec_open2(in_context, coder, nullptr) < 0) {
      cerr << "Could not open coder" << endl;
      exit(1);
    }

    if (avcodec_open2(out_context, decoder, nullptr) < 0) {
      cerr << "Could not open decoder" << endl;
      exit(1);
    }

    auto saveFrame = [&](AVFrame *frame) {
      for (int y = 0; y < frame->height; y++) {
        for (int x = 0; x < frame->width; x++) {
          out_rgb_data.push_back(frame->data[0][y * frame->width + x]);
          out_rgb_data.push_back(frame->data[1][y * frame->width + x]);
          out_rgb_data.push_back(frame->data[2][y * frame->width + x]);
        }
      }
    };

    auto decodePkt = [&](AVPacket *pkt) {
      decode(out_context, out_frame, pkt, saveFrame);
    };

    for (size_t image = 0; image < image_count; image++) {
      if (av_frame_make_writable(in_frame) < 0) {
        exit(1);
      }

      for (int y = 0; y < in_context->height; y++) {
        for (int x = 0; x < in_context->width; x++) {
          in_frame->data[0][y * width + x] = in_rgb_data[((image * height + y) * width + x) * 3 + 0];
          in_frame->data[1][y * width + x] = in_rgb_data[((image * height + y) * width + x) * 3 + 1];
          in_frame->data[2][y * width + x] = in_rgb_data[((image * height + y) * width + x) * 3 + 2];
        }
      }

      in_frame->pts = image;

      encode(in_context, in_frame, pkt, decodePkt);
    }

    encode(in_context, nullptr, pkt, decodePkt);

    decodePkt(nullptr);

    cerr << bpp << " " << PSNR(in_rgb_data, out_rgb_data) << endl;

    avcodec_close(in_context);
    avcodec_close(out_context);
  }

  avcodec_free_context(&in_context);
  avcodec_free_context(&out_context);
  av_frame_free(&in_frame);
  av_frame_free(&out_frame);
  av_packet_free(&pkt);

  return 0;
}
