extern "C" {
#include "read_png.h"
}
#include <iostream>

int main() {
  unsigned int width = 0;
  unsigned int height = 0;
  GLbyte * texture = 0;

  read_png("wood.png", &width, &height, &texture);
  assert(texture);
  assert(width > 0);
  assert(height > 0);

  for(size_t byte = 0; byte != width*height*3; ++byte)
    std::cout.put(texture[byte]);
}
