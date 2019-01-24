/******************************************************************************\
* SOUBOR: ppm.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#ifndef PPM_H
#define PPM_H

#include <fstream>
#include <vector>

using namespace std;

/******************************************************************************\
* Stavy automatu parsujici hlavicku PPM souboru.
\******************************************************************************/
enum State {
  STATE_INIT,
  STATE_P,
  STATE_P6,
  STATE_P6_SPACE,
  STATE_WIDTH,
  STATE_WIDTH_SPACE,
  STATE_HEIGHT,
  STATE_HEIGHT_SPACE,
  STATE_DEPTH,
  STATE_END
};

/******************************************************************************\
* Funkce proparsuje data ze streamu input do bufferu
  rgb_data.
* Velikosti obrazku ulozi do prommene width a height.
\******************************************************************************/
int readPPM(ifstream &input, vector<uint8_t> &rgb_data, uint64_t &width, uint64_t &height, uint32_t &color_depth);

/******************************************************************************\
* Funkce odesle PPM data z bufferu do streamu output.
\******************************************************************************/
int writePPM(const uint8_t *rgb_data, uint64_t width, uint64_t height, uint32_t color_depth, ofstream &output);

/******************************************************************************\
* Funkce ve streamu preskoci na prvni znak za koncem
  prvniho radku, na ktery narazi.
\******************************************************************************/
void skipUntilEol(ifstream &input);

/******************************************************************************\
* Funkce proparsuje hlavicku ppm souboru a data z ni
  vrati v patricnych prommenych.
\******************************************************************************/
bool parseHeader(ifstream &input, uint64_t &width, uint64_t &height, uint32_t &depth);

#endif
