/***********************************************************************************
 * Main routine dealing with processing frames
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
 ***************************************************************************************/

#include "spiceweasel.h"

/* temporary data */

TFrame orig_frame;
TFrame gamma_frame;
TFrame average_frame;
TFrame diffed_frame;
TFrame despeckle_frame;
TFrame sharp_frame;

void process_init()
{
  orig_frame.allocated = 0;
  gamma_frame.allocated = 0;
  average_frame.allocated = 0;
  diffed_frame.allocated = 0;
  despeckle_frame.allocated = 0;
  sharp_frame.allocated = 0;
}

int process_frames(TFrame **framebuffer, int nframes, int centreframe, TFrame *output)
{

  /************ BACKGROUND SUBTRACTION ***************/

  /* Compute minimum frame (only of 256 left columns) */
  minimum_frames(framebuffer, nframes, &average_frame, 10000000);

  //despeckle_median(&average_frame, &diffed_frame, 1);

  /* Subtract from centre frame */
  subtract_background(framebuffer[centreframe], &average_frame, &diffed_frame);

  /* Normalize/Amplify the frame */
  //normalize_frame(&diffed_frame, &diffed_frame);
  amplify_frame(&diffed_frame, &diffed_frame, 4.0);

  /* Gamma correct differenced frame (enhance low intensity) */
  gamma_correct_frame(&diffed_frame, &diffed_frame, 2.0);
  
  /* Despeckle the differenced frame */
  //despeckle_median(&diffed_frame, &despeckle_frame, 1);
  
  gauss_blur(&diffed_frame, &despeckle_frame, 3.0);
  
  /* Sharpen despeckled frame */
  //sharpen_simple(&despeckle_frame, &sharp_frame, 0.4);
  //sharpen_unsharp(&despeckle_frame, &sharp_frame, 4.0, 1.0);

  /************** ENHANCE ORIGINAL *********************/

  /* Amplify original frame - keep for output */
  amplify_frame(framebuffer[centreframe], &orig_frame, 2.0);

  /* Gamma enhance */
  gamma_correct_frame(&orig_frame, &gamma_frame, 1.5);

  /* Despeckle */
  //despeckle_median(&gamma_frame, &despeckle_frame, 1);

  //kuwahara_filter(&orig_frame, &despeckle_frame, 2);

  /* Sharpen */
  //sharpen_unsharp(&despeckle_frame, &gamma_frame, 4.0, 1.2);
  

  /************** CONCATENATE FRAMES *******************/

  concatenate_frames(output, 3, &gamma_frame,
		     &diffed_frame, &despeckle_frame);

  output->number = framebuffer[centreframe]->number;

  return(0);
}
