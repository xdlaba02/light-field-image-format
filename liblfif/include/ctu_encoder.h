/**
* @file ctu_encoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 1. 9. 2020
* @copyright 2020 Drahomír Dlabaja
* @brief Functions for encoding an image with CTU.
*/

#ifndef CTU_ENCODER_H
#define CTU_ENCODER_H

template<size_t D>
class CTUEncoder {
  size_t m_quant_coef = 50;

  double m_min_mse {};
  double m_max_mse {};

  template <typename T>
  T mse(const T &a, const T &b) {
    T mse = 0.0;

    iterate_dimensions<D>(a.size(), [&](const std::array<size_t, D> &sample) {
      T diff = a[sample] - b[sample];
      mse += diff * diff;
    });

    return mse;
  }

  void quantizeByMSE(const DynamicBlock<INPUTUNIT, D> &input_block, DynamicBlock<QDATAUNIT, D> &quantized_block) {
    DynamicBlock<DCTDATAUNIT, D> dct_block(dct_block.size());
    DynamicBlock<DCTDATAUNIT, D> rounded_block(dct_block.size());
    DynamicBlock<INPUTUNIT, D> decoded_block(dct_block.size());

    forwardDiscreteCosineTransform<D>(input_block, dct_block);

    double mse {};
    while (true) {
      iterate_dimensions<D>(dct_block.size(), [&](const std::array<size_t, D> &sample) {
        rounded_block[sample] = std::round(dct_block[sample] / m_quant_coef) * m_quant_coef;
      });

      inverseDiscreteCosineTransform<D>(rounded_block, decoded_block);

      mse = mse(input_block, decoded_block);

      if (mse >= m_min_mse && mse <= m_max_mse) {
        break;
      }

      m_quant_coef *= (m_min_mse + m_max_mse) / (2 * mse);
    }

    iterate_dimensions<D>(dct_block.size(), [&](const std::array<size_t, D> &sample) {
      quantized_block[sample] = std::round(dct_block[sample] / m_quant_coef);
    });
  }

  template<typename F>
  void normalEncode(F &&puller, const std::array<size_t, D> &block_size, CABACEncoder &output) {
    DynamicBlock<INPUTTRIPLET, D> whole_block(block_size);
    DynamicBlock<INPUTUNIT, D> block(block_size);
    DynamicBlock<QDATAUNIT, D> quantized_block(block_size);

    iterate_dimensions<D>(block_size, [&](const std::array<size_t, D> &sample) {
      whole_block[sample] = puller(sample);
    });

    for (size_t channel = 0; channel < 3; channel++) {
      iterate_dimensions<D>(block_size, [&](const std::array<size_t, D> &sample) {
        block[sample] = whole_block[sample][channel];
      });

      quantizeByMSE(block, quantized_block);

      int64_t pidx = bwt(channel_data.data(), channel_data.size());
      if (pidx < 0) {
        std::cerr << "bwt error\n";
      }

      encode_bwt<D>(quantized_block, outputs[D], contexts[channel != 0], threshold, scan_table);
    }
  }

  template<typename F>
  void treeEncode(F &&puller, const std::array<size_t, D> &block_size, CABACEncoder &output) {
    std::array<D + 1, std::stringstream> output_streams {};

    for (size_t i = 0; i < D; i++) {
      if (input.size(i) > 1) {
        OBitstream bitstream(output_streams[i]);
        CABACEncoder encoder {output};
        encoder.setStream(bitstream);

        size_t halfblock_offset = input.size(i) / 2;

        std::array<D> halfblock_size(input.size());
        halfblock_size[i] = halfblock_offset;

        treeEncode<D>(puller, halfblock_size, encoder);

        auto second_puller = [&](auto image_pos) {
          image_pos[i] += halfblock_offset;
          return puller(image_pos);
        };

        halfblock_size[i] = block_size[i] - halfblock_offset;

        treeEncode<D>(second_puller, halfblock_size, encoder);
      }
    }

    DynamicBlock<INPUTUNIT, D> block(block_size);
    DynamicBlock<DCTDATAUNIT, D> dct_block(block_size);
    DynamicBlock<QDATAUNIT, D> quantized_block(block_size);

    for (size_t channel = 0; channel < 3; channel++) {

      iterate_dimensions<D>(block_size, [&](const std::array<size_t, D> &sample) {
        block[sample] = puller(sample)[channel];
      });

      forwardDiscreteCosineTransform<D>(block, dct_block);
      quantize<D>(dct_block, quantized_block, quant_coef);
      encode_bwt<D>(quantized_block, outputs[D], contexts[channel != 0], threshold, scan_table);

    }


  }

public:
  template<typename F>
  CTUEncoder(
      F &&puller,
      const std::array<D, size_t> &base_block_size,
      const std::array<D, size_t> &input_size,
      size_t max_value,
      double target_psnr,
      double psnr_tolerance,
      std::ostream &output
  ) {
    m_min_mse = (max_value * max_value) / std::pow(10, (target_psnr - psnr_tolerance) / 10);
    m_max_mse = (max_value * max_value) / std::pow(10, (target_psnr + psnr_tolerance) / 10);

    std::array<D, size_t> block_dims {};
    for (size_t i = 0; i < D; i++) {
      block_dims[i] = ceil(input_size[i] / static_cast<double>(base_block_size[i]));
    }

    OBitstream   bitstream {};
    CABACEncoder cabac     {};

    DynamicBlock<INPUTTRIPLET, D> current_block {};

    bitstream.open(&output);
    cabac.init(bitstream);

    iterate_dimensions<D>(block_dims, [&](const auto &block) {

      getBlock<D>(block_size.data(), puller, block, input_size, [&](const auto &block_pos, const auto &value) { current_block[block_pos] = value; });

      auto inputF = [&](const auto &block_pos) {
        return current_block[block_pos];
      };

      treeEncode<D>(inputF, base_block_size, cabac);
    });

    cabac.terminate();
    bitstream.flush();
  }
};

#endif
