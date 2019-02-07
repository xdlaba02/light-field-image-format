/******************************************************************************\
* SOUBOR: x265test.cc
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

  ofstream output(output_file);
  output << "'x265' 'PSNR [dB]' 'bitrate [bpp]'" << endl;

  for (double bpp = 0.1; bpp <= 10; bpp *= 1.25893) {
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

    auto saveFrame = [&](AVFrame *frame) {
      sws_scale(out_convert_ctx, frame->data, frame->linesize, 0, height, rgb_frame->data, rgb_frame->linesize);

      for (int i = 0; i < rgb_frame->width * rgb_frame->height * 3; i++) {
        out_rgb_data.push_back(rgb_frame->data[0][i]);
      }
    };

    auto decodePkt = [&](AVPacket *pkt) {
      if (pkt) {
        compressed_size += pkt->size;
      }
      decode(out_context, out_frame, pkt, saveFrame);
    };

    for (size_t image = 0; image < image_count; image++) {
      uint8_t *inData[1] = { &in_rgb_data[image * width * height * 3] };
      int inLinesize[1] = { static_cast<int>(3 * width) };
      sws_scale(in_convert_ctx, inData, inLinesize, 0, height, in_frame->data, in_frame->linesize);

      in_frame->pts = image;

      encode(in_context, in_frame, pkt, decodePkt);
    }

    encode(in_context, nullptr, pkt, decodePkt);

    decodePkt(nullptr);

    avcodec_close(in_context);
    avcodec_close(out_context);

    cerr << bpp << " " << PSNR(in_rgb_data, out_rgb_data) << " " << compressed_size * 8.0 / image_pixels << endl;
    output << bpp << " " << PSNR(in_rgb_data, out_rgb_data) << " " << compressed_size * 8.0 / image_pixels << endl;
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
