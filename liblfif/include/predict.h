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

template <size_t D>
struct interpolate {
  template <typename F>
  interpolate(const size_t BS[D], F &&main_ref, const int64_t main_ref_pos[D], int8_t multiplier, INPUTUNIT &output) {
    int64_t pos = std::floor(main_ref_pos[D - 1] / static_cast<double>(multiplier));
    int64_t frac = main_ref_pos[D - 1] % multiplier;

    auto inputF1 = [&](size_t index) {
      return main_ref(pos * get_stride<D - 1>(BS) + index);
    };

    if (frac == 0) {
      interpolate<D - 1>(BS, inputF1, main_ref_pos, multiplier, output);
    }
    else {
      INPUTUNIT val1 {};
      INPUTUNIT val2 {};

      auto inputF2 = [&](size_t index) {
        return main_ref((pos + 1) * get_stride<D - 1>(BS) + index);
      };

      interpolate<D - 1>(BS, inputF1, main_ref_pos, multiplier, val1);
      interpolate<D - 1>(BS, inputF2, main_ref_pos, multiplier, val2);

      output = (val1 * (multiplier - frac) + val2 * frac) / multiplier;
    }
  }
};

template <>
struct interpolate<0> {
  template <typename F>
  interpolate(const size_t *, F &&main_ref, const int64_t *, int8_t, INPUTUNIT &output) {
    output = main_ref(0);
  }
};

template <size_t D>
struct low_pass_sum {
  template <typename F>
  low_pass_sum(const size_t BS[D], F &&input) {
    for (size_t slice = 0; slice < BS[D - 1]; slice++) {
      auto inputF = [&](size_t index) -> auto & {
        return input(slice * get_stride<D - 1>(BS) + index);
      };

      low_pass_sum<D - 1>(BS, inputF);
    }

    for (size_t noodle = 0; noodle < get_stride<D - 1>(BS); noodle++) {
      auto inputF = [&](size_t index) -> auto & {
        return input(index * get_stride<D - 1>(BS) + noodle);
      };

      low_pass_sum<1>(&BS[D - 1], inputF);
    }
  }
};

template <>
struct low_pass_sum<1> {
  template <typename F>
  low_pass_sum(const size_t BS[1], F &&input) {
    auto prev_value = input(0);

    for (size_t i { 0 }; i < BS[0] - 1; i++) {
      auto tmp = input(i);
      input(i) = prev_value + 2 * input(i) + input(i + 1);
      prev_value = tmp;
    }

    input(BS[0] - 1) = prev_value + 3 * input(BS[0] - 1);
  }
};

template <size_t D>
void low_pass_filter(DynamicBlock<INPUTUNIT, D> &main_ref) {
  auto inputF = [&](size_t index) -> auto & {
    return main_ref[index];
  };

  low_pass_sum<D>(main_ref.size().data(), inputF);

  for (size_t i { 0 }; i < get_stride<D>(main_ref.size()); i++) {
    main_ref[i] /= constpow(4, D);
  }
}

template <size_t D, typename F>
void project_neighbours_to_main_ref(const std::array<size_t, D> &BS, DynamicBlock<INPUTUNIT, D - 1> &main_ref, const int8_t direction[D], size_t main_ref_idx, F &&inputF) {
  iterate_dimensions<D - 1>(main_ref.size(), [&](const std::array<size_t, D - 1> &pos) {
    std::array<int64_t, D> position {};

    for (size_t i {}; i < D - 1; i++) {
      size_t idx = i < main_ref_idx ? i : i + 1;
      position[idx] = pos[i];

      if (direction[idx] >= 0) {
        position[idx] -= BS[main_ref_idx];
      }
      if (direction[idx] <= 0) {
        position[idx] -= 1;
      }
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
    for (size_t i { 0 }; i < D; i++) {
      position[i] *= direction[nearest_neighbour_idx];
      position[i] -= direction[i] * num_steps;
      position[i] /= direction[nearest_neighbour_idx];

      if (position[i] < 0) {
        position[i] = 0;
      }

      else if ((direction[i] >= 0) && (position[i] > static_cast<int64_t>(BS[i]))) { // is this important?
        position[i] = BS[i];
      }
      else if (position[i] > static_cast<int64_t>(BS[i]) * 2) { // is THIS important?
        position[i] = BS[i] * 2;
      }


      if (direction[i] > 0) {
        position[i]--;
      }
    }

    main_ref[pos] = inputF(position);
  });
}

template <size_t D>
void predict_from_main_ref(DynamicBlock<INPUTUNIT, D> &output, const int8_t direction[D], const DynamicBlock<INPUTUNIT, D - 1> &main_ref, size_t main_ref_idx) {
  iterate_dimensions<D>(output.size(), [&](std::array<size_t, D> &pos) {
    int64_t main_ref_pos[D - 1] {};
    for (size_t i { 0 }; i < D - 1; i++) {
      size_t idx = i < main_ref_idx ? i : i + 1;
      main_ref_pos[i]  = pos[idx] + 1; //zjisti souradnici z indexu

      if (direction[idx] >= 0) {
        main_ref_pos[i] += output.size(main_ref_idx);
      }

      main_ref_pos[i] *= direction[main_ref_idx]; //vynasobi se tak, aby se nemuselo konvertovat do floating point
      main_ref_pos[i] -= direction[idx] * (pos[main_ref_idx] + 1); //vytvori projekci souradnice na hlavni referencni rovinu


      if (main_ref_pos[i] > static_cast<int64_t>(output.size(idx) + output.size(main_ref_idx)) * direction[main_ref_idx]) {
        main_ref_pos[i] = (output.size(idx) + output.size(main_ref_idx)) * direction[main_ref_idx];
      }
    }

    auto inputF = [&](size_t index) {
      return main_ref[index];
    };

    interpolate<D - 1>(main_ref.size().data(), inputF, main_ref_pos, direction[main_ref_idx], output[pos]);
  });
}

template <size_t D, typename F>
void predict_direction(DynamicBlock<INPUTUNIT, D> &output, const int8_t direction[D], F &&inputF) {
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

  size_t ref_size[D - 1] {};

  for (size_t i {}; i < D - 1; i++) {
    size_t idx = i < main_ref_idx ? i : i + 1;
    ref_size[i] = output.size(idx) + output.size(main_ref_idx) + 1;
  }

  DynamicBlock<INPUTUNIT, D - 1> ref(ref_size);

  project_neighbours_to_main_ref<D>(output.size(), ref, direction, main_ref_idx, inputF);
  low_pass_filter<D - 1>(ref);
  predict_from_main_ref<D>(output, direction, ref, main_ref_idx);
}

template<size_t D, typename F>
void predict_DC(DynamicBlock<INPUTUNIT, D> &output, F &inputF) {
  INPUTUNIT sum         {};
  size_t    samples_cnt {};

  for (size_t neighbour_idx { 0 }; neighbour_idx < D; neighbour_idx++) {
    std::array<size_t, D - 1> neighbour_block_size {};
    for (size_t i { 0 }; i < D - 1; i++) {
      size_t idx              = i < neighbour_idx ? i : i + 1;
      neighbour_block_size[i] = output.size()[idx];
    }

    samples_cnt += get_stride<D - 1>(neighbour_block_size.data());

    iterate_dimensions<D - 1>(neighbour_block_size, [&](const std::array<size_t, D - 1> &pos) {
      std::array<int64_t, D> position {};
      position[neighbour_idx]--;

      for (size_t i { 0 }; i < D - 1; i++) {
        size_t idx     = i < neighbour_idx ? i : i + 1;
        position[idx] += pos[i];
      }

      sum += inputF(position);
    });
  }

  for (size_t i { 0 }; i < get_stride<D>(output.size()); i++) {
    output[i] = sum / samples_cnt;
  }
}

template<size_t D, typename F>
void predict_planar(DynamicBlock<INPUTUNIT, D> &output, F &inputF) {
  output.fill(0);

  for (size_t neighbour_idx { 0 }; neighbour_idx < D; neighbour_idx++) {
    DynamicBlock<INPUTUNIT, D> tmp_prediction(output.size());

    int8_t direction[D] {};
    direction[neighbour_idx] = 1;

    predict_direction<D>(tmp_prediction, direction, inputF);

    for (size_t i { 0 }; i < get_stride<D>(tmp_prediction.size()); i++) {
      output[i] += tmp_prediction[i];
    }
  }

  for (size_t i { 0 }; i < output.stride(D); i++) {
    output[i] /= D;
  }
}

#endif
