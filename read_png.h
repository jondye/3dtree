#ifndef read_png_h
#define read_png_h

#include <GL/gl.h>

/* Read a PNG file.  Returns the width and height and the data in RGB format.
 * A buffer is allocated by the function to hold the data and must be freed by
 * the client.
 * The file must be RGB, 8-bit, non-interlaced. */
void read_png(char * file_name,
              unsigned int * width_ptr,
              unsigned int * height_ptr,
              GLbyte ** data_ptr);

#endif /* read_png_h */
