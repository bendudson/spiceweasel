/* Header file for processing using a script 
   Used by process_script.c and run_script.c */

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

/* Set the default script name */
#define DEFAULT_SCRIPT "default.sps"

/* Default search directory for scripts */
#ifndef DEFAULT_SPS_PATH
#define DEFAULT_SPS_PATH "/usr/local/share/spiceweasel/"
#endif

/* The environment variable containing a path to scripts */
#define SCRIPT_ENV    "SPS_PATH"

/* Frame identifiers */
#define UNKNOWN_FRAME  -1
#define OUTPUT_FRAME   -2
#define INPUT_FRAME    -3

/* Processing methods */
#define PROC_NULL            -1
#define PROC_SUBTRACT         0
#define PROC_NORMALIZE        1
#define PROC_AMPLIFY          2
#define PROC_GAMMA            3
#define PROC_OFFSET           4
#define PROC_DESPECKLE_MEDIAN 5
#define PROC_KUWAHARA         6
#define PROC_SHARPEN          7
#define PROC_UNSHARP_MASK     8
#define PROC_CONCATENATE      9
#define PROC_COPY            10
#define PROC_GAUSSBLUR       11

/* Some processing methods cannot have the same input as output.
   List these in the following array, end array with PROC_NULL */

#ifdef SPSORIGIN
int proc_noio[6] = {PROC_DESPECKLE_MEDIAN, PROC_KUWAHARA, 
		    PROC_SHARPEN, PROC_UNSHARP_MASK, 
		    PROC_GAUSSBLUR, PROC_NULL};
#else
extern int *proc_noio;
#endif


/******* Frame processing steps *******
 * These structures used when running *
 **************************************/

typedef struct { /* An argument to a processing step */
  char *name; /* Name of frame (not used when processing) */
  float fval;
  int ival;
  int frame; /* ID of frame */
}TProcArg;

typedef struct {  /* Define a processing step */
  int method;       /* Which method to use */
  int nargs;        /* Number of arguments */
  int input;        /* Input frame */
  TProcArg *args;   /* Array of arguments */
  int result;       /* Result frame */
}TProcess;

typedef struct { /* Set of sequential commands for processing frames */
  int ntemp; /* Number of intermediate frames needed */
  int minimum_frame; /* The ID of the minimum frame */
  int average_frame; /* ID of average frame */
  int nsteps;  /* Number of processing steps */
  TProcess *step; /* List of processing steps */
}TCommands;

/********* GLOBAL VARIABLES ***********/
#ifdef SPSORIGIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL TCommands command; /* Defines the processing to be done */

#undef GLOBAL

#endif /* __SCRIPT_H__ */
