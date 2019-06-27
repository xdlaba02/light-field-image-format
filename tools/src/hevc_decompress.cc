/******************************************************************************\
* SOUBOR: hevc_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "file_mask.h"

#include <ppm.h>

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

  AVPacket *pkt                {};
  AVFrame *out_frame           {};
  AVFrame *rgb_frame           {};
  AVCodec *decoder             {};
  AVCodecContext *out_context  {};
  AVCodecParserContext *parser {};
  SwsContext *out_convert_ctx  {};

  ifstream input               {};

  const size_t INBUF_SIZE = 4096;
  uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];

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
    cerr << "Decoder AV_CODEC_ID_H265 not found" << endl;
    exit(1);
  }

  parser = av_parser_init(decoder->id);
  if (!parser) {
    cerr << "Parser not found!" << endl;
    exit(1);
  }

  out_context = avcodec_alloc_context3(decoder);
  if (!out_context) {
    cerr << "Could not allocate video coder context" << endl;
    exit(1);
  }

  if (avcodec_open2(out_context, decoder, nullptr) < 0) {
    cerr << "Could not open decoder" << endl;
    exit(1);
  }

  size_t view_counter { 0 };

  auto saveFrame = [&](AVFrame *frame) {
    PPMFileStruct ppm {};
    Pixel *ppm_row    {};

    rgb_frame->format = AV_PIX_FMT_YUV444P;
    rgb_frame->width  = frame->width;
    rgb_frame->height = frame->height;

    out_convert_ctx = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUV444P, rgb_frame->width, rgb_frame->height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
    if (!out_convert_ctx) {
      cerr << "Could not get image conversion context" << endl;
      exit(1);
    }

    int outLinesize[1] = { static_cast<int>(3 * frame->width) };
    sws_scale(out_convert_ctx, frame->data, frame->linesize, 0, frame->height, rgb_frame->data, outLinesize);

    ppm.width = rgb_frame->width;
    ppm.height = rgb_frame->height;
    ppm.color_depth = 255;

    ppm_row = allocPPMRow(ppm.width);

    size_t last_slash_pos = string(output_file_mask).find_last_of('/');

    std::string filename = get_name_from_mask(output_file_mask, '#', view_counter);

    view_counter++;

    if (last_slash_pos != string::npos) {
      string command("mkdir -p " + filename.substr(0, last_slash_pos));
      system(command.c_str());
    }

    ppm.file = fopen(filename.c_str(), "wb");
    if (!ppm.file) {
      cerr << "ERROR: CANNOT OPEN " << filename << " FOR WRITING" << endl;
      exit(1);
    }

    if (writePPMHeader(&ppm)) {
      cerr << "ERROR: CANNOT WRITE TO " << filename << endl;
      exit(1);
    }

    for (size_t row = 0; row < ppm.height; row++) {
      for (size_t col = 0; col < ppm.width; col++) {
        ppm_row[col].r = rgb_frame->data[0][(row * ppm.width + col) * 3 + 0];
        ppm_row[col].g = rgb_frame->data[0][(row * ppm.width + col) * 3 + 1];
        ppm_row[col].b = rgb_frame->data[0][(row * ppm.width + col) * 3 + 2];
      }


      if (writePPMRow(&ppm, ppm_row)) {
        cerr << "ERROR: CANNOT WRITE TO " << filename << endl;
        exit(1);
      }
    }

    fclose(ppm.file);
    freePPMRow(ppm_row);
  };

  input.open(input_file_name);
  if (!input) {
    cerr << "Could not open " << input_file_name << " for reading\n";
    exit(1);
  }

  while (input) {
    input.read(reinterpret_cast<char *>(inbuf), INBUF_SIZE);
    size_t data_size = input.gcount();
    if (!data_size) {
      break;
    }

    uint8_t *ptr = inbuf;
    while (data_size > 0) {
      int64_t ret = av_parser_parse2(parser, out_context, &pkt->data, &pkt->size, ptr, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (ret < 0) {
        cerr << "Error while parsing\n";
        exit(1);
      }

      ptr       += ret;
      data_size -= ret;

      if (pkt->size) {
        decode(out_context, out_frame, pkt, saveFrame);
      }
    }
  }

  decode(out_context, out_frame, NULL, saveFrame);

  avcodec_close(out_context);
  avcodec_free_context(&out_context);
  av_parser_close(parser);
  av_frame_free(&out_frame);
  av_frame_free(&rgb_frame);
  av_packet_free(&pkt);
  free(out_convert_ctx);

  return 0;
}
