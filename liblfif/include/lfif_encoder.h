/**
* @file lfif_encoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for encoding an image.
*/

#pragma once

#include "components/colorspace.h"
#include "components/bitstream.h"

#include "block_predictor.h"
#include "block_encoder_dct.h"
#include "lfif.h"

#include <cstdint>

#include <ostream>
#include <sstream>
#include <map>

/**
* @brief Function which encodes the image to the stream.
* @param enc The encoder structure.
* @param input Callback function specified by client with signature T input(size_t index), where T is pixel/sample type.
* @param output Output stream to which the image shall be encoded.
*/
template<size_t D, typename IF, typename OF>
void encodeStreamDCT(const LFIF<D> &image, std::ostream &output, IF &&puller) {
  StackAllocator::init(2147483648); //FIXME

  DynamicBlock<float, D> current_block[3] {image.block_size, image.block_size, image.block_size};

  OBitstream   bitstream {};
  CABACEncoder cabac     {};

  BlockEncoderDCT block_encoder_Y(image.block_size, image.discarded_bits);
  BlockEncoderDCT block_encoder_UV(image.block_size, image.discarded_bits);

  BlockPredictor predictorY(image.size);
  BlockPredictor predictorU(image.size);
  BlockPredictor predictorV(image.size);

  bitstream.open(output);
  cabac.init(bitstream);

  Array<size_t, D> aligned_image_size {};
  for (size_t i = 0; i < D; i++) {
    aligned_image_size[i] = aligned_image_size[i] = image.size[i] + image.block_size[i] - (image.size[i] % image.block_size[i]);
  }

  block_for<D>({}, image.block_size, aligned_image_size, [&](const Array<size_t, D> &offset) {
    for (size_t i = 0; i < D; i++) {
      std::cerr << offset[i] << " ";
    }
    std::cerr << " out of ";
    for (size_t i { 0 }; i < D; i++) {
      std::cerr << aligned_image_size[i] << " ";
    }
    std::cerr << "\n";

    moveBlock<D>(puller, image.size, offset,
                 [&](const auto &block_pos, const auto &value) {
                   current_block[0][block_pos] = YCbCr::RGBToY(value[0], value[1], value[2]) - pow(2, image.depth_bits - 1);
                   current_block[1][block_pos] = YCbCr::RGBToCb(value[0], value[1], value[2]);
                   current_block[2][block_pos] = YCbCr::RGBToCr(value[0], value[1], value[2]);
                 }, image.block_size, {},
                 image.block_size);


    BlockPredictor<D>::PredictionType prediction_type {};
    if (image.predicted) {
      prediction_type = predictorY.selectPredictionType(current_block[0], offset);

      predictorY.forwardPass(current_block[0], offset, prediction_type)
      predictorU.forwardPass(current_block[1], offset, prediction_type)
      predictorV.forwardPass(current_block[2], offset, prediction_type)
    }

    block_encoder_Y.encodeBlock(current_block[0], cabac);
    block_encoder_UV.encodeBlock(current_block[1], cabac);
    block_encoder_UV.encodeBlock(current_block[2], cabac);

    if (image.predicted) {
      predictorY.encodePredictionType<D>(prediction_type, cabac);

      //TODO decode encoded blocks from current_block[i], otherwise it does not work

      predictorY.backwardPass(current_block[0], offset, prediction_type)
      predictorU.backwardPass(current_block[1], offset, prediction_type)
      predictorV.backwardPass(current_block[2], offset, prediction_type)
    }
  });

  cabac.terminate();
  bitstream.flush();

  StackAllocator::cleanup();
}
