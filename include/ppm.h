/*******************************************************\
* SOUBOR: ppm.h
* AUTOR: Drahomir Dlabaja (xdlaba02)
* DATUM: 19. 10. 2018
\*******************************************************/

#ifndef PPM_H
#define PPM_H

#include <fstream>
#include <string>
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
* Funkce proparsuje a nacte data ppm souboru filename
  do bufferu data.
* Velikosti obrazku ulozi do prommene width a height.
\*******************************************************/
bool loadPPM(const char *filename, uint64_t &width, uint64_t &height, vector<uint8_t> &data);

/*******************************************************\
* Funkce vytvori ppm soubor filename a ulozi do nej
  rgb data.
\*******************************************************/
bool savePPM(const char *filename, const uint64_t width, const uint64_t height, vector<uint8_t> &data);

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
