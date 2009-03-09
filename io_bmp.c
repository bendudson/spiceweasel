/*****************************************
 * Reads in a greyscale bitmap from file
 * NOTE: BMP stores image upside-down relative to other formats
 ****************************************/

#include <stdio.h>
#include <stdlib.h>

#include "io_bmp.h"
#include "spiceweasel.h"

int read_bmp(char *filename, TRawFrame *frame)
{
  int i, j, padding, n;
  BITMAPFILEHEADER        bmfh;
  BITMAPINFOHEADER        bmih;
  FILE *bitmap_file;
  
  if((bitmap_file=fopen(filename,"rb"))==NULL) {
    return(IO_ERROR_OPEN);
  } 

  /* Read the bitmap file header */
  if(fread(&bmfh,14,1,bitmap_file)!=1) {
    fclose(bitmap_file);
    return(IO_ERROR_FORMAT);
  }

  /* Read the bitmap information header */
  if(fread(&bmih,40,1,bitmap_file)!=1) {
    fclose(bitmap_file);
    return(IO_ERROR_FORMAT);
  }

#ifdef DEBUG
  /* Print out details of bitmap */
  printf("\nType   : %c%c",bmfh.bfType,bmfh.bfType>>8);
  printf("\nSize   : %d",(int) bmfh.bfSize);
  printf("\nRes1   : %d",(int) bmfh.bfReserved1);
  printf("\nRes2   : %d",(int) bmfh.bfReserved2);
  printf("\nOffBits: %d\n",(int) bmfh.bfOffBits);
  printf("\nSize       : %d",(int) bmih.biSize);
  printf("\nWidth      : %d",(int) bmih.biWidth);
  printf("\nHeight     : %d",(int) bmih.biHeight);
  printf("\nPlanes     : %d",(int) bmih.biPlanes);
  printf("\nBitCount   : %d",(int) bmih.biBitCount);
  printf("\nCompression: %d",(int) bmih.biCompression);
  printf("\nSizeImage  : %d",(int) bmih.biSizeImage);
  printf("\nXPels      : %d",(int) bmih.biXPelsPerMeter);
  printf("\nYPels      : %d",(int) bmih.biYPelsPerMeter);
  printf("\nClrUsed    : %d",(int) bmih.biClrUsed);
  printf("\nClrImpor   : %d",(int) bmih.biClrImportant);
  printf("\n");
#endif

  if(bmih.biCompression!=0) {
    fclose(bitmap_file);
    return(IO_ERROR_FORMAT);
  }

  if((bmih.biBitCount % 8) != 0) { /* Data must be in bytes */
    fclose(bitmap_file);
    return(IO_ERROR_FORMAT);
  }

  frame->bpp = bmih.biBitCount;
  
  n = frame->bpp / 8; /* Bytes per pixel */

  if((frame->bpp % 8) != 0) {
    return(IO_ERROR_FORMAT);
  }

  /* Check if the frame has been allocated */
  if(frame->allocated) {
    /* Data already allocated - check same size */
    if((frame->width != bmih.biWidth) || (frame->height != bmih.biHeight)) {
      return(IO_ERROR_SIZE);
    }
    
  }else {
    /* Allocate memory */
    frame->data = (unsigned char**) malloc(sizeof(char*)*bmih.biHeight);
    for(i=0;i!=bmih.biHeight;i++)
      frame->data[i] = (char*) malloc(n*sizeof(char)*bmih.biWidth);
    frame->width = bmih.biWidth;
    frame->height = bmih.biHeight;
    frame->allocated =  1;
  }

  /* Read n bytes per pixel */
  for(j=0;j!=frame->height;j++) {
    for(i=0;i!=frame->width;i++) {
      if(fread(&(frame->data[frame->height-1-j][n*i]), 
	       1, n, bitmap_file) != n) {
	/* Premature end of file */
	fclose(bitmap_file);
	return(IO_ERROR_FORMAT);
      }
    }
    /* Scan lines always end on 32-bit boundaries */
    if((frame->width * n) % 4 != 0) {
      /* Needs extra padding */
      fread(&padding, 4 - ((frame->width * n) % 4), 1, bitmap_file);
    }
  }

  frame->channels = bmih.biPlanes;

  fclose(bitmap_file);
  return(0);
}

int write_bmp(char *filename, TFrame *frame)
{
  int i, j, k, nbyte, padding;
  BITMAPFILEHEADER        bmfh;
  BITMAPINFOHEADER        bmih;
  FILE *bitmap_file;
  unsigned char pixel[3];
  
  padding = 0;

  if((bitmap_file=fopen(filename,"wb"))==NULL) {
    return(IO_ERROR_OPEN);
  }

  /* Set the bitmap file header */
  bmfh.bfType = 'M';
  bmfh.bfType = (bmfh.bfType << 8) + 'B';
  
  bmfh.bfSize = 0;
  bmfh.bfReserved1 = 0;
  bmfh.bfReserved2 = 54;
  bmfh.bfOffBits = 0;
  

  /* Set the information header */
  bmih.biSize = 40;
  bmih.biWidth = frame->width;
  bmih.biHeight = frame->height;
  bmih.biPlanes = 1;
  bmih.biBitCount = 24;
  bmih.biCompression = 0;
  bmih.biSizeImage = frame->width * frame->height * 3;
  bmih.biXPelsPerMeter = 0;
  bmih.biYPelsPerMeter = 0;
  bmih.biClrUsed = 0;
  bmih.biClrImportant = 0;
  
  /* Write the headers */
  if(fwrite(&bmfh, 14, 1, bitmap_file) != 1) {
    fclose(bitmap_file);
    return(IO_ERROR_WRITE);
  }

  if(fwrite(&bmih, 40, 1, bitmap_file) != 1) {
    fclose(bitmap_file);
    return(IO_ERROR_WRITE);
  }

  nbyte = 3;
  /* Write the data */

  for(j=0;j!=frame->height;j++) {
    for(i=0;i!=frame->width;i++) {
      if(frame->data[i][j] > 255.5)
	frame->data[i][j] = 255.5;
      pixel[0] = (unsigned char) frame->data[i][frame->height-1-j];
      for(k=1;k<nbyte;k++)
	pixel[k] = pixel[0];
      
      if(fwrite(pixel, 1, nbyte, bitmap_file) != nbyte) {
	fclose(bitmap_file);
	return(IO_ERROR_WRITE);
      }
    }
    /* Scan lines always end on 32-bit boundaries */
    if((frame->width * nbyte) % 4 != 0) {
      /* Needs extra padding */
      fwrite(&padding, 4 - ((frame->width * nbyte) % 4), 1, bitmap_file);
    }
  }
  fclose(bitmap_file);
  fflush(stdout);

  return(0);
}
