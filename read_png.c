#include "read_png.h"
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ERROR_MSG 255

static void abort_read(char * file_name) {
   char error_msg[MAX_ERROR_MSG] = { '\0' };
   snprintf(error_msg, MAX_ERROR_MSG, "Unable to open image file %s",
     file_name);
   perror(error_msg);
   exit(1);
}

static int check_image_format(png_structp png, png_infop info) {
   char * type;

   int const bit_depth = png_get_bit_depth(png, info);
   int const interlace_type = png_get_interlace_type(png, info);
   int const color_type = png_get_color_type(png, info);
   switch(color_type) {
     case PNG_COLOR_TYPE_GRAY:    type = "Gray";        break;
     case PNG_COLOR_TYPE_GA:      type = "Gray Alpha";  break;
     case PNG_COLOR_TYPE_PALETTE: type = "Palette";     break;
     case PNG_COLOR_TYPE_RGB:     type = "RGB";         break;
     case PNG_COLOR_TYPE_RGBA:    type = "RGBA";        break;
     default:                     type = "Unknown";     break;
   }

   if(bit_depth != 8 ||
      color_type != PNG_COLOR_TYPE_RGB ||
      interlace_type != PNG_INTERLACE_NONE)
   {
     fprintf(stderr,
         "Image is %dbit, %s, %sinterlaced\n"
         "Must be 8bit, RGB, non-interlaced.\n",
         bit_depth, type,
         (interlace_type == PNG_INTERLACE_NONE) ? "non-" : "");
     return 0;
   }
   return 1;
}

/* Read a PNG file.  Returns the width and height and the data in RGB format.
 * The file must be RGB, 8-bit, non-interlaced. */
void read_png(char * file_name, unsigned int * width_ptr,
              unsigned int * height_ptr, GLbyte ** data_ptr) {

   FILE * file = fopen(file_name, "rb");
   if( !file ) abort_read(file_name);

   /* Create and initialize the png_struct with the default error handler
      functions. */
   png_structp png_ptr =
	   png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if(!png_ptr) {
      fclose(file);
      abort_read(file_name);
   }

   /* Allocate and initialize the memory for image information. */
   png_infop info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      fclose(file);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      abort_read(file_name);
   }

   /* Set up the input control for using standard C streams */
   png_init_io(png_ptr, file);

   int png_transforms = 0; /* No transformations */

   /* Read entire image */
   png_read_png(png_ptr, info_ptr, png_transforms, NULL);

   png_uint_32 const width = png_get_image_width(png_ptr, info_ptr);
   png_uint_32 const height = png_get_image_height(png_ptr, info_ptr);

   if(!check_image_format(png_ptr, info_ptr))
   {
     fprintf(stderr, "Error while processing %s\n", file_name);
     exit(1);
   }

   *width_ptr = width;
   *height_ptr = height;
   *data_ptr = (GLbyte*) malloc(width*height*3);

   /* Copy the data to the texture array */
   png_bytepp image_rows = png_get_rows(png_ptr, info_ptr);
   int const bytes_per_row = png_get_rowbytes(png_ptr, info_ptr);
   GLbyte * destination = (*data_ptr);
   for(int row = 0; row < height; ++row) {
     memcpy(destination, image_rows[row], bytes_per_row);
     destination += bytes_per_row;
   }

   /* clean up after the read, and free any memory allocated */
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

   /* close the file */
   fclose(file);
}

