/*****************************************************************
 * High Dynamic Range Compression
 *
 * Method by Raanan Fattal, Dani Lishinski and Michael Werman
 *****************************************************************/
#include <stdlib.h>
#include "spiceweasel.h"

void calc_gradient(TFrame *input, float **x, float **y);

int hdr_fattal(TFrame *input, TFrame *output)
{
  int width, height;
  float **dx, **dy;

  width = input->width;
  height = input->height;

  /* Allocate memory */
  dx = float_array(width, height);
  dy = float_array(width, height);

  /* Calculate gradient of frame */
  calc_gradient(input, dx, dy);

  /* Compute gradient attenuation function */

  /* Calculate Div G */

  /* Invert to get new image */
  
  /* Free memory */
  free(dx[0]);
  free(dy[0]);

  return(0);
}

/* Calculates gradient reduction factor using a recursive pyramid */
void gradient_pyramid(TFrame *input, float **phi, FILTER *filter, int maxlevels, int reduce)
{
  TFrame frame_small; /* Reduce size frame */
  float **phi_small;
  int width, height;
  
  /* Calculate size of reduced frame */
  
  /* Allocate memory */
  phi_small = float_array(width, height);
  frame_small.data = float_array(width, height);
  frame_small.width = width;
  frame_small.height = height;
  frame_small.allocated = 1;

  /* Smooth using filter, reduce to smaller size */

  /* Send to next level */
  gradient_pyramid(&frame_small, phi_small, filter, maxlevels-1, reduce);
  
  /* Interpolate phi_small onto phi */

  /* Calculate derivative of frame */

  /* Calculate phi */
}


/* Gradient calculation */
void calc_gradient(TFrame *input, float **x, float **y)
{
  int i, j;

  for(i=1;i<(input->width-1);i++) {
    for(j=1;j<(input->height-1);j++) {
      x[i][j] = input->data[i+1][j] - input->data[i-1][j];
      y[i][j] = input->data[i][j+1] - input->data[i][j-1];
    }
  }
  
  for(i=0;i<input->width;i++) {
    x[i][0] = 0.0;
    y[i][0] = 0.0;
    x[i][input->height-1] = 0.0;
    y[i][input->height-1] = 0.0;
  }
  
  for(j=0;j<input->height;j++) {
    x[0][j] = 0.0;
    y[0][j] = 0.0;
    x[input->width-1][j] = 0.0;
    y[input->width-1][j] = 0.0;
  }
}
