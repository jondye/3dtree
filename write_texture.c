#include <iostream>
#include <memory>

#include "wood_texture.h"
const unsigned int width = 96;
const unsigned int height = 96;
char const * const data = wood_header_data;

int main()
{
  for(size_t byte = 0; byte != width*height*4; byte+=4) {
    char const * pixel = &data[byte];
    char const r = ((( pixel[0] - 33)        << 2) | ((pixel[1] - 33) >> 4));
    char const g = ((((pixel[1] - 33) & 0xF) << 4) | ((pixel[2] - 33) >> 2));
    char const b = ((((pixel[2] - 33) & 0x3) << 6) | ((pixel[3] - 33)     ));
    std::cout.put(r);
    std::cout.put(g);
    std::cout.put(b);
  }
}
