/***********************************************************
 * Main routine dealing with reading data from input files
 * 
 * MIT LICENSE:
 *
 * Copyright (c) 2006 B.Dudson, UKAEA Fusion and Oxford University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Changelog: 
 *
 * October 2006: Initial release by Ben Dudson, UKAEA Fusion/Oxford University
 *
 ***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "spiceweasel.h"

#define IPXGLOBALORIGIN
#include "io_ipx.h"

TRawFrame readraw;  /* Temporary storage for reading */

void read_init()
{
  readraw.allocated = 0;

  if(input_format == FORMAT_IPX) {
    /* IPX - one file containing all images */
    printf("Reading IPX file %s...", input_template);
    fflush(stdout);
    if(IPX_read_open(input_template, &ipx_read_status)) {
      printf("failed!\n");
      exit(1);
    }
    printf("done\n");
  }
}

void read_finish()
{
  if(input_format == FORMAT_IPX) {
    IPX_read_close(&ipx_read_status);
  }
}

int read_frame(int number, TFrame *frame)
{
  char filename[MAX_NAME_LEN];
  int errcode;
  int i, j;
  float factor;

  frame->time = 0.0;

  if(input_format == FORMAT_IPX) { /* IPX VIDEO FORMAT */
    /* IPX code reads in a single frame and converts to floats */
    if(IPX_read_frame(number, frame, &ipx_read_status)) {
      printf("Error: Could not read frame %d from IPX file %s\n", number, input_template);
      exit(1);
    }
    
  }else { /* A SET OF FRAME STILLS */

    /* Get the filename */
    sprintf(filename, input_template, number);

    /* Read the data in raw format */

    errcode = 0;
    switch(input_format) {
    case FORMAT_BMP: {
      errcode = read_bmp(filename, &readraw);
      break;
    }
    case FORMAT_PNG: {
      errcode = read_png(filename, &readraw);
      break;
    }
    default: {
      errcode = IO_ERROR_FORMAT;
    }
    }
    
    /* Check error code */
    
    if(errcode) {
      switch(errcode) {
      case IO_ERROR_OPEN: {
	printf("Error: Could not open input file %s\n", filename);
	break;
      }
      case IO_ERROR_FORMAT: {
	printf("Error: Input file %s has an invalid file format\n", filename);
	break;
      }
      case IO_ERROR_SIZE: {
	printf("Error: Size of frame %s different to previous frames\n", filename);
	break;
      }
      case IO_ERROR_OTHER: {
	printf("Error: Could not read input file %s\n", filename);
	break;
      }
      }
      exit(1);
    }
    
    /* Check/Allocate the frame */

    /* Check if the frame has been allocated */
    if(frame->allocated) {
      /* Data already allocated - check same size */
      if((frame->width != readraw.width) || (frame->height != readraw.height)) {
	/* This should never happen - checked already */
	printf("\n====== OUT OF CHEESE ERROR =========\n");
	exit(1);
      }
      
    }else {
      /* Allocate memory. Note organised in COLUMNS */
      frame->data = (float**) malloc(sizeof(float*)*readraw.width);
      for(i=0;i!=readraw.width;i++)
	frame->data[i] = (float*) malloc(sizeof(float)*readraw.height);
      frame->width = readraw.width;
      frame->height = readraw.height;
      frame->allocated =  1;
    }

    /* Change the data to be floating point between 0 and 1 */
    
    factor = (float) ((1 << readraw.bpp) - 1) ;
  
    if(readraw.channels != 1) {
      /* Input frame is in color */
      if(readraw.bpp > 8) {
	printf("\nSorry: Cannot read color images with bpp > 8 yet\n");
	exit(1);
      }else {
	/* Convert red channel */
	for(i=0;i<frame->width;i++) {
	  for(j=0;j<frame->height;j++) {
	    frame->data[i][j] = ((float) readraw.data[j][readraw.channels*i]) / factor;
	  }
	}
      }
    }else {
      /* Greyscale image */
      if(readraw.bpp > 16) {
	printf("\nSorry: Cannot read greyscale images > 16bpp yet\n");
	exit(1);
      }else if(readraw.bpp > 8) {
	/* Data has 2 bytes per pixel */
	for(i=0;i<frame->width;i++) {
	  for(j=0;j<frame->height;j++) {
	    frame->data[i][j] = ( 256.0*((float) readraw.data[j][2*i]) + 
				  ((float) readraw.data[j][2*i+1]) ) / factor;
	  }
	}
      }else {
	/* if less than 8 bit, data must be unpacked into one byte per pixel */
	for(i=0;i<frame->width;i++) {
	  for(j=0;j<frame->height;j++) {
	    frame->data[i][j] = ((float) readraw.data[j][i]) / factor;
	  }
	}
      }
    }
  }

  frame->number = number;
  
  return(0);
}
