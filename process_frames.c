/**********************************************************************************
 * Routines for processing frame structures
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
 **********************************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "spiceweasel.h"

int allocate_output(int width, int height, TFrame *output)
{
  int errcode;
  int i;

  errcode = 0;
  if(output->allocated != 1) {
    /* Allocate memory */
    output->width = width;
    output->height = height;
    output->data = (float**) malloc(sizeof(float*)*width);
    for(i=0;i<width;i++)
      output->data[i] = (float*) malloc(sizeof(float)*height);

    output->allocated = 1;
  }else {
    /* Check size is correct */
    if(output->width != width)
      errcode = 1;
    if(output->height != height)
      errcode = 2;
  }
  return(errcode);
}

/* Background frame calculation - only processes first width columns */

int average_frames(TFrame **framebuffer, int nframes, TFrame *output, int width)
{
  int height;
  int i, f, j;
  float nf, *ptr;

  nf = (float) nframes;

  height = framebuffer[0]->height;

  if((width > framebuffer[0]->width) || (width < 0))
    width = framebuffer[0]->width;
  
  if(allocate_output(width, height, output)) {
    return(1);
  }
 
  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = framebuffer[0]->data[i][j];
    }
    for(f=1;f<nframes;f++) {
      ptr = framebuffer[f]->data[i];
      for(j=0;j<height;j++) {
	output->data[i][j] += *ptr;
	ptr++;
      }
    }
    for(j=0;j<height;j++) {
      output->data[i][j] /= nf;
    }
  }
  return(0);
}

int minimum_frames(TFrame **framebuffer, int nframes, TFrame *output, int width)
{
  int height;
  int i, f, j;
  float nf, *ptr;

  nf = (float) nframes;

  height = framebuffer[0]->height;

  if((width > framebuffer[0]->width) || (width < 0))
    width = framebuffer[0]->width;

  if(allocate_output(width, height, output)) {
    return(1);
  }
  
  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = framebuffer[0]->data[i][j];
    }
    for(f=1;f<nframes;f++) {
      ptr = framebuffer[f]->data[i];
      for(j=0;j<height;j++) {
	if(*ptr < output->data[i][j])
	  output->data[i][j] = *ptr;
	ptr++;
      }
    }
  }
  return(0);
}

float *median_buffer;
int median_allocated = 0;

int median_frames(TFrame **framebuffer, int nframes, TFrame *output, int width)
{
  if(median_allocated != 1) {
    
  }

  return(0);
}

int subtract_background(TFrame *orig, TFrame *background, TFrame *output)
{
  int width, height;
  int i, j;

  width = orig->width;
  height = orig->height;
  
  if(background->width < width)
    width = background->width;

  if(background->height < height)
    height = background->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = orig->data[i][j] - background->data[i][j];
    }
  }
  return(0);
}

/* Places frames next to each other from left to right*/
int concatenate_frames(TFrame *output, int n, TFrame *first, ...)
{
  va_list ap; /* List of arguments */
  int width, height;
  int a, i, j;
  TFrame *frame;
  int pos;

  if(n < 1)
    return(1);

  width = first->width;
  height = first->height;

  /* Get the size of the output frame */
  va_start(ap, first);
  for(i=1;i<n;i++) {
    frame = (TFrame*) va_arg(ap, TFrame*);
    width += frame->width;
    if(frame->height < height)
      height = frame->height;
  }
  va_end(ap);

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i<first->width;i++) {
    for(j=0;j<height;j++)
      output->data[i][j] = first->data[i][j];
  }
  pos = first->width;
  
  va_start(ap, first);
  for(a=1;a<n;a++) {
    frame = (TFrame*) va_arg(ap, TFrame*);
    for(i=0;i<frame->width;i++) {
      for(j=0;j<height;j++)
	output->data[i+pos][j] = frame->data[i][j];
    }
    pos += frame->width;
    
  }
  va_end(ap);
  return(0);
}

/* Places frames next to each other from left to right*/
int concat_frames(TFrame *output, int n, TFrame **list)
{
  int width, height;
  int a, i, j;
  int pos;

  if(n < 1)
    return(1);

  width = list[0]->width;
  height = list[0]->height;

  /* Get the size of the output frame */
  for(i=1;i<n;i++) {
    width += list[i]->width;
    if(list[i]->height < height)
      height = list[i]->height;
  }

  if(allocate_output(width, height, output)) {
    return(1);
  }

  pos = 0;
  
  for(a=0;a<n;a++) {
    for(i=0;i<list[a]->width;i++) {
      for(j=0;j<height;j++)
	output->data[i+pos][j] = list[a]->data[i][j];
    }
    pos += list[a]->width;
    
  }
  return(0);
}

/* Just copy input frame to output */
int copy_frame(TFrame *input, TFrame *output)
{
  int width, height;
  int i, j;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = input->data[i][j];
    }
  }
  
  return(0);
}

/* Normalizes so that minimum is 0 and maximum is 1.0 */
int normalize_frame(TFrame *input, TFrame *output)
{
  int width, height;
  int i, j;
  float val, min, max, amp;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }
  
  /* Calculate minimum and maximum */
  min = max = input->data[0][0];
  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      val = input->data[i][j];
      if(val < min)
	min = val;
      if(val > max)
	max = val;
    }
  }
  
  amp = 1.0 / (max - min); /* Amplification factor */

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = (input->data[i][j] - min) * amp;
    }
  }
  return(0);
}

/* Amplify a frame by a given amount */
int amplify_frame(TFrame *input, TFrame *output, float factor)
{
  int width, height;
  int i, j;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = input->data[i][j] * factor;
    }
  }
  return(0);
}

/* Applies gamma correction to a frame (relative to 1.0) */
int gamma_correct_frame(TFrame *input, TFrame *output, float gamma)
{
  int width, height;
  int i, j;
  
  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      if(input->data[i][j] < 0.0) {
	output->data[i][j] = 0.0;
      }else {
	output->data[i][j] = powf(input->data[i][j], 1.0/gamma);
      }
    }
  }
  return(0);
}

/* Adds an offset to all the points */
int offset_frame(TFrame *input, TFrame *output, float midpoint)
{
  int width, height;
  int i, j;
  
  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = input->data[i][j] + midpoint;
    }
  }
  return(0);
}

/************************ SMOOTHING ALGORITHMS *******************/

// Use a median filter to despeckle an image
int despeckle_median(TFrame *input, TFrame *output, int radius)
{
  int n;
  int width, height;
  int i, j, k, l, p, mid;
  float *vals;

  width = input->width;
  height = input->height;

  if((radius < 1) || (radius > width/2))
    radius = 1;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  n = 2*radius + 1; /* Size of area */
  n *= n; /* Number of pixels */
  vals = (float*) malloc(sizeof(float)*n); /* All the values */

  mid = (n-1)/2; // n is always odd

  /* Copy boundaries */
  for(i=0;i!=width;i++) {
    for(j=0;j!=radius;j++) {
      output->data[i][j] = input->data[i][j];
      output->data[i][height-j-1] = input->data[i][height-j-1];
    }
  }
  for(i=0;i!=radius;i++) {
    for(j=0;j!=height;j++) {
      output->data[i][j] = input->data[i][j];
      output->data[width-i-1][j] = input->data[width-i-1][j];
    }
  }


  for(i=radius;i!=(width-radius);i++) {
    for(j=radius;j!=(height-radius);j++) {
      /* Despeckle about this point */
      p = 0;
      for(k=i-radius;k<=(i+radius);k++) {
	for(l=j-radius;l<=(j+radius);l++) {
	  vals[p] = input->data[k][l];
	  p++;
	}
      }

      /* Sort values */
      shell_sort(n, vals);
      
      output->data[i][j] = vals[mid];
    }
  }

  free(vals);
  return(0);
}

/* Edge-preserving filter - Kuwahara
   Window of width 2L + 1
*/
int kuwahara_filter(TFrame *input, TFrame *output, int L)
{
  int i, j, n, m;
  int width, height;
  float val, mean, sigma;
  float minmean, minsigma, num;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  /* Set boundaries */
  for(i=0;i<L;i++) {
    for(j=0;j<L;j++) {
      output->data[i][j] = input->data[i][j];
      output->data[i][height-j-1] = input->data[i][height-j-1];
      output->data[width-i-1][j] = input->data[width-i-1][j];
      output->data[width-i-1][height-j-1] = input->data[width-i-1][height-j-1];
    }
  }
  
  num = (float) L+1;
  num *= num;

  /* Calculate filter */
  for(i=L;i<(width-L);i++) {
    for(j=L;j<(width-L);j++) {
      // region 1
      minmean = 0.0; minsigma = 0.0;
      for(n=-L;n<=0;n++) {
	for(m=0;m<=L;m++) {
	  val = input->data[i+n][j+m];
	  minmean += val;
	  minsigma += val*val;
	}
      }
      minsigma -= minmean*minmean;
      
      // region 2
      mean = 0.0; sigma = 0.0;
      for(n=0;n<=L;n++) {
	for(m=0;m<=L;m++) {
	  val = input->data[i+n][j+m];
	  mean += val;
	  sigma += val*val;
	}
      }
      sigma -= mean*mean;
      if(sigma < minsigma) {
	minmean = mean;
	minsigma = sigma;
      }
      
      // region 3
      mean = 0.0; sigma = 0.0;
      for(n=-L;n<=0;n++) {
	for(m=-L;m<=0;m++) {
	  val = input->data[i+n][j+m];
	  mean += val;
	  sigma += val*val;
	}
      }
      sigma -= mean*mean;
      if(sigma < minsigma) {
	minmean = mean;
	minsigma = sigma;
      }

      // region 4
      mean = 0.0; sigma = 0.0;
      for(n=0;n<=L;n++) {
	for(m=-L;m<=0;m++) {
	  val = input->data[i+n][j+m];
	  mean += val;
	  sigma += val*val;
	}
      }
      sigma -= mean*mean;
      if(sigma < minsigma) {
	minmean = mean;
	minsigma = sigma;
      }

      output->data[i][j] = minmean / num;
    }
  }

  return(0);
}

/* Try to remove pixel noise */
int denoise_pixel(TFrame *input, TFrame *output, float amount)
{
  int i, j, x, y;
  int width, height;
  float min, max;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  /* Copy boundaries */
  for(i=0;i<width;i++) {
    output->data[i][0] = input->data[i][0];
    output->data[i][height-1] = input->data[i][height-1];
  }
  for(j=0;j<height;j++) {
    output->data[0][j] = input->data[0][j];
    output->data[width-1][j] = input->data[width-1][j];
  }

  for(i=1;i<(width-1);i++) {
    for(j=1;j<(height-1);j++) {
      /* Calculate minimum and maximum of surrounding pixels */
      min = input->data[i-1][j];
      max = min;
      for(x=(i-1);x<=(i+1);x++) {
	for(y=(j-1);y<=(j+1);y++) {
	  if((x != i) || (y != j)) { /* Exclude central pixel */
	    if(input->data[x][y] > max)
	      max = input->data[x][y];
	    if(input->data[x][y] < min)
	      min = input->data[x][y];
	  }
	}
      }

      output->data[i][j] = input->data[i][j];

      if(input->data[i][j] > max) {
	/* Central pixel is brighter than all surrounding pixels */
	output->data[i][j] -= amount * (input->data[i][j] - max);
      }
      if(input->data[i][j] < min) {
	/* pixel is dimmer than all surrounding pixels */
	output->data[i][j] += amount * (min - input->data[i][j]);
      }
    }
  }
  
  return(0);
}

int gauss_blur(TFrame *input, TFrame *output, float sigma)
{
  int i, j;
  int width, height;
  int fwidth; /* Filter width */
  FILTER filter;
  float dx;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }
  
  fwidth = (int) (6.0 * sigma); /* 3 sigma in each direction */
  if(fwidth % 2 != 0) {
    /* Should be even since will add another point (for middle) */
    fwidth += 1;
  }
  if(fwidth == 0) {
    fwidth = 2;
  }
  fwidth++; /* Include the centre point -> odd width */

  alloc_filter(&filter, fwidth, fwidth); /* a square filter */

  /* Set the values of the filter */
  for(i=0;i!=fwidth;i++) {
    for(j=0;j!=fwidth;j++) {
      dx = (float) (i - filter.x)*(i - filter.x) + (j - filter.y)*(j - filter.y);
      filter.weight[i][j] = expf(-0.5 * dx / (sigma*sigma)) / (2.0 * PI * sigma*sigma);
    }
  }

  /* Apply filter */
  apply_filter(input, &filter, output);

  /* Free the filter */
  free_filter(&filter);

  return(0);
}

/************************ SHARPEN ALGORITHMS *********************/

int sharpen_simple(TFrame *input, TFrame *output, float k)
{
  int i, j;
  int width, height;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  /* Copy boundaries */
  for(i=0;i<width;i++) {
    output->data[i][0] = input->data[i][0];
    output->data[i][height-1] = input->data[i][height-1];
  }
  for(j=0;j<height;j++) {
    output->data[0][j] = input->data[0][j];
    output->data[width-1][j] = input->data[width-1][j];
  }

  /* Sharpen image with simple algorithm */
  for(i=1;i<(width-1);i++) {
    for(j=1;j<(height-1);j++) {
      output->data[i][j] = (input->data[i][j] 
			    - k*(input->data[i][j+1] + input->data[i][j-1] +
				 input->data[i+1][j] + input->data[i-1][j])/4.0)/(1.0-k);
    }
  }
  return(0);
}

/* Unsharp masking */
int sharpen_unsharp(TFrame *input, TFrame *output, float sigma, float amount)
{
  int width, height;
  int i, j;

  width = input->width;
  height = input->height;

  /* First produce gaussian blurred frame */
  if(gauss_blur(input, output, sigma)) {
    return(0);
  }

  /* Output now contains blurred version of input */

  for(i=0;i<width;i++) {
    for(j=0;j<height;j++) {
      output->data[i][j] = input->data[i][j] + 
	amount*(input->data[i][j] - output->data[i][j]);
    }
  }

  return(0);
}


/************************ SORTING ALGORITHMS *********************/

/* Shell sort */

void shell_sort(unsigned long n, float *in)
{
  unsigned long i,j,inc;
  float v;
  float *a;

  a = in-1;

  inc=1;
  do {
    inc *= 3;
    inc++;
  } while (inc <= n);
  do { 
    inc /= 3;
    for (i=inc+1;i<=n;i++) {
      v=a[i];
      j=i;
      while (a[j-inc] > v) {
	a[j]=a[j-inc];
	j -= inc;
	if (j <= inc) break;
      }
      a[j]=v;
    }
  } while (inc > 1);
}


/************************ FILTER ALGORITHMS *********************/

float **float_array(int width, int height)
{
  float **m;
  int i;

  m = (float**) malloc(sizeof(float*)*width);
  m[0] = (float*) malloc(sizeof(float)*width*height);
  
  for(i=1;i!=width;i++) {
    m[i] = m[i-1] + height;
  }
  return(m);
}

void free_array(float **arr)
{
  free(arr[0]);
  free(arr);
}

void alloc_filter(FILTER *filter, int width, int height)
{
  filter->width = width;
  filter->height = height;
  filter->x = (width-1) / 2;
  filter->y = (height-1) / 2;
  filter->normalize = 1;
  filter->weight = float_array(width, height);
}

void free_filter(FILTER *filter)
{
  free(filter->weight[0]);
  free(filter->weight);
}

int apply_filter(TFrame *input, FILTER *filter, TFrame *output)
{
  int i, j, x, y;
  int xpos, ypos;
  float total, val;
  int width, height;

  width = input->width;
  height = input->height;

  if(allocate_output(width, height, output)) {
    return(1);
  }

  for(i=0;i!=width;i++) {
    for(j=0;j!=height;j++) {
      /* Apply filter around this point */

      total = val = 0.0;
      for(x=0; x!=filter->width; x++) {
	for(y=0; y!=filter->height; y++) {
	  /* Work out position to apply this weight to */
	  xpos = i + x - filter->x;
	  ypos = j + y - filter->y;
	  if((xpos >= 0) && (xpos < width) &&
	     (ypos >= 0) && (ypos < height)) {
	    val += input->data[xpos][ypos] * filter->weight[x][y];
	    total += filter->weight[x][y];
	  }
	}
      }
      if(filter->normalize) {
	val /= total;
      }
      output->data[i][j] = val;
    }
  }
  return(0);
}
