/*******************************************************************************
 * This is a replacement for process_main.c which uses a predefined
 * set of commands specified in an "SPS" file.
 * The script is read in "process_script.c" and run here.
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
 ***********************************************************************************/

#include <stdlib.h>

#include "spiceweasel.h"
#include "script.h"

TFrame *tmp_frame; /* Array of intermediate frames */
TFrame **frame_list; /* A list of frames for concatenation */

/* Initialize variables needed to run script */
void process_init()
{
  int i;
  int maxc;

  if(command.ntemp > 0) {
    /* Need intermediate frames */
    tmp_frame = (TFrame*) malloc(sizeof(TFrame)*command.ntemp);
    for(i=0;i<command.ntemp;i++) {
      tmp_frame[i].allocated = 0;
    }
  }

  /* Concatenate: Needs a separate array of vectors. Find largest concatenation */
  maxc = 0;
  for(i=0;i<command.nsteps;i++) {
    if(command.step[i].method == PROC_CONCATENATE) {
      if(command.step[i].nargs > maxc)
	maxc = command.step[i].nargs;
    }
  }
  if(maxc > 0) {
    frame_list = (TFrame**) malloc(sizeof(TFrame*) * maxc);
  }
}

TFrame *get_frame(int id, TFrame *input, TFrame *output)
{
  if(id == UNKNOWN_FRAME) {
    printf("Error in compiled script: Unknown frame\n");
    exit(1);
  }else if(id == OUTPUT_FRAME) {
    return(output);
  }else if(id == INPUT_FRAME) {
    return(input);
  }
  return(&(tmp_frame[id]));
}

#define GETFRAME(id) get_frame(id, framebuffer[centreframe], output)

/******************* RUN SCRIPT *****************/

int process_frames(TFrame **framebuffer, int nframes, int centreframe, TFrame *output)
{
  int i, j;
  TProcess *proc;
  TFrame *in, *out, *f;

  if(command.minimum_frame != UNKNOWN_FRAME) {
    /* Calculate the minimum background */
    minimum_frames(framebuffer, nframes, &(tmp_frame[command.minimum_frame]), -1);
  }
  if(command.average_frame != UNKNOWN_FRAME) {
    /* Calculate average background */
    average_frames(framebuffer, nframes, &(tmp_frame[command.average_frame]), -1);
  }
  /* Go through commands */
  for(i=0;i<command.nsteps;i++) {
    proc = &(command.step[i]);

    /* Get pointers to the input and outputs */
    if(proc->method != PROC_CONCATENATE) { /* concatenate has no input */
      in = GETFRAME(proc->input);
    }
    out = GETFRAME(proc->result);

    switch(proc->method) {
    case PROC_SUBTRACT: {
      /* Get pointer to the argument */
      f = GETFRAME(proc->args[0].frame);
      subtract_background(in, f, out);
      break;
    }
    case PROC_NORMALIZE: {
      normalize_frame(in, out);
      break;
    }
    case PROC_AMPLIFY: {
      amplify_frame(in, out, proc->args[0].fval);
      break;
    }
    case PROC_GAMMA: {
      gamma_correct_frame(in, out, proc->args[0].fval);
      break;
    }
    case PROC_OFFSET: {
      offset_frame(in, out, proc->args[0].fval);
      break;
    }
    case PROC_DESPECKLE_MEDIAN: {
      despeckle_median(in, out, proc->args[0].ival);
      break;
    }
    case PROC_KUWAHARA: {
      kuwahara_filter(in, out, proc->args[0].ival);
      break;
    }
    case PROC_SHARPEN: {
      sharpen_simple(in, out, proc->args[0].fval);
      break;
    }
    case PROC_UNSHARP_MASK: {
      sharpen_unsharp(in, out, proc->args[0].fval, proc->args[1].fval);
      break;
    }
    case PROC_CONCATENATE: {
      /* Build an array of frames */
      for(j=0;j<proc->nargs;j++) {
	frame_list[j] = GETFRAME(proc->args[j].frame);
      }
      concat_frames(out, proc->nargs, frame_list);
      break;
    }
    case PROC_COPY: {
      copy_frame(in, out);
      break;
    }
    case PROC_GAUSSBLUR: {
      gauss_blur(in, out, proc->args[0].fval);
      break;
    }
    default: {
      printf("Error in compiled script: Unknown function %d\n", proc->method);
      exit(1);
    }
    }
    
  }
  /* Set number of output frame */
  output->number = framebuffer[centreframe]->number;

  /* Set time of output frame */
  output->time = framebuffer[centreframe]->time;

  return(0);
}
