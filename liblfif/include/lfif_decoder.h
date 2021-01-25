/**
* @file lfif_decoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for decoding an image.
*/

#pragma once

#include "components/bitstream.h"
#include "components/colorspace.h"
#include "components/predict.h"

#include "block_predictor.h"
#include "block_encoder_dct.h"
#include "lfif.h"

#include <cstdint>

#include <istream>
#include <sstream>


/**
* @brief Function which decodes CABAC encoded image from stream.
* @param dec The decoder structure.
* @param input Input stream from which the image will be decoded.
* @param output Output callback function which will be returning pixels with signature void output(size_t index, Array<float, 3> value), where index is a pixel index in memory and value is said pixel.
*/
template<size_t D, typename F>
void decodeStreamDCT(std::istream &input, const LFIF<D> &image, F &&pusher) {
  StackAllocator::init(2147483648); //FIXME

  DynamicBlock<float, D> current_block[3] {image.block_size, image.block_size, image.block_size};

  Array<size_t, D> aligned_image_size {};
  for (size_t i = 0; i < D; i++) {
    aligned_image_size[i] = image.size[i] + image.block_size[i] - (image.size[i] % image.block_size[i]);
  }

  IBitstream   bitstream {};
  CABACDecoder cabac     {};

  BlockEncoderDCT<D> block_decoder_Y(image.block_size, image.discarded_bits);
  BlockEncoderDCT<D> block_decoder_UV(image.block_size, image.discarded_bits);

  BlockPredictor<D> predictorY(image.size);
  BlockPredictor<D> predictorU(image.size);
  BlockPredictor<D> predictorV(image.size);

  bitstream.open(input);
  cabac.init(bitstream);

  block_for<D>({}, image.block_size, aligned_image_size, [&](const Array<size_t, D> &offset) {
    for (size_t i = 0; i < D; i++) {
      std::cerr << offset[i] << " ";
    }
    std::cerr << " out of ";
    for (size_t i = 0; i < D; i++) {
      std::cerr << aligned_image_size[i] << " ";
    }
    std::cerr << "\n";

    block_decoder_Y.decodeBlock(cabac, current_block[0]);
    block_decoder_UV.decodeBlock(cabac, current_block[1]);
    block_decoder_UV.decodeBlock(cabac, current_block[2]);

    if (image.predicted) {
      auto prediction_type = predictorY.decodePredictionType(cabac);
      predictorY.backwardPass(current_block[0], offset, prediction_type);
      predictorU.backwardPass(current_block[1], offset, prediction_type);
      predictorV.backwardPass(current_block[2], offset, prediction_type);
    }

    moveBlock<D>([&](const auto &pos) {
      float Y  = current_block[0][pos] + pow(2, image.depth_bits - 1);
      float Cb = current_block[1][pos];
      float Cr = current_block[2][pos];

      uint16_t R = std::clamp<float>(std::round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, std::pow(2, image.depth_bits) - 1);
      uint16_t G = std::clamp<float>(std::round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, std::pow(2, image.depth_bits) - 1);
      uint16_t B = std::clamp<float>(std::round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, std::pow(2, image.depth_bits) - 1);

      return std::array<uint16_t, 3> { R, G, B };
    }, image.block_size, {},
    pusher, image.size, offset,
    image.block_size);
  });

  cabac.terminate();

  StackAllocator::cleanup();
}
