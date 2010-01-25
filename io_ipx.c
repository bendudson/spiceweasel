/*********************************************************************
 * Code to read/write IPX files
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io_ipx.h"
#include "spiceweasel.h"

#ifdef HAVE_OPENJPEG_OPENJPEG_H
#include "openjpeg/openjpeg.h"
#else
#include "openjpeg.h"
#endif

/*********************** IPX HEADER ROUTINES *************************/

/* Callback functions */
void error_callback(const char *msg, void *client_data);
void warning_callback(const char *msg, void *client_data);
void info_callback(const char *msg, void *client_data);

/* Number of bytes in header elements */
#define IPX_SHORT 2
#define IPX_UINT 4
#define IPX_FLOAT 4
#define IPX_DOUBLE 8

/* Need to read in element-by-element due to byte packing */
int ipx_read_header(FILE *fd, IPX_header *header)
{
  fread(&(header->id), 1, 8, fd);
  fread(&(header->size), IPX_UINT, 1, fd);
  if(header->size != 286) {
    printf("Error: Cannot handle IPX headers not 286 bytes long\n");
    return(1);
  }
  fread(&(header->codec), 1, 8, fd);
  fread(&(header->date_time), 1, 20, fd);
  fread(&(header->shot), IPX_UINT, 1, fd);
  fread(&(header->trigger), IPX_FLOAT, 1, fd);
  fread(&(header->lens), 1, 24, fd);
  fread(&(header->filter), 1, 24, fd);
  fread(&(header->view), 1, 64, fd);
  fread(&(header->numFrames), IPX_UINT, 1, fd);
  fread(&(header->camera), 1, 64, fd);
  fread(&(header->width), IPX_SHORT, 1, fd);
  fread(&(header->height), IPX_SHORT, 1, fd);
  fread(&(header->depth), IPX_SHORT, 1, fd);
  fread(&(header->orient), IPX_UINT, 1, fd);
  fread(&(header->taps), IPX_SHORT, 1, fd);
  fread(&(header->color), IPX_SHORT, 1, fd);
  fread(&(header->hBin), IPX_SHORT, 1, fd);
  fread(&(header->left), IPX_SHORT, 1, fd);
  fread(&(header->right), IPX_SHORT, 1, fd);
  fread(&(header->vBin), IPX_SHORT, 1, fd);
  fread(&(header->top), IPX_SHORT, 1, fd);
  fread(&(header->bottom), IPX_SHORT, 1, fd);
  fread(header->offset, IPX_SHORT, 2, fd);
  fread(header->gain, IPX_FLOAT, 2, fd);
  fread(&(header->preExp), IPX_UINT, 1, fd);
  fread(&(header->exposure), IPX_UINT, 1, fd);
  fread(&(header->strobe), IPX_UINT, 1, fd);
  fread(&(header->board_temp), IPX_FLOAT, 1, fd);
  fread(&(header->ccd_temp), IPX_FLOAT, 1, fd);

  return(0);
}

int ipx_write_header(FILE *fd, IPX_header *header)
{
  header->size = 286;

  fwrite(&(header->id), 1, 8, fd);
  fwrite(&(header->size), IPX_UINT, 1, fd);
  fwrite(&(header->codec), 1, 8, fd);
  fwrite(&(header->date_time), 1, 20, fd);
  fwrite(&(header->shot), IPX_UINT, 1, fd);
  fwrite(&(header->trigger), IPX_FLOAT, 1, fd);
  fwrite(&(header->lens), 1, 24, fd);
  fwrite(&(header->filter), 1, 24, fd);
  fwrite(&(header->view), 1, 64, fd);
  fwrite(&(header->numFrames), IPX_UINT, 1, fd);
  fwrite(&(header->camera), 1, 64, fd);
  fwrite(&(header->width), IPX_SHORT, 1, fd);
  fwrite(&(header->height), IPX_SHORT, 1, fd);
  fwrite(&(header->depth), IPX_SHORT, 1, fd);
  fwrite(&(header->orient), IPX_UINT, 1, fd);
  fwrite(&(header->taps), IPX_SHORT, 1, fd);
  fwrite(&(header->color), IPX_SHORT, 1, fd);
  fwrite(&(header->hBin), IPX_SHORT, 1, fd);
  fwrite(&(header->left), IPX_SHORT, 1, fd);
  fwrite(&(header->right), IPX_SHORT, 1, fd);
  fwrite(&(header->vBin), IPX_SHORT, 1, fd);
  fwrite(&(header->top), IPX_SHORT, 1, fd);
  fwrite(&(header->bottom), IPX_SHORT, 1, fd);
  fwrite(header->offset, IPX_SHORT, 2, fd);
  fwrite(header->gain, IPX_FLOAT, 2, fd);
  fwrite(&(header->preExp), IPX_UINT, 1, fd);
  fwrite(&(header->exposure), IPX_UINT, 1, fd);
  fwrite(&(header->strobe), IPX_UINT, 1, fd);
  fwrite(&(header->board_temp), IPX_FLOAT, 1, fd);
  fwrite(&(header->ccd_temp), IPX_FLOAT, 1, fd);

  return(0);
}

/*********************** IPX READING ROUTINES ************************/

/* Open an IPX file for reading */
int IPX_read_open(char *filename, IPX_status *status)
{
  int i, offset;

  /* Open IPX file */
  if((status->fd = fopen(filename, "rb")) == (FILE*) NULL) {
    status->header.numFrames = 0;
    return(1);
  }

  /* Read IPX header */
  if(ipx_read_header(status->fd, &(status->header))) {
    fclose(status->fd);
    status->header.numFrames = 0;
    return(2);
  }

  /* Allocate memory for list of frames */
  status->frames = (IPX_frame*) malloc(sizeof(IPX_frame)*status->header.numFrames);
  
  /* Read frame headers */
  offset = status->header.size;
  for(i=0;i<status->header.numFrames;i++) {
    if(fseek(status->fd, offset, SEEK_SET)) {
      /* Error reading */
      fclose(status->fd);
      free(status->frames);
      status->header.numFrames = 0;
      return(2);
    }
    status->frames[i].offset = offset;
    fread(&(status->frames[i].size), IPX_UINT, 1, status->fd);
    fread(&(status->frames[i].time), IPX_DOUBLE, 1, status->fd);
    offset += status->frames[i].size;
  }
  /* Finished! */
  return(0);
}

/* Read a frame from an IPX file */
int IPX_read_frame(int fnr, TFrame *frame, IPX_status *status)
{
  int offset, size;
  int i, j, p;
  static unsigned int data_max = 0;
  static unsigned char *data;
  float factor;

  /* JPEG 2000 variables */
  static int jp2_init = 1;
  static opj_dparameters_t parameters;	/* decompression parameters */
  static opj_dinfo_t* dinfo = NULL;	/* handle to a decompressor */
  static opj_event_mgr_t event_mgr;		/* event manager */
  opj_cio_t *cio = NULL;
  opj_image_t *image = NULL;

  if(fnr >= status->header.numFrames) {
    return(1);
  }
  
  offset = status->frames[fnr].offset + IPX_UINT + IPX_DOUBLE; //sizeof(uint) + sizeof(double);
  if(fseek(status->fd, offset, SEEK_SET)) {
    return(2);
  }

  size = status->frames[fnr].size - IPX_UINT - IPX_DOUBLE; //sizeof(uint) - sizeof(double);
  
  /* Allocate memory */
  if(data_max < size) {
    if(data_max == 0) {
      data = (unsigned char*) malloc(size);
    }else {
      data = (unsigned char*) realloc(data, size);
    }
    data_max = size;
  }
  
  /* Read data */
  if(fread(data, size, 1, status->fd) != 1) {
    return(2);
  }
  
  /* Decode JP2 image */

  if(jp2_init) {
    jp2_init = 0;
    /* set decoding parameters to default values */
    opj_set_default_decoder_parameters(&parameters);

    /* get a decoder handle */
    dinfo = opj_create_decompress(CODEC_JP2);
    
    /* Setup callbacks */
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;
    
    /* catch events using our callbacks and give a local context */
    opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, stderr);

    /* setup the decoder decoding parameters */
    opj_setup_decoder(dinfo, &parameters);
  }

  /* open a byte stream */
  cio = opj_cio_open((opj_common_ptr)dinfo, data, size);

  /* decode the stream and fill the image structure */
  image = opj_decode(dinfo, cio);
  if(!image) {
    opj_cio_close(cio);
    return(3);
  }
  
  /* close the byte stream */
  opj_cio_close(cio);

  /* Copy image into TRawFrame structure */

  /* Check frame data is allocated. If not, allocate it */
  if(allocate_output(status->header.width, status->header.height, frame)) {
    /* Frame is wrong size */
    return(4);
  }
  
  factor = (float) ((1 << image->comps[0].prec) - 1);
  //factor = (float) ((1 << 16) - 1);

  p = 0;
  for(j=0;j<status->header.height;j++) {
    for(i=0;i<status->header.width;i++) {
      frame->data[i][j] = ((float) image->comps[0].data[p]) / factor;
      p++;
    }
  }

  if(fnr == (status->header.numFrames-1)) {
    /* This is the last frame */
    frame->last = 1;
  }
  
  /* free image data structure */
  opj_image_destroy(image);

  frame->time = status->frames[fnr].time;

  return(0);
}

/* Close an IPX file opened for reading */
int IPX_read_close(IPX_status *status)
{
  fclose(status->fd);

  if(status->header.numFrames > 0)
    free(status->frames);
  status->header.numFrames = 0;
  
  return(0);
}

/*********************** IPX WRITING ROUTINES ************************
 * When writing, header fields should already be set except:
 * - id
 * - codec
 * - size
 * - numFrames
 * - width
 * - height
 * - depth
 * Which are overwritten by this code
 *********************************************************************/

/**
sample error callback expecting a FILE* client object
*/
void error_callback(const char *msg, void *client_data) {
	FILE *stream = (FILE*)client_data;
	fprintf(stream, "[ERROR] %s", msg);
}
/**
sample warning callback expecting a FILE* client object
*/
void warning_callback(const char *msg, void *client_data) {
	FILE *stream = (FILE*)client_data;
	fprintf(stream, "[WARNING] %s", msg);
}
/**
sample debug callback expecting a FILE* client object
*/
void info_callback(const char *msg, void *client_data) {
	FILE *stream = (FILE*)client_data;
	//fprintf(stream, "[INFO] %s", msg);
}

/* Open an IPX file for writing (overwrite) */
int IPX_write_open(char *filename, int precision, IPX_status *status)
{
  if((status->fd = fopen(filename, "wb")) == (FILE*) NULL) {
    return(1);
  }

  /* Write a dummy header - will overwrite when finished */

  status->header.numFrames = 0;
  status->header.width = 0;
  status->header.height = 0;
  status->header.depth = precision;

  strcpy(status->header.id, "IPX 01");
  strcpy(status->header.codec, "JP2");

  if(ipx_write_header(status->fd, &(status->header))) {
    fclose(status->fd);
    return(2);
  }
  return(0);
}

/* Add a frame to the end of an IPX file */
int IPX_write_frame(TFrame *frame, IPX_status *status)
{
  opj_image_t *image = NULL;
  uint codestream_length, datasize;
  opj_cio_t *cio = NULL;
  opj_image_cmptparm_t cmptparm;
  
  int i, j, p;
  float factor;

  static int jp2_init = 1;
  static opj_event_mgr_t event_mgr;		/* event manager */
  static opj_cparameters_t parameters;	/* compression parameters */
  static opj_cinfo_t* cinfo; 

  if((status->header.height == 0) || (status->header.width == 0)) {
    /* Width and height not set yet */
    status->header.height = frame->height;
    status->header.width = frame->width;
  }

  if((status->header.height != frame->height) || 
     (status->header.width != frame->width)) {
    /* Frame is wrong size */
    return(IO_ERROR_SIZE);
  }

  if(jp2_init) {
    jp2_init = 0;
    /* Setup callbacks */
    memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;
    
    /* Set default compression parameters */
    opj_set_default_encoder_parameters(&parameters);
    
    /* Get a compressor handle */
    cinfo = opj_create_compress(CODEC_JP2);
  }
  
  /* Create an image structure */
  memset(&cmptparm, 0, sizeof(opj_image_cmptparm_t));

  /* Set image parameters */
  cmptparm.prec = status->header.depth;
  cmptparm.bpp = cmptparm.prec;
  cmptparm.sgnd = 0;
  cmptparm.dx = parameters.subsampling_dx;
  cmptparm.dy = parameters.subsampling_dy;
  cmptparm.w = frame->width;
  cmptparm.h = frame->height;

  /* Create image */
  image = opj_image_create(1, &cmptparm, CLRSPC_GRAY);
  if(!image) {
    printf("Error: Cannot create image\n");
    exit(0);
  }

  /* set image offset and reference grid */
  image->x0 = parameters.image_offset_x0;
  image->y0 = parameters.image_offset_y0;
  image->x1 = parameters.image_offset_x0 + 
    (cmptparm.w - 1) * parameters.subsampling_dx + 1;
  image->y1 = parameters.image_offset_y0 + 
    (cmptparm.h - 1) * parameters.subsampling_dy + 1;

  factor = (float) ((1 << cmptparm.prec) - 1);
  p = 0;
  for(j=0;j<status->header.height;j++) {
    for(i=0;i<status->header.width;i++) {
      image->comps[0].data[p] = (int) (factor * frame->data[i][j]);
      p++;
    }
  }
  image->comps[0].bpp = cmptparm.bpp;

  /* if no rate entered, lossless by default */
  if(parameters.tcp_numlayers == 0) {
    parameters.tcp_rates[0] = 0;
    parameters.tcp_numlayers++;
    parameters.cp_disto_alloc = 1;
  }

  /* catch events using our callbacks and give a local context */
  //opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, stderr);	

  /* setup the encoder parameters using the current image and using user parameters */
  opj_setup_encoder(cinfo, &parameters, image);

  /* open a byte stream for writing */
  /* allocate memory for all tiles */
  if((cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0)) == NULL) {
    printf("Error: could not open CIO buffer\n");
    exit(0);
  }

  /* encode the image */
  if(!opj_encode(cinfo, cio, image, NULL)) {
    opj_cio_close(cio);
    return(IO_ERROR_OTHER);
  }
  /* Length of JP2 code stream */
  codestream_length = cio_tell(cio);
  /* Length of header + JP2 data */
  datasize = codestream_length + IPX_UINT + IPX_DOUBLE; //sizeof(uint) + sizeof(double);

  /*
  printf("Writing frame %d at position %ld, size %d\n", 
  	 status->header.numFrames, ftell(status->fd), codestream_length);
  */

  /* Write frame header */
  //printf("Writing header: %ld, %d ->", ftell(status->fd), sizeof(uint)+sizeof(double));

  fwrite(&datasize, IPX_UINT, 1, status->fd);
  fwrite(&(frame->time), IPX_DOUBLE, 1, status->fd);
  
  //printf(" %ld\n", ftell(status->fd));

  /* write the frame data */
  fwrite(cio->buffer, 1, codestream_length, status->fd);

  /* Update header */
  status->header.numFrames++;
  
  /* Free memory */
  opj_cio_close(cio);
  opj_image_destroy(image);
  
  return(0);
}

/* Close a file opened for writing */
int IPX_write_close(IPX_status *status)
{
  /* Set right and bottom so consistent with width and height */
  status->header.right = status->header.left + status->header.width - 1;
  status->header.bottom = status->header.top + status->header.height - 1;

  /* Go to start of file */
  if(fseek(status->fd, 0L, SEEK_SET)) {
    return(1);
  }

  /* Write proper header */
  if(ipx_write_header(status->fd, &(status->header))) {
    return(2);
  }
  
  /* Close the file */
  fclose(status->fd);

  /*
    if(status->header.numFrames > 0)
    free(status->frames);
  */
  status->header.numFrames = 0;
  
  return(0);
}

