/******************************************************************************\
* SOUBOR: runlength.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef RUNLENGTH_H
#define RUNLENGTH_H

template<size_t D>
inline RunLengthEncodedBlock runLenghtEncodeBlock(const TraversedBlock<D> &input) {
  RunLengthEncodedBlock output {};

  output.push_back({0, static_cast<RunLengthAmplitudeUnit>(input[0])});

  size_t zeroes = 0;
  for (size_t pixel_index = 1; pixel_index < constpow(8, D); pixel_index++) {
    if (input[pixel_index] == 0) {
      zeroes++;
    }
    else {
      while (zeroes >= 16) {
        output.push_back({15, 0});
        zeroes -= 16;
      }
      output.push_back({static_cast<RunLengthZeroesCountUnit>(zeroes), input[pixel_index]});
      zeroes = 0;
    }
  }

  output.push_back({0, 0});

  return output;
}

template<size_t D>
inline TraversedBlock<D> runLenghtDecodeBlock(const RunLengthEncodedBlock &input) {
  TraversedBlock<D> output {};

  size_t pixel_index = 0;
  for (auto &pair: input) {
    pixel_index += pair.zeroes;
    output[pixel_index] = pair.amplitude;
    pixel_index++;
  }

  return output;
}

#endif
