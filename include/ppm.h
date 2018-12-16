/*******************************************************\
* SOUBOR: ppm.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef PPM_H
#define PPM_H

#include <fstream>
#include <vector>

using namespace std;

/*******************************************************\
* Stavy automatu parsujici hlavicku PPM souboru.
\*******************************************************/
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

/*******************************************************\
* Funkce proparsuje data ze streamu input do bufferu
  rgb_data.
* Velikosti obrazku ulozi do prommene width a height.
\*******************************************************/
bool readPPM(ifstream &input, uint64_t &width, uint64_t &height, vector<uint8_t> &rgb_data);

/*******************************************************\
* Funkce odesle PPM data z bufferu do streamu output.
\*******************************************************/
bool writePPM(ofstream &output, const uint64_t width, const uint64_t height, const uint8_t *rgb_data);

/*******************************************************\
* Funkce ve streamu preskoci na prvni znak za koncem
  prvniho radku, na ktery narazi.
\*******************************************************/
void skipUntilEol(ifstream &input);

/*******************************************************\
* Funkce proparsuje hlavicku ppm souboru a data z ni
  vrati v patricnych prommenych.
\*******************************************************/
bool parseHeader(ifstream &input, uint64_t &width, uint64_t &height, uint32_t &depth);

/*******************************************************\
* Funkce vytvori hlavicku ppm souboru a zapise ji
  do streamu.
\*******************************************************/
void writeHeader(ofstream &output, uint64_t width, uint64_t height, uint32_t depth);

#endif
