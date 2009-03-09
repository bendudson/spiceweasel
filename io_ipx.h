/*********************************************************************
 * Header for IPX file read/writing
 *********************************************************************/

#ifndef __IO_IPX_H__
#define __IO_IPX_H__ 1

#include <stdio.h>
#include <sys/types.h>

#include "spiceweasel.h"

//typedef unsigned int uint;  /* 32-bit */
//typedef unsigned short int ushort; /* 16-bit */

typedef struct {
  char   id[8];
  uint   size;
  char   codec[8];
  char   date_time[20];
  uint   shot;
  float  trigger;
  char   lens[24];
  char   filter[24];
  char   view[64];
  uint   numFrames;
  char   camera[64];
  ushort width;
  ushort height;
  ushort depth;
  uint   orient;
  ushort taps;
  ushort color;
  ushort hBin;
  ushort left;
  ushort right;
  ushort vBin;
  ushort top;
  ushort bottom;
  ushort offset[2];
  float  gain[2];
  uint   preExp;
  uint   exposure;
  uint   strobe;
  float  board_temp;
  float  ccd_temp;
  uint padding; /* Not sure where extra 2 bytes goes... */
}IPX_header;

typedef struct {
  long offset;
  uint size;
  double time;
}IPX_frame;

/* Status data for ipx read/write */
typedef struct {
  FILE *fd;          /* Open file descriptor */
  IPX_header header; /* IPX file header */
  IPX_frame *frames; /* List of frames */
}IPX_status;

/********* PROTOTYPES ************/

int IPX_read_open(char *filename, IPX_status *status);
int IPX_read_frame(int fnr, TFrame *frame, IPX_status *status);
int IPX_read_close(IPX_status *status);

int IPX_write_open(char *filename, int precision, IPX_status *status);
int IPX_write_frame(TFrame *frame, IPX_status *status);
int IPX_write_close(IPX_status *status);

/************ GLOBAL VARIABLES **************/

#ifndef IPXGLOBALORIGIN
#define GLOBAL extern
#else
#define GLOBAL 
#endif

GLOBAL IPX_status ipx_read_status; /* IPX file handle */

#undef GLOBAL

#endif
