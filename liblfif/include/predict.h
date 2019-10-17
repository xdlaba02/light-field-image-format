/**
* @file predict.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 9. 8. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Prediction stuff.
*/

#ifndef PREDICT_H
#define PREDICT_H

#include "lfiftypes.h"

#include <cstdint>
#include <cassert>
#include <cmath>

//TODO
// INTERPOLACE PRI PROMITANI NA HLAVNI REFERENCI - PROBLY ZTRATA CASU, viz HEVC, implementoval jsem aspon lepsi zaokrouhlovani

template <size_t BS, size_t D>
struct interpolate {
  template <typename F>
  interpolate(F &&main_ref, const int64_t main_ref_pos[D], int8_t multiplier, INPUTUNIT &output) {
    int64_t pos = std::floor(main_ref_pos[D - 1] / static_cast<double>(multiplier));
    int64_t frac = main_ref_pos[D - 1] % multiplier;

    auto inputF1 = [&](size_t index) {
      return main_ref(pos * constpow(BS, D - 1) + index);
    };

    if (frac == 0) {
      interpolate<BS, D - 1>(inputF1, main_ref_pos, multiplier, output);
    }
    else {
      INPUTUNIT val1 {};
      INPUTUNIT val2 {};

      auto inputF2 = [&](size_t index) {
        return main_ref((pos + 1) * constpow(BS, D - 1) + index);
      };

      interpolate<BS, D - 1>(inputF1, main_ref_pos, multiplier, val1);
      interpolate<BS, D - 1>(inputF2, main_ref_pos, multiplier, val2);

      output = (val1 * (multiplier - frac) + val2 * frac) / multiplier;
    }
  }
};

template <size_t BS>
struct interpolate<BS, 0> {
  template <typename F>
  interpolate(F &&main_ref, const int64_t *, int8_t, INPUTUNIT &output) {
    output = main_ref(0);
  }
};

template <size_t BS, size_t D>
struct low_pass_sum {
  template <typename F>
  low_pass_sum(F &&input) {
    for (size_t slice = 0; slice < BS; slice++) {
      auto inputF = [&](size_t index) -> auto & {
        return input(slice * constpow(BS, D - 1) + index);
      };

      low_pass_sum<BS, D - 1>{inputF}; //brackets used to disambiguate between creating temporary object and creating object named inputF
    }

    for (size_t noodle = 0; noodle < constpow(BS, D - 1); noodle++) {
      auto inputF = [&](size_t index) -> auto & {
        return input(index * constpow(BS, D - 1) + noodle);
      };

      low_pass_sum<BS, 1>{inputF};
    }
  }
};

template <size_t BS>
struct low_pass_sum<BS, 1> {
  template <typename F>
  low_pass_sum(F &&input) {
    auto prev_value = input(0);

    for (size_t i { 0 }; i < BS - 1; i++) {
      auto tmp = input(i);
      input(i) = prev_value + 2 * input(i) + input(i + 1);
      prev_value = tmp;
    }

    input(BS - 1) = prev_value + 3 * input(BS - 1);
  }
};

template <size_t BS, size_t D>
void low_pass_filter(Block<INPUTUNIT, BS * 2 + 1, D - 1> &main_ref) {
  auto inputF = [&](size_t index) -> auto & {
    return main_ref[index];
  };

  low_pass_sum<BS * 2 + 1, D - 1>{inputF};

  for (size_t i { 0 }; i < constpow(BS * 2 + 1, D - 1); i++) {
    main_ref[i] /= constpow(4, D - 1);
  }
}

template <size_t BS, size_t D>
void project_neighbours_to_main_ref(Block<INPUTUNIT, BS * 2 + 1, D - 1> &main_ref, const int8_t direction[D], const INPUTUNIT *src, const std::array<size_t, D + 1> &input_stride, size_t main_ref_idx, const std::array<int64_t, D - 1> &ref_offsets) {
  for (size_t i { 0 }; i < constpow(BS * 2 + 1, D - 1); i++) {
    std::array<int64_t, D> position {};

    for (size_t d { 0 }; d < D - 1; d++) {
      size_t idx = d < main_ref_idx ? d : d + 1;
      position[idx] = i % constpow(BS * 2 + 1, d + 1) / constpow(BS * 2 + 1, d);
      position[idx] -= ref_offsets[d];
    }

    size_t nearest_neighbour_idx {};

    // najde nejblizsi stenu po vektoru direction
    double neighbour_distance {};
    for (size_t d { 0 }; d < D; d++) {
      if (direction[d] > 0) {
        double distance {};
        distance = static_cast<double>(position[d]) / direction[d];
        if (distance <= neighbour_distance) {
          neighbour_distance = distance;
          nearest_neighbour_idx = d;
        }
      }
    }

    int64_t num_steps = position[nearest_neighbour_idx];

    //posune soradnice podle vektoru diection tak, aby byly pokud mozno vsechny vetsi nebo rovno nule
    for (size_t d { 0 }; d < D; d++) {
      position[d] *= direction[nearest_neighbour_idx];
      position[d] -= direction[d] * num_steps;
      position[d] /= direction[nearest_neighbour_idx];

      if (position[d] < 0) {
        position[d] = 0;
      }
      else if (direction[d] >= 0 && position[d] >= (static_cast<int64_t>(BS) + 1)) {
        position[d] = BS;
      }
      else if (position[d] >= (static_cast<int64_t>(BS) * 2 + 1)) {
        position[d] = BS * 2;
      }
    }

    int64_t src_idx {};
    for (size_t d = 0; d < D; d++) {
      src_idx += position[d] * input_stride[d];
    }

    main_ref[i] = src[src_idx];
  }
}

template <size_t BS, size_t D>
void predict_from_main_ref(Block<INPUTUNIT, BS, D> &output, const int8_t direction[D], const Block<INPUTUNIT, BS * 2 + 1, D - 1> &main_ref, size_t main_ref_idx, const std::array<int64_t, D - 1> &ref_offsets) {
  for (size_t i { 0 }; i < constpow(BS, D); i++) {

    int64_t main_ref_distance = i % constpow(BS, main_ref_idx + 1) / constpow(BS, main_ref_idx);

    int64_t main_ref_pos[D - 1] {};
    for (size_t d { 0 }; d < D - 1; d++) {
      size_t idx = d < main_ref_idx ? d : d + 1;
      main_ref_pos[d]  = i % constpow(BS, idx + 1) / constpow(BS, idx); //zjisti souradnici z indexu
      main_ref_pos[d] += ref_offsets[d]; //posune se podle smeru projekce
      main_ref_pos[d] *= direction[main_ref_idx]; //vynasobi se tak, aby se nemuselo pocitat s floating point
      main_ref_pos[d] -= direction[idx] * main_ref_distance; //vytvori projekci souradnice na hlavni referencni rovinu
    }

    auto inputF = [&](size_t index) {
      return main_ref[index];
    };

    interpolate<BS * 2 + 1, D - 1>(inputF, main_ref_pos, direction[main_ref_idx], output[i]);
  }
}

template <size_t BS, size_t D>
void predict_direction(Block<INPUTUNIT, BS, D> &output, const int8_t direction[D], const INPUTUNIT *src, const std::array<size_t, D + 1> &input_stride) {
  Block<INPUTUNIT, BS * 2 + 1, D - 1> ref {};

  int64_t ptr_offset   { 0 };
  size_t  main_ref_idx { 0 };

  // find which neighbouring block will be main
  for (size_t d = 0; d < D; d++) {
    if (direction[d] >= direction[main_ref_idx]) {
      main_ref_idx = d;
    }
  }

  if (direction[main_ref_idx] <= 0) {
    return;
  }

  std::array<int64_t, D - 1> ref_offsets {};
  for (size_t d { 0 }; d < D - 1; d++) {
    size_t idx {};
    idx = d < main_ref_idx ? d : d + 1;

    if (direction[idx] >= 0) {
      ref_offsets[d] += BS;
    }
    if (direction[idx] <= 0) {
      ref_offsets[d] += 1;
    }
  }

  for (size_t d { 0 }; d < D; d++) {
    if (direction[d] > 0) {
      ptr_offset -= input_stride[d];
    }
  }

  project_neighbours_to_main_ref<BS, D>(ref, direction, &src[ptr_offset], input_stride, main_ref_idx, ref_offsets);
  low_pass_filter<BS, D>(ref);
  predict_from_main_ref<BS, D>(output, direction, ref, main_ref_idx, ref_offsets);
}

template<size_t BS, size_t D>
void predict_DC(Block<INPUTUNIT, BS, D> &output, const bool neighbours[D], const INPUTUNIT *src, const std::array<size_t, D + 1> &input_stride) {
  INPUTUNIT sum { 0 };
  size_t    neighbour_cnt { 0 };

  for (size_t neighbour_idx { 0 }; neighbour_idx < D; neighbour_idx++) {
    if (neighbours[neighbour_idx]) {
      for (size_t i { 0 }; i < constpow(BS, D - 1); i++) {
        int64_t offset {};

        for (size_t d {0}; d < D; d++) {
          if (d == neighbour_idx) {
            offset -= input_stride[d];
          }
          else {
            size_t pow_val { d < neighbour_idx ? d : d - 1 };
            offset += ((i % constpow(BS, pow_val + 1)) / constpow(BS, pow_val)) * input_stride[d];
          }
        }

        sum += src[offset];
      }

      neighbour_cnt++;
    }
  }

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    output[i] = sum / (constpow(BS, D - 1) * neighbour_cnt);
  }
}

template<size_t BS, size_t D>
void predict_planar(Block<INPUTUNIT, BS, D> &output, const bool neighbours[D], const INPUTUNIT *src, const std::array<size_t, D + 1> &input_stride) {
  Block<INPUTUNIT, BS, D> tmp_prediction {};
  size_t                  neighbour_cnt  { 0 };

  output.fill(0);

  for (size_t neighbour_idx { 0 }; neighbour_idx < D; neighbour_idx++) {
    if (neighbours[neighbour_idx]) {
      int8_t direction[D] {};

      direction[neighbour_idx] = 1;

      predict_direction<BS, D>(tmp_prediction, direction, src, input_stride);

      for (size_t i { 0 }; i < constpow(BS, D); i++) {
        output[i] += tmp_prediction[i];
      }

      neighbour_cnt++;
    }
  }

  for (size_t i { 0 }; i < constpow(BS, D); i++) {
    output[i] /= neighbour_cnt;
  }
}

#endif
