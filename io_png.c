/**************************************************************
 *  PNG FILE I/O ROUTINES. 
 *  Currently these allocates then frees memory but should
 *  optimise properly.
 **************************************************************/

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include "spiceweasel.h"

int read_png(char *filename, TRawFrame *frame)
{
  unsigned char header[8];
  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;
  int channels, width, height, bit_depth, color_type, rowbytes;
  int nbytes;
  int i;
  
  /* OPEN FILE */
  if(!(fp = fopen(filename, "rb")))
    return(IO_ERROR_OPEN);  /* Could not open */

  /* READ THE FILE HEADER */
  fread(header, 1, 7, fp);
  if(png_sig_cmp(header, 0, 7)) {
    return(IO_ERROR_FORMAT); /* File not PNG */
  }
  fclose(fp);
  fp = fopen(filename, "rb");
  
  /* ALLOCATE MEMORY */

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
				   NULL, NULL, NULL);
  if(!png_ptr)
    return(IO_ERROR_OTHER); /* PNG Error */

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr,
			    (png_infopp)NULL, (png_infopp)NULL);
    return(IO_ERROR_OTHER); /* PNG Error */
  }

  /* READ PNG HEADER */
  png_init_io(png_ptr, fp);

  png_read_info(png_ptr, info_ptr);

  /* Extract header */

  channels   = png_get_channels(png_ptr, info_ptr);
  width      = png_get_image_width(png_ptr, info_ptr);
  height     = png_get_image_height(png_ptr, info_ptr);
  bit_depth  = png_get_bit_depth(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  rowbytes   = png_get_rowbytes(png_ptr, info_ptr);
  
  /* SET INPUT TRANSFORMATIONS */

  /* Strip out alpha channel */
  if (color_type & PNG_COLOR_MASK_ALPHA)
    png_set_strip_alpha(png_ptr);
  
  /* Store each pixel in at least one byte */
  if (bit_depth < 8)
    png_set_packing(png_ptr);

  /* Number of bytes per pixel */
  nbytes = 1;
  if(bit_depth > 8)
    nbytes = 2;
 
  frame->channels = channels;

  /* Check if the frame has been allocated */
  if(frame->allocated) {
    /* Data already allocated - check same size */
    if((frame->width != width) || 
       (frame->height != height) || 
       (frame->rowbytes != rowbytes)) {
      return(IO_ERROR_SIZE);
    }  
  }else {
    /* Allocate memory. Note organised in COLUMNS */
    frame->data = (unsigned char**) malloc(sizeof(unsigned char*)*height);
    for(i=0;i!=height;i++)
      frame->data[i] = (unsigned char*) malloc(rowbytes*sizeof(unsigned char));
    frame->width = width;
    frame->height = height;
    frame->allocated =  1;
    frame->bpp = bit_depth;
    frame->rowbytes = rowbytes;
  }

  /* READ PNG DATA */
  
  png_read_image(png_ptr, frame->data);
  
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  
  fclose(fp);
  
  return(0);
}

int write_png(char *filename, TRawFrame *frame)
{
  FILE *fp;

  png_structp png_ptr;
  png_infop   info_ptr;

  /* OPEN FILE */
  if(!(fp = fopen(filename, "wb")))
    return(IO_ERROR_OPEN);  /* Could not open */

  /* Allocate memory */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    return(IO_ERROR_OTHER);
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr,
			     (png_infopp)NULL);
    return(IO_ERROR_OTHER);
  }

  png_init_io(png_ptr, fp);

  if(frame->channels == 1) {
    /* Write greyscale */
    png_set_IHDR(png_ptr, info_ptr, frame->width, frame->height,
		 frame->bpp, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  }else {
    /* Should be 3 channel RGB */
    png_set_IHDR(png_ptr, info_ptr, frame->width, frame->height,
		 frame->bpp,PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  }
  png_set_rows(png_ptr, info_ptr, frame->data);

  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  png_write_end(png_ptr, NULL);
  
  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);

  return(0);
}
