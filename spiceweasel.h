#ifndef __SPICEWEASEL_H__
#define __SPICEWEASEL_H__ 1

#include <stdio.h>

/* Define name of input file */
//#define DEFAULT_INPUT_NAME "../idl_code/14396/ma14396_comhalf000%04d.bmp"
//#define DEFAULT_INPUT_NAME "../lmode/frame%04d.bmp"
//#define DEFAULT_INPUT_NAME "/home/bdudson/mast_video/14396/ma14396_Comhalf-000%04d.png"
//#define DEFAULT_INPUT_NAME "../idl_code/15226_frame_%04d.png"
//#define DEFAULT_INPUT_NAME "../15368/15368_frame_%04d.png"
#define DEFAULT_INPUT_NAME "/net/fuslsa/data/MAST_IMAGES/rbb/rbb015368.ipx"

/* Define name of output file */
#define DEFAULT_OUTPUT_NAME "processed_%04d.png"

/* Name of file when only a shot number is specified */
#define SHOT_NAME "/net/fuslsa/data/MAST_IMAGES/rbb/rbb%06d.ipx"

#define OUTPUT_BYTEDEPTH 1

#define OUTPUT_COLOR 0

#define MAX_NAME_LEN 255

typedef struct {
  int number;
  double time;
  /* Data stored in columns since this is how the frames are sliced
   Data stored as floats. */
  int width, height;
  int allocated;  /* Indicates whether data has been allocated */
  
  float **data;

  int last; /* Flag indicating last frame */
}TFrame;

typedef struct {
  int bpp;
  int rowbytes;
  int channels; /* Number of channels - 1 for greyscale */
  int allocated;

  int width, height;

  unsigned char **data; /* Raw data. Stored in rows. */
}TRawFrame;

/* Color map definition */
typedef struct {
  int n;
  float *value; /* Value between 0 and 1. Sorted by value */
  float *red;
  float *green;
  float *blue;
}TColorMap;

typedef struct {
  int width; /* Width of filter in pixels */
  int height; /* Height of filter in pixels */
  int x, y; /* Centre of filter (where result goes) */
  int normalize; /* if 1, normalize result to weights applied */
  float **weight; /* Weights */
}FILTER;

#define FORMAT_UNKNOWN -1
#define FORMAT_BMP      0
#define FORMAT_PNG      1
#define FORMAT_IPX      2

/* File I/O error types */

#define IO_ERROR_OPEN     1    /* Couldn't open file */
#define IO_ERROR_FORMAT   2    /* Incorrect format */
#define IO_ERROR_SIZE     3    /* Input frame size has changed */
#define IO_ERROR_WRITE    4
#define IO_ERROR_OTHER    5    

#define PI 3.1415926535897932384626433832795028841971693993751058209

/************ GLOBAL VARIABLES **************/

#ifndef GLOBALORIGIN
#define GLOBAL extern
#else
#define GLOBAL 
#endif

/* Name of input and output files */
GLOBAL char input_template[MAX_NAME_LEN], output_template[MAX_NAME_LEN]; 
GLOBAL int input_format, output_format; /* File format for input and output */

GLOBAL TColorMap colormap; /* Color map for output */

#undef GLOBAL
/*************** PROTOTYPES *****************/

/* io_png.c */
int read_png(char *filename, TRawFrame *frame);
int write_png(char *filename, TRawFrame *frame);

/* io_bmp.c */
int read_bmp(char *filename, TRawFrame *frame);
int write_bmp(char *filename, TFrame *frame);

/* process_frames.c */

int allocate_output(int width, int height, TFrame *output);

int average_frames(TFrame **framebuffer, int nframes, TFrame *output, int width);
int minimum_frames(TFrame **framebuffer, int nframes, TFrame *output, int width);
int subtract_background(TFrame *orig, TFrame *background, TFrame *output);
int concatenate_frames(TFrame *output, int n, TFrame *first, ...);
int concat_frames(TFrame *output, int n, TFrame **list);
int copy_frame(TFrame *input, TFrame *output);
int normalize_frame(TFrame *input, TFrame *output);
int amplify_frame(TFrame *input, TFrame *output, float factor);
int gamma_correct_frame(TFrame *input, TFrame *output, float gamma);
int offset_frame(TFrame *input, TFrame *output, float midpoint);

int despeckle_median(TFrame *input, TFrame *output, int radius);
int kuwahara_filter(TFrame *input, TFrame *output, int L);
int denoise_pixel(TFrame *input, TFrame *output, float amount);
int gauss_blur(TFrame *input, TFrame *output, float sigma);

int sharpen_simple(TFrame *input, TFrame *output, float k);
int sharpen_unsharp(TFrame *input, TFrame *output, float sigma, float amount);

void shell_sort(unsigned long n, float *a);

float **float_array(int width, int height);
void free_array(float **arr);
void alloc_filter(FILTER *filter, int width, int height);
void free_filter(FILTER *filter);
int apply_filter(TFrame *input, FILTER *filter, TFrame *output);

/* read_main.c */
void read_init();
void read_finish();
int read_frame(int number, TFrame *frame);

/* process_main.c */
void process_init();
int process_frames(TFrame **framebuffer, int nframes, int centreframe, TFrame *output);

/* process_script.c */
int process_script(char *exe_cmd, char *file);

#endif /* __SPICEWEASEL_H__ */

