/************************************************************************
 * Multi-threaded code to process video frames, producing a composite
 * output frame. Use three threads:
 * - Input thread: reads in frames for processing
 * - Processing thread: Computes background frame(s), processes frame
 *                      and produces output frame data
 * - Output thread: Writes out frame to file
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
 ************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GLOBALORIGIN
#include "spiceweasel.h"

#include "io_ipx.h"

/************** GLOBAL DATA ***********/

/* thread syncronization */
pthread_mutex_t input_ready_mutex, process_ready_mutex, output_ready_mutex;
pthread_cond_t input_ready_cond, process_ready_cond, output_ready_cond;
int input_ready, process_ready, output_ready;

/* Input data */
TFrame *input_frame[2];
int frame_read;  /* Frame number last read */

/* Output data */
TFrame *output_frame[2];
int frame_written; /* Frame number last written */

int startframe, endframe; /* Frame numbers to process */


/************** ACTUAL PROCESSING ROUTINES ***************/

int check_format(char *template) /* Check file extension of template */
{
  int n;
  int format;

  n = strlen(template);
  if(n < 4) {
    return(FORMAT_UNKNOWN);
  }
  if(strncasecmp(&(template[n-4]), ".bmp", 4) == 0) {
    /* bitmap format */
    format = FORMAT_BMP;
  }else if(strncasecmp(&(template[n-4]), ".png", 4) == 0) {
    format = FORMAT_PNG;
  }else if(strncasecmp(&(template[n-4]), ".ipx", 4) == 0) {
    format = FORMAT_IPX;
  }else {
    format = FORMAT_UNKNOWN;
  }

  return(format);
}

void read_colormap()
{
    
  /*
  // Simple temperature scale
  colormap.n = 5;
  colormap.value = (float*) malloc(sizeof(float)*5);
  colormap.red   = (float*) malloc(sizeof(float)*5);
  colormap.green = (float*) malloc(sizeof(float)*5);
  colormap.blue  = (float*) malloc(sizeof(float)*5);

  colormap.value[0] = 0.00; colormap.red[0] =   0.0; colormap.green[0] =   0.0; colormap.blue[0] =   0.0;
  colormap.value[1] = 0.25; colormap.red[1] = 255.0; colormap.green[1] =   0.0; colormap.blue[1] =   0.0;
  colormap.value[2] = 0.50; colormap.red[2] =   0.0; colormap.green[2] = 255.0; colormap.blue[2] =   0.0;
  colormap.value[3] = 0.75; colormap.red[3] =   0.0; colormap.green[3] =   0.0; colormap.blue[3] = 255.0;
  colormap.value[4] = 1.00; colormap.red[4] = 255.0; colormap.green[4] = 255.0; colormap.blue[4] = 255.0;
  
  */
  
  // Yellowy temperature scale

  colormap.n = 4;
  colormap.value = (float*) malloc(sizeof(float)*4);
  colormap.red   = (float*) malloc(sizeof(float)*4);
  colormap.green = (float*) malloc(sizeof(float)*4);
  colormap.blue  = (float*) malloc(sizeof(float)*4);
  
  colormap.value[0] = 0.00; colormap.red[0] =   0.0; colormap.green[0] =   0.0; colormap.blue[0] =   0.0;
  colormap.value[1] = 0.33; colormap.red[1] = 255.0; colormap.green[1] =   0.0; colormap.blue[1] =   0.0;
  colormap.value[2] = 0.67; colormap.red[2] = 255.0; colormap.green[2] = 255.0; colormap.blue[2] =   0.0;
  colormap.value[3] = 1.00; colormap.red[3] = 255.0; colormap.green[3] = 255.0; colormap.blue[3] = 255.0;
}

TRawFrame writeraw;
IPX_status ipx_write_status;

void write_init()
{
  int depth;
  writeraw.allocated = 0;

  if(output_format == FORMAT_IPX) {
    
    depth = 16;
    /* Setup output header */
    if(input_format == FORMAT_IPX) {
      depth = ipx_read_status.header.depth;
      
      /* Copy header from input */
      memcpy(&(ipx_write_status.header), &(ipx_read_status.header), sizeof(IPX_header));
    }else {
      /* Clear header */
      memset(&(ipx_write_status.header), 0, sizeof(IPX_header));
    }

    /* Open output file */
    printf("Opening IPX output file %s...", output_template);
    if(IPX_write_open(output_template, depth, &ipx_write_status)) {
      printf("failed!\n");
      exit(1);
    }
    printf("done\n");
    
  }

  read_colormap();
}

void write_finish()
{
  if(output_format == FORMAT_IPX) {
    IPX_write_close(&ipx_write_status);
  }
}

int write_frame(TFrame *frame)
{
  char filename[MAX_NAME_LEN];
  int errcode;
  int rowbytes;
  int i, j, k, p, n;
  float val, v1, v2, vd;

  if(output_format == FORMAT_IPX) {
    /* Write frames to an IPX file */
    if(IPX_write_frame(frame, &ipx_write_status)) {
      printf("Error writing frame\n");
      exit(1);
    }
  }else {
    /* Write a series of files, one per frame */
    //printf("***Writing output frame %d\n", frame->number);
    sprintf(filename, output_template, frame->number);

    rowbytes = frame->width * OUTPUT_BYTEDEPTH;
    if(OUTPUT_COLOR) 
      rowbytes *= 3; /* 3 channel */

    if(writeraw.allocated) {
      /* Data already allocated - check same size */
      if((writeraw.width != frame->width) || 
	 (writeraw.height != frame->height) ||
	 (writeraw.rowbytes != rowbytes)) {
	return(IO_ERROR_SIZE);
      }  
    }else {
      /* Allocate memory */
      
      writeraw.bpp = 8*OUTPUT_BYTEDEPTH;
      writeraw.channels = 1;
      if(OUTPUT_COLOR)
	writeraw.channels = 3;
      writeraw.rowbytes = rowbytes;
      
      writeraw.data = (unsigned char**) malloc(sizeof(unsigned char*)*frame->height);
      for(i=0;i!=frame->height;i++)
	writeraw.data[i] = (unsigned char*) malloc(rowbytes*sizeof(unsigned char));
      writeraw.width = frame->width;
      writeraw.height = frame->height;
      writeraw.allocated =  1;
    }

    /* Convert frame to output format */
    if(OUTPUT_COLOR) {
      /* Output 3-channel color */
      n = colormap.n - 1;
      for(i=0;i<frame->width;i++) {
	for(j=0;j<frame->height;j++) {
	  val = frame->data[i][j];
	  if(val <= colormap.value[0]) {
	    writeraw.data[j][3*i] = colormap.red[0];
	    writeraw.data[j][3*i + 1] = colormap.green[0];
	    writeraw.data[j][3*i + 2] = colormap.blue[0];
	  }else if(val >= colormap.value[n]) {
	    writeraw.data[j][3*i] = colormap.red[n];
	    writeraw.data[j][3*i + 1] = colormap.green[n];
	    writeraw.data[j][3*i + 2] = colormap.blue[n];
	  }else {
	    /* Interpolate */
	    p = 1;
	    for(k=1;k<=n;k++) {
	      if(colormap.value[k] >= val) {
		p = k;
		k = n+1; /* break loop */
	      }
	    }
	    /* p is now the first index greater than current value */
	    vd = colormap.value[p] - colormap.value[p-1];
	    v1 = (val - colormap.value[p-1]) / vd;
	    v2 = (colormap.value[p] - val) / vd;
	    
	    //printf("%d, %d -> %d, %f, %f\n", j, i, p, v1, v2);
	    
	    /* red channel */
	    writeraw.data[j][3*i] = (unsigned char) (0.5 + colormap.red[p]*v1 + colormap.red[p-1]*v2);
	    /* green channel */
	    writeraw.data[j][3*i+1] = (unsigned char) (0.5 + colormap.green[p]*v1 + colormap.green[p-1]*v2);
	    /* blue channel */
	    writeraw.data[j][3*i+2] = (unsigned char) (0.5 + colormap.blue[p]*v1 + colormap.blue[p-1]*v2);
	  }
	}
      }
    }else {
      /* Greyscale */
      if(writeraw.bpp > 16) {
	printf("\nSorry: Cannot write greyscale images > 16bpp\n");
	exit(1);
      }else if(writeraw.bpp > 8) {
	/* Put into 2 bytes per pixel */
	printf("\nSorry: Cannor write greyscale images > 8bpp yet\n");
	exit(1);
      }else {
	/* One byte per pixel */
	for(i=0;i<frame->width;i++) {
	  for(j=0;j<frame->height;j++) {
	    if(frame->data[i][j] > 1.0)
	      frame->data[i][j] = 1.0;
	    if(frame->data[i][j] < 0.0)
	      frame->data[i][j] = 0.0;
	    writeraw.data[j][i] = (unsigned char) (0.5 + frame->data[i][j] * 255.0);
	  }
	}
      }
    }
    
    /* Write the data in a format */
    
    errcode = 0;
    switch(output_format) {
    case FORMAT_BMP: {
      //errcode =  write_bmp(filename, frame);
      break;
    }
    case FORMAT_PNG: {
      errcode =  write_png(filename, &writeraw);
      break;
    }
    default: {
      errcode = IO_ERROR_FORMAT;
    }
    }
    
    if(errcode) {
      switch(errcode) {
      case IO_ERROR_OPEN: {
	printf("Error: Could not open output file %s\n", filename);
	break;
      }
      case IO_ERROR_FORMAT: {
	printf("Error: Output file %s has an invalid file format\n", filename);
	break;
      }
      case IO_ERROR_WRITE: {
	printf("Error: Write error on file %s\n", filename);
	break;
      }
      case IO_ERROR_OTHER: {
      printf("Error: Could not write output file %s\n", filename);
      break;
      }
      }
      exit(1);
    }
  }

  return(0);
}

/************* MULTI-THREADED SYNCRONIZATION CODE ***********/

/* This routine reads in a set of inputs, keeps reading when needed */
void* input_routine(void *args)
{
  int frame;
  int cycle;
  int finished;

  cycle = 1; /* Opposite to processing thread */
  frame = startframe; /* Frame number to read */
  finished = 0;

  do {
    /* Tell main thread input is ready */
    pthread_mutex_lock(&input_ready_mutex);
    input_ready = cycle;
    pthread_cond_signal(&input_ready_cond);
    pthread_mutex_unlock(&input_ready_mutex);

    /* Wait for main thread */
    pthread_mutex_lock(&process_ready_mutex);
    while(process_ready == cycle) {
      pthread_cond_wait(&process_ready_cond, &process_ready_mutex);
    }
    pthread_mutex_unlock(&process_ready_mutex);

    /* Read in data to input_frame[cycle] */
    read_frame(frame, input_frame[cycle]);

    frame_read = frame;
    
    /* Read routine may already have set this as last frame.
       If this is the last requested frame, mark and finish */
    //input_frame[cycle]->last = 0;
    if(frame == endframe)
      input_frame[cycle]->last = 1;
    if(input_frame[cycle]->last == 1)
      finished = 1;

    cycle ^= 1; /* Flip between frames */
    frame++;
  }while(!finished);
  //printf("Input terminating\n");
  
  /* Tell main thread input is ready */
  pthread_mutex_lock(&input_ready_mutex);
  input_ready = cycle;
  pthread_cond_signal(&input_ready_cond);
  pthread_mutex_unlock(&input_ready_mutex);

  fflush(stdout);
  pthread_exit(NULL);
}

/* Outputs finished frames */
void* output_routine(void *args)
{
  int finished;
  int cycle;

  cycle = 1; /* Opposite to processing thread */
  finished = 0;

  do {
    /* Tell main thread output ready */
    pthread_mutex_lock(&output_ready_mutex);
    output_ready = cycle;
    pthread_cond_signal(&output_ready_cond);
    pthread_mutex_unlock(&output_ready_mutex);

    /* Wait for main thread */
    pthread_mutex_lock(&process_ready_mutex);
    while(process_ready == cycle) {
      pthread_cond_wait(&process_ready_cond, &process_ready_mutex);
    }
    pthread_mutex_unlock(&process_ready_mutex);

    if(output_frame[cycle]->allocated) { /* If valid data present */
      /* Output data in output_frame[cycle] */
      write_frame(output_frame[cycle]);

      /* Check if this is the last frame */
      if(output_frame[cycle]->last) {
	//printf("Output reached last frame\n");
	finished = 1;  
      }

      frame_written = output_frame[cycle]->number; 
    }

    cycle ^= 1; /* Flip between frames */
  }while(!finished);

  pthread_exit(NULL);
}

int main(int argc, char **argv)
{
  /* Thread handles */
  pthread_t input_thread, output_thread;
  void *retval;

  int nframes; /* Size of framebuffer */
  int framereplace, centreframe;
  TFrame **framebuffer; /* Buffer of frames */
  TFrame *tmpframe;
  
  int cycle;  /* Keeps track of which buffer to use */
  int finished, status;
  int i, n;

  char *script;

  /* Status information */
  int last_read, last_written;
  float progress, total;
  time_t start_time, end_time;

  /* Start timing */
  start_time = time(NULL);
  
  strncpy(input_template, DEFAULT_INPUT_NAME, MAX_NAME_LEN-1);
  input_template[MAX_NAME_LEN-1] = 0;
  strncpy(output_template, DEFAULT_OUTPUT_NAME, MAX_NAME_LEN-1);
  output_template[MAX_NAME_LEN-1] = 0;
  
  /******** CHECK COMMAND-LINE ARGUMENTS ********/

  if(argc < 4) {
    printf("Useage: %s <start frame> <final frame> <buffer size> [options]\n", argv[0]);
    printf("  Options:\n");
    printf("    -s <shot number>     Set input shot number\n");
    printf("    -i <input template>  Set input file template\n");
    printf("    -o <output template> Set output file template\n");
    printf("    -p <SPS file>        Set processing script\n");
    printf("  See README.txt for more details\n\n");
    return(1);
  }
  if(sscanf(argv[1], "%d", &startframe) != 1) {
    printf("Useage: First argument must be starting frame number\n");
    return(1);
  }
  if(sscanf(argv[2], "%d", &endframe) != 1) {
    printf("Useage: Second argument must be final frame number\n");
    return(1);
  }
  if(sscanf(argv[3], "%d", &nframes) != 1) {
    printf("Useage: Third argument must be size of the frame buffer\n");
    return(1);
  }

  if((nframes & 1) == 0) {
    nframes++;
    printf("---Frame buffer size must be odd: changing to %d\n", nframes);
  }

  if(startframe + nframes > (endframe+1)) {
    printf("***Not enough frames to fill buffer\n");
    return(1);
  }

  /* Go through options */

  script = (char*) NULL;

  for(i=4; i<argc;i++) {
    if(strncasecmp(argv[i], "-i", 2) == 0) {
      /* Set input name */
      i++;
      if(i == argc) {
	printf("Option useage is -i <input template>\n");
	return(1);
      }
      strncpy(input_template, argv[i], MAX_NAME_LEN-1);
      input_template[MAX_NAME_LEN-1] = 0;
      
    }else if(strncasecmp(argv[i], "-s", 2) == 0) {
      /* Set a shot number */
      i++;
      if(i == argc) {
	printf("Option useage is -s <shot number>\n");
	return(1);
      }
      if(sscanf(argv[i], "%d", &n) != 1) {
	printf("Shot number (-s option) must be an integer\n");
	return(1);
      }
      sprintf(input_template, SHOT_NAME, n);
    }else if(strncasecmp(argv[i], "-o", 2) == 0) {
      /* Set output name */
      i++;
      if(i == argc) {
	printf("Option useage is -o <output template>\n");
	return(1);
      }
      strncpy(output_template, argv[i], MAX_NAME_LEN-1);
      output_template[MAX_NAME_LEN-1] = 0;
    }else if(strncasecmp(argv[i], "-p", 2) == 0) {
      /* Set processing script */
      i++;
      if(i == argc) {
	printf("Option useage is -p <processing script>\n");
	return(1);
      }
      script = argv[i];
    }
  }

  /******** CHECK EXTENSION OF INPUT AND OUTPUT FILES ********/

  if((input_format = check_format(input_template)) == FORMAT_UNKNOWN) {
    printf("Error: Unrecognised input format\n");
    return(1);
  }
  if((output_format = check_format(output_template)) == FORMAT_UNKNOWN) {
    printf("Error: Unrecognised output format\n");
    return(1);
  }

  /******** PRINT INTRO PUFF *********/

  printf("\n   Spice-Weasel photron video enhancement\n");
  printf("            Version %s\n", VERSION);
  printf("Knocking MAST videos up a notch since april 2006\n");
  printf("      Developed by B.Dudson and A.Meakins\n\n");

  
  /************ READ PROCESSING SCRIPT ************/
  
  if(process_script(argv[0], script)) {
    /* Some error compiling script */
    return(1);
  }

  /******** INITIALIZE FRAME BUFFER **********/

  /* Initialize processing variables */
  read_init(); /* Note: read MUST init before write */
  process_init();
  write_init();

  printf("Initializing frame buffer...");
  fflush(stdout);
  framebuffer = (TFrame**) malloc(sizeof(TFrame*)*nframes);
  
  for(i=0;i!=nframes;i++) {
    framebuffer[i] = (TFrame*) malloc(sizeof(TFrame));
    framebuffer[i]->allocated = 0;
    framebuffer[i]->last = 0;
    if(read_frame(startframe, framebuffer[i])) {
      printf("\n***Error reading input frame %d\n", startframe);
      exit(1);
    }
    startframe++;
  }
  centreframe = (nframes-1)/2; /* The frame in the middle of the buffer */
  framereplace = 0;

  input_frame[0]  = (TFrame*) malloc(sizeof(TFrame));   input_frame[0]->allocated  = 0;
  input_frame[1]  = (TFrame*) malloc(sizeof(TFrame));   input_frame[1]->allocated  = 0;
  output_frame[0] = (TFrame*) malloc(sizeof(TFrame));   output_frame[0]->allocated = 0;
  output_frame[1] = (TFrame*) malloc(sizeof(TFrame));   output_frame[1]->allocated = 0;

  input_frame[0]->last = 0;
  input_frame[1]->last = 0;
  output_frame[0]->last = 0;
  output_frame[1]->last = 0;

  frame_read = startframe-1;
  frame_written = 0;

  total = (float) (endframe - startframe + 2);
  progress = 0.0;
 
  printf("done\n");

  /******** INITIALIZE SYNC VARIABLES **********/

  pthread_mutex_init(&input_ready_mutex, NULL);
  pthread_mutex_init(&output_ready_mutex, NULL);
  pthread_mutex_init(&process_ready_mutex, NULL);
  pthread_cond_init(&input_ready_cond, NULL);
  pthread_cond_init(&output_ready_cond, NULL);
  pthread_cond_init(&process_ready_cond, NULL);

  input_ready = 0;
  output_ready = 0;
  process_ready = 1;
  
  cycle = 0;
  finished = 0;
  status = -1;

#ifndef SINGLE_THREAD
  /********* CREATE I/O THREADS *********/
  if(startframe <= endframe) { /* Check there are more frames to read */

    printf("Starting input thread...");
    fflush(stdout);
    if(pthread_create(&input_thread, NULL, input_routine, NULL)) {
      printf("Failed\n");
      return(1);
    }
    
    printf("done\n");
    
  }else {
    /* Exit after one frame */
    output_frame[cycle]->last = 1;
    finished = 1;
  }
  
  printf("Starting output thread...");
  fflush(stdout);
  if(pthread_create(&output_thread, NULL, output_routine, NULL)) {
    printf("Failed\n");
    return(1);
  }    
  printf("done\n");
#else
  printf("Single-threaded version\n");
#endif // SINGLE_THREAD

  /*********** START PROCESSING LOOP *********/

  printf("========= PROCESSING FRAMES ===========\n");

  do {

    /* Check input and output processes are ready */
#ifndef SINGLE_THREAD
    if(finished != 1) {  /* If 1, input thread doesn't exist */
      pthread_mutex_lock(&input_ready_mutex);
      while(input_ready == cycle) {
	pthread_cond_wait(&input_ready_cond, &input_ready_mutex);
      }
      input_ready = cycle ^ 1;
      pthread_mutex_unlock(&input_ready_mutex);
    }
    pthread_mutex_lock(&output_ready_mutex);
    while(output_ready == cycle) {
      pthread_cond_wait(&output_ready_cond, &output_ready_mutex);
    }
    output_ready = cycle ^ 1;
    pthread_mutex_unlock(&output_ready_mutex);
#endif
    
    last_read = frame_read;
    last_written = frame_written;

#ifndef SINGLE_THREAD
    pthread_mutex_lock(&process_ready_mutex);
    process_ready = cycle;
    pthread_cond_broadcast(&process_ready_cond);
    pthread_mutex_unlock(&process_ready_mutex);
#endif
    
    printf("\r Input %5d Output %5d Progress %2.f%%", 
	   last_read, last_written, progress / total);
    fflush(stdout);
    progress += 100.0;

    if(status == -1) {
      /* This just skips reading first time around - all data in buffer */
      status = 0;
    }else {

#ifdef SINGLE_THREAD
      /* Need to read in the next frame */
      frame_read++;
      read_frame(frame_read, input_frame[cycle]);
      if(frame_read == endframe)
	input_frame[cycle]->last = 1;
#endif

      /* Check if this is the last frame */
      if(input_frame[cycle]->last == 1) {
	//printf("Processing reached last frame: %d\n", input_frame[cycle]->number);
	output_frame[cycle]->last = 1;
	finished = 1;
      }

      /********** SWAP FRAME BUFFERS ********/
      tmpframe = framebuffer[framereplace];
      framebuffer[framereplace] = input_frame[cycle];
      input_frame[cycle] = tmpframe;
    
      /* Circular buffer - update indices to the frame to be replaced next
	 and the frame to be processed */
      framereplace++;
      centreframe++;
      if(framereplace == nframes)
	framereplace = 0;
      if(centreframe == nframes)
	centreframe = 0;
    }

    /************* PROCESS DATA ****************
     * output frame into output_frame[cycle]   */

    process_frames(framebuffer, nframes, centreframe, output_frame[cycle]);

    /******************************************/

#ifdef SINGLE_THREAD
    /* Write out frame */
    write_frame(output_frame[cycle]);
    frame_written = output_frame[cycle]->number; 
#else
    /* Mult-threaded */
    cycle ^= 1; /* Flip between 0 and 1 */
#endif
    
  }while(finished != 1);

  /********** WAIT FOR OUTPUT TO FINISH ***********/

#ifndef SINGLE_THREAD
  //printf("Processing finished, waiting for output to finish\n");
  pthread_mutex_lock(&output_ready_mutex);
  while(output_ready == cycle) {
    pthread_cond_wait(&output_ready_cond, &output_ready_mutex);
  }
  pthread_mutex_unlock(&output_ready_mutex);

  pthread_mutex_lock(&process_ready_mutex);
  process_ready = cycle;
  pthread_cond_broadcast(&process_ready_cond);
  pthread_mutex_unlock(&process_ready_mutex);

  /* Wait for i/o to finish */
  pthread_join(output_thread, &retval);
#endif

  /* Clean up */
  read_finish();
  write_finish();

  end_time = time(NULL);

  printf("\nGot %d blasts from the spice-weasel in %d seconds. Bam!!!\n",
	 endframe - startframe + 2, (int) (end_time - start_time));
  return(0);
}


