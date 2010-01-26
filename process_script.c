/**********************************************************************************
 * Processes a script file to produce a set of processing commands
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
 *************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "spiceweasel.h"
#define SPSORIGIN 1
#include "script.h"

/* in parse_nextline.c */
int parse_nextline(FILE *fp, char* buffer, int maxbuffer, int first);

#define MAX_LINE_LEN 512

/******** Temporary structures ********
 * Used to represent script commands  *
 **************************************/

typedef struct {
  char *name;
  int linenr; /* Which line number this was defined on */
  int id;
  int nargs;
  char **args;

  int nprocess;
  TProcess **process; /* Processing steps */

  int ndep; /* Number of dependencies */
  char **dep; /* Dependencies */
  int *dep_id;

  /* Flags used when resolving/"compiling" script */
  int resolving;
  int calculated;
}TTarget;

int parse_script(FILE *script);
int resolve_script();
void dummy_script(TCommands *cmd);

/* Utility routines (at end of file) */
int strip_space(char *string);                      /* Strip all spaces from a string */
void add_dependency(TTarget *target, char *depend); /* Add a dependency to a target */
int find_dependency(TTarget *targ, char *name);     /* Find a dependency in the list */
int add_arg(TProcess *process);                     /* Add an argument to a processing step */
int add_framearg(TProcess *process, char *name);    /* Add a frame argument */
int add_floatarg(TProcess *process, char *arg);     /* Add a float argument */
int add_intarg(TProcess *proces, char *arg);        /* Add an integer argument */
int add_process(TCommands *cmd, int nargs); /* Add a processing step to a command list */
int append_processes(TCommands *cmd, int nproc, TProcess **proc);

/***** GLOBAL VARIABLES ******/
/* List of defined targets */
TTarget **target;
int ntargets;

/* Open a processing script */
int process_script(char *exe_cmd, char *file)
{
  FILE *fd;
  char buffer[256];
  char *str;
  char name[256];
  int found, i, n;

  if(file == (char*) NULL) {
    /* No script file specified - look for default file */
    strcpy(name, DEFAULT_SCRIPT);
  }else {
    strcpy(name, file);
  }

  /***********************************/

  /* Look in local directory */
  if((fd = fopen(name, "rt")) != (FILE*) NULL) {
    printf("Using processing script ./%s\n", name);

  }else {
    printf("Failed to read '%s'\n", name);
  }

  if(fd == (FILE*) NULL) {
    /* Try adding .sps to the end */
    sprintf(buffer, "%s.sps", name);
    if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
      printf("Using processing script %s\n", buffer);
    }else {
      printf("Failed to read '%s'\n", buffer);
    }
  }

  /* Look in $(pwd)/scripts directory */
    sprintf(buffer, "%s/%s", "scripts", name);
    if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
//  if((fd = fopen(name, "rt")) != (FILE*) NULL) {
    printf("Using processing script ./%s\n", buffer);

  }else {
    printf("Failed to read '%s'\n", name);
  }

  if(fd == (FILE*) NULL) {
    /* Try adding .sps to the end */
    sprintf(buffer, "%s/%s.sps", "scripts", name);
    if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
      printf("Using processing script %s\n", buffer);
    }else {
      printf("Failed to read '%s'\n", buffer);
    }
  }

  if(fd == (FILE*) NULL) {
    /* Check in the default directory */
    sprintf(buffer, "%s/%s", DEFAULT_SPS_PATH, name);
    if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
      printf("Using processing script %s\n", buffer);
    }else {
      printf("Failed to read '%s'\n", buffer);
    }
  }

  if(fd == (FILE*) NULL) {
    /* Check in the default directory with .sps */
    sprintf(buffer, "%s/%s.sps", DEFAULT_SPS_PATH, name);
    if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
      printf("Using processing script %s\n", buffer);
    }else {
      printf("Failed to read '%s'\n", buffer);
    }
  }

  if(fd == (FILE*) NULL) {
    /* Check for an environment variable */
    if((str = getenv(SCRIPT_ENV)) != (char*) NULL) {
      /* Environment variable set */
      sprintf(buffer, "%s/%s", str, name);

      if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
	printf("Using processing script %s\n", buffer);
      }else {
	printf("Failed to read '%s'\n", buffer);

	sprintf(buffer, "%s/%s.sps", str, name);
	if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
	  printf("Using processing script %s\n", buffer);
	}else {
	  printf("Failed to read '%s'\n", buffer);
	}
      }
    }
  }

  if(fd == (FILE*) NULL) {
    /* Look in the executable directory */

    n = strlen(exe_cmd);

    /* Find the last '/' in the command string */
    found = 0;
    for(i=n-1;i>=0;i--) {
      if(exe_cmd[i] == '/') {
	exe_cmd[i+1] = 0;
	found = 1;
	i = 0;
      }
    }
    if(found) { /* Not the current directory (probably) */
      sprintf(buffer, "%s%s", exe_cmd, name);

      if((fd = fopen(buffer, "rt")) != (FILE*) NULL) {
	printf("Using processing script %s\n", buffer);
      }else {
	printf("Failed to read '%s'\n", buffer);
	printf("No processing scripts found!\n");
	return(1);
      }
    }else {
      printf("No processing scripts found!\n");
      return(1);
    }
  }

  /* Read the script */
  if(parse_script(fd)) {
    return(-1);
  }
  fclose(fd);

  /* Compile dependencies */
  if(resolve_script()) {
    return(-1);
  }
  printf("Script successfully compiled\n");

  /* Do a dummy run */
  dummy_script(&command);

  return(0);
}

/*********************** READ SCRIPT **************************
 * Read in a script file and produce a list of targets, their *
 * processing steps and dependencies.                         *
 **************************************************************/

int parse_script(FILE *script)
{
  char buffer[MAX_LINE_LEN];
  char *str;
  int linenr;
  int n, i, p, q;
  int got_output;

  int next_id;
  TTarget *curtarget, *targ;
  TTarget **tmptarg;

  TProcess *curproc;
  TProcess **tmpproc;

  int nprocargs;
  char **procarg;

  curtarget = (TTarget*) NULL;
  ntargets = 0;
  next_id = 0;
  got_output = 0;

  linenr = parse_nextline(script, buffer, MAX_LINE_LEN-1, 1);
  if(linenr == -1) {
    return(1);
  }
  do {
    //printf("%d: |%s|\n", linenr, buffer);

    n = strlen(buffer);

    /* Check if this is a target definition - contains a colon */
    p = 0;
    for(i=0;i<n;i++) {
      if(buffer[i] == ':') {
	/* is a target definition */
	p = i;
	i = n;
      }
    }
    if(p > 0) {

      /* Create a new target structure */

      tmptarg = target;
      target = (TTarget**) malloc(sizeof(TTarget*)*(ntargets+1));
      if(ntargets > 0) {
	for(i=0;i<ntargets;i++)
	  target[i] = tmptarg[i];

	free(tmptarg);
      }
      target[ntargets] = (TTarget*) malloc(sizeof(TTarget));
      curtarget = target[ntargets];
      ntargets++;

      curtarget->nprocess = 0;

      /* Get name of target */
      buffer[p] = 0; /* Terminate at colon */
      str = &(buffer[p+1]); /* Pointer to arguments */
      q = strip_space(buffer); /* Strip out all spaces */

      /* Set name/ID */
      curtarget->id = next_id;  next_id++;
      curtarget->name = (char*) malloc(q+1);
      strncpy(curtarget->name, buffer, q+1);
      curtarget->name[q] = 0; /* Make sure it's null-terminated */

      curtarget->linenr = linenr;

      /* Check if name is pre-defined */

      if(strcmp(buffer, "OUTPUT") == 0) {
	/* This is the output frame */
	if(got_output) {
	  /* output already defined */
	  printf("Error line %d: Output target already defined on line %d!\n", linenr, got_output);
	  return(1);
	}
	curtarget->id = OUTPUT_FRAME;
	next_id--; /* Don't need to use this ID */
	got_output = linenr;

	/* Move output target to target[0] */
	targ = target[0];
	target[0] = target[ntargets-1];
	target[ntargets-1] = targ;

      }else if( (strcmp(buffer, "MINIMUM") == 0) ||
		(strcmp(buffer, "AVERAGE") == 0) ||
		(strcmp(buffer, "INPUT") == 0) ) {
	/* These are reserved frame names - cannot have a target called this */
	printf("Error line %d: Cannot use %s as a target - it is a predefined frame\n", linenr, buffer);
	return(1);
      }else {
	/* Check if there is already a target called this */
	for(i=0;i<(ntargets-1);i++) {
	  if(strcmp(buffer, target[i]->name) == 0) {
	    printf("Error line %d: Target %s already defined on line %d\n",
		   linenr, buffer, target[i]->linenr);
	    return(1);
	  }
	}
      }

      /* Get input frames from str */
      q = strip_space(str); /* Strip all spaces. frames separated by commas */
      if(q == 0) {
	/* No input frames */
	printf("Error line %d: No input for target %s\n", linenr, buffer);
	return(1);
      }
      /* Count number of commas */
      p = 1;
      for(i=0;i<q;i++) {
	if(str[i] == ',')
	  p++;
      }
      //printf("Target %s has %d inputs:", buffer, p);
      /* Allocate array of strings */
      curtarget->nargs = p;
      curtarget->args = (char**) malloc(sizeof(char*)*(p+1));
      curtarget->args[0] = (char*) malloc(q+1);
      strncpy(curtarget->args[0], str, q+1);
      curtarget->args[0][q] = '\0';
      p = 1;
      for(i=0;i<q;i++) {
	if(str[i] == ',') {
	  /* End of one string, beginning of another */
	  curtarget->args[0][i] = 0;
	  curtarget->args[p] = &(curtarget->args[0][i+1]);
	  p++;
	}
      }

      /* Check the arguments */
      for(i=0;i<curtarget->nargs;i++) {
	/* Check length */
	if(strlen(curtarget->args[i]) == 0) {
	  printf("Error line %d: Target %s has a zero-length input (extra comma?)\n", linenr, buffer);
	  return(1);
	}
	//printf("|%s| ", curtarget->args[i]);
      }
      //printf("\n");

      /* Copy the arguments into the dependency list */
      curtarget->dep = (char**) malloc(sizeof(char*)*curtarget->nargs);
      for(i=0;i<curtarget->nargs;i++) {
	curtarget->dep[i] = curtarget->args[i];
      }
      curtarget->ndep = curtarget->nargs;

    }else if(strncmp(buffer, "INCLUDE ", 8) == 0) {
      /* Include a file */

      printf("Sorry line %d: Eventually you will be able to include other scripts (maybe)\n", linenr);
      return(1);
    }else {
      /* Not a target or include - must be a processing step */
      if(curtarget == (TTarget*) NULL) { /* No current target */
	printf("Error line %d: No target specified here\n", linenr);
	return(1);
      }
      /* Separate the command from the arguments */
      for(i=0;i<n;i++) {
	if(buffer[i] == ' ') {
	  buffer[i] = 0;
	  str = &(buffer[i+1]);
	  i = n;
	}else if(buffer[i] == 0) {
	  /* No arguments */
	  str = &(buffer[i]);
	  i = n;
	}
      }
      /* buffer now contains command, str contains arguments */

      /* Add a processing step to the current target */
      tmpproc = curtarget->process;
      curtarget->process = (TProcess**) malloc(sizeof(TProcess*)*(curtarget->nprocess + 1));
      if(curtarget->nprocess > 0) {
	for(i=0;i<curtarget->nprocess;i++)
	  curtarget->process[i] = tmpproc[i];
	free(tmpproc);
      }
      curtarget->process[curtarget->nprocess] = (TProcess*) malloc(sizeof(TProcess));
      curproc = curtarget->process[curtarget->nprocess];
      curtarget->nprocess++;

      /* Process arguments */

      nprocargs = 0;
      q = 0;
      for(i=0;i<strlen(str);i++) {
	if(isspace(str[i]) == 0) {
	  /* Not a space */
	  if(q == 0) {
	    q = 1;
	    nprocargs++;
	  }
	}else {
	  q = 0;
	}
      }
      //printf("%d args: ", nprocargs);
      if(nprocargs > 0) {
	/* Separate into separate strings */
	procarg = (char**) malloc(sizeof(char*)*(nprocargs+1));
	procarg[0] = (char*) malloc(strlen(str)+1);
	strcpy(procarg[0], str);
	p = 1;
	q = 0;
	for(i=0;i<strlen(str);i++) {
	  if(isspace(str[i]) == 0) {
	    /* Not a space */
	    if(q == 0) {
	      q = 1;
	      procarg[p] = &(procarg[0][i]);
	    }
	  }else {
	    q = 0;
	    procarg[0][i] = 0;
	  }
	}
	/*
	for(i=0;i<nprocargs;i++) {
	  printf("|%s| ", procarg[i]);
	}
	*/
      }
      //printf("\n");
      curproc->nargs = 0;

      /* Test what the command is */

      if(strcmp(buffer, "SUBTRACT") == 0) {
	/* Expect a single argument - should be a frame */
	curproc->method = PROC_SUBTRACT;
	if(nprocargs != 1) {
	  printf("Error line %d: Subtract has one argument\n", linenr);
	  return(1);
	}
	/* Argument is the name of a frame - copy into argument */
	add_framearg(curproc, procarg[0]);
	/* Add to dependency list */
	add_dependency(curtarget, procarg[0]);

      }else if(strcmp(buffer, "NORMALIZE") == 0) {
	curproc->method = PROC_NORMALIZE;
	if(nprocargs != 0) {
	  printf("Error line %d: Normalize has no arguments\n", linenr);
	  return(1);
	}

      }else if(strcmp(buffer, "AMPLIFY") == 0) {
	curproc->method = PROC_AMPLIFY;
	/* Should have one floating-point argument */
	if(nprocargs != 1) {
	  printf("Error line %d: Amplify has one argument\n", linenr);
	  return(1);
	}
	if(add_floatarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to amplify is a floating point number\n", linenr);
	  return(1);
	}
      }else if(strcmp(buffer, "GAMMA") == 0) {
	curproc->method = PROC_GAMMA;
	if(nprocargs != 1) {
	  printf("Error line %d: Gamma has one argument\n", linenr);
	  return(1);
	}
	if(add_floatarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to gamma is a floating point number\n", linenr);
	  return(1);
	}
      }else if(strcmp(buffer, "OFFSET") == 0) {
	curproc->method = PROC_OFFSET;
	if(nprocargs != 1) {
	  printf("Error line %d: Offset has one argument\n", linenr);
	  return(1);
	}
	if(add_floatarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to offset is a floating point number\n", linenr);
	  return(1);
	}
      }else if(strcmp(buffer, "DESPECKLE_MEDIAN") == 0) {
	curproc->method = PROC_DESPECKLE_MEDIAN;
	if(nprocargs != 1) {
	  printf("Error line %d: Despeckle_median has one argument\n", linenr);
	  return(1);
	}
	if(add_intarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to despeckle_median is an integer\n", linenr);
	  return(1);
	}
      }else if(strcmp(buffer, "KUWAHARA") == 0) {
	curproc->method = PROC_KUWAHARA;
	if(nprocargs != 1) {
	  printf("Error line %d: Kuwahara has one argument\n", linenr);
	  return(1);
	}
	if(add_intarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to kuwahara is an integer\n", linenr);
	  return(1);
	}
      }else if(strcmp(buffer, "SHARPEN") == 0) {
	curproc->method = PROC_SHARPEN;
	if(nprocargs != 1) {
	  printf("Error line %d: Sharpen has one argument\n", linenr);
	  return(1);
	}
	if(add_floatarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to sharpen is a floating point number\n", linenr);
	  return(1);
	}

      }else if(strcmp(buffer, "UNSHARP_MASK") == 0) {
	curproc->method = PROC_UNSHARP_MASK;
	if(nprocargs != 2) {
	  printf("Error line %d: Unsharp_mask has 2 arguments\n", linenr);
	  return(1);
	}
	if(add_floatarg(curproc, procarg[0])) {
	  printf("Error line %d: First argument to unsharp_mask is a floating point number\n", linenr);
	  return(1);
	}
	if(add_floatarg(curproc, procarg[1])) {
	  printf("Error line %d: Second argument to unsharp_mask is a floating point number\n", linenr);
	  return(1);
	}
      }else if(strcmp(buffer, "GAUSS_BLUR") == 0) {
	curproc->method = PROC_GAUSSBLUR;
	if(nprocargs != 1) {
	  printf("Error line %d: Gauss_blur takes one argument\n", linenr);
	}
	if(add_floatarg(curproc, procarg[0])) {
	  printf("Error line %d: Argument to gauss_blur is floating point number (sigma)\n", linenr);
	  return(1);
	}
      }else {
	printf("Error line %d: Unknown processing command %s in target %s\n", linenr, buffer, curtarget->name);
	return(1);
      }


    }/* End of process */

  }while((linenr = parse_nextline(script, buffer, MAX_LINE_LEN-1, 0)) != -1);

  return(0);
}

/*********************** RESOLVE SCRIPT ***********************
 * Takes a list of targets and produces a sequential set of   *
 * processing steps, resolving dependencies.                  *
 **************************************************************/

int resolve_script_rec(char *name);

int resolve_script()
{
  int i, j;
  int n;

  /* Clear flags in targets */
  for(i=0;i<ntargets;i++) {
    target[i]->resolving = 0;   /* 1 if currently being resolved (prevent loops) */
    target[i]->calculated = UNKNOWN_FRAME; /* The ID of the frame for this target */
  }

  command.ntemp = 0;      /* No intermediate frames */
  command.minimum_frame = UNKNOWN_FRAME; /* No minimum frame */
  command.average_frame = UNKNOWN_FRAME; /* No average frame */
  command.nsteps = 0;     /* No processing steps */

  n = resolve_script_rec("OUTPUT"); /* Resolve the output target */

  if((n != command.ntemp-1) || (n == UNKNOWN_FRAME)) { /* If worked properly, output should be last frame allocated */
    printf("Could not compile script\n");
    return(-1);
  }
  /* Need to replace last frame with output in all commands */
  for(i=0;i<command.nsteps;i++) {
    if(command.step[i].input == n)
      command.step[i].input = OUTPUT_FRAME;
    if(command.step[i].result == n)
      command.step[i].result = OUTPUT_FRAME;
    /* Go through arguments */
    for(j=0;j<command.step[i].nargs;j++) {
      if(command.step[i].args[j].frame == n) {
	command.step[i].args[j].frame = OUTPUT_FRAME;
      }
    }
  }
  command.ntemp--; /* Don't need last intermediate frame */
  return(0);
}

/* Recursive code to resolve dependencies.
   Passed the name of a frame, returns ID */
int resolve_script_rec(char *name)
{
  int i, j, p, q;
  TTarget *curtarget;
  TProcess *curproc;
  int next_input; /* ID of the input to the next stage */

  //printf("Resolving %s\n", name);

  /* Check for pre-defined frames */
  if(strcmp(name, "INPUT") == 0) {
    /* Just the input frame */
    return(INPUT_FRAME);
  }else if(strcmp(name, "MINIMUM") == 0) {
    if(command.minimum_frame == UNKNOWN_FRAME) {
      /* Calculate minimum and put into temporary frame */
      command.minimum_frame = command.ntemp;
      command.ntemp++;
      //printf("Assigning minimum frame to %d\n", command.minimum_frame);
    }

    return(command.minimum_frame);
  }else if(strcmp(name, "AVERAGE") == 0) {
    if(command.average_frame == UNKNOWN_FRAME) {
      /* Calculate average and put into temporary frame */
      command.average_frame = command.ntemp;
      command.ntemp++;
    }
    return(command.average_frame);
  }

  /* Find the name in the list of targets */
  p = -1;
  for(i=0;i<ntargets;i++) {
    if(strcmp(target[i]->name, name) == 0) {
      /* Found target */
      p = i;
      i = ntargets;
    }
  }

  if(p == -1) {
    /* Target not found */
    printf("Error: Frame %s is not defined\n", name);
    return(UNKNOWN_FRAME);
  }

  curtarget = target[p];

  if(curtarget->resolving) {
    /* This target is currently being resolved => Circular dependency */
    printf("Error: target %s (line %d) has a circular dependency\n", name, curtarget->linenr);
    return(UNKNOWN_FRAME);
  }

  if(curtarget->calculated == UNKNOWN_FRAME) {
    /* frame not calculated yet */
    //printf("Frame %s not calculated yet\n", curtarget->name);
    /**************** Resolve dependencies **************/

    curtarget->resolving = 1; /* Currently resolving */
    curtarget->dep_id = (int*) malloc(sizeof(int)*curtarget->ndep);
    for(i=0;i<curtarget->ndep;i++) {
      curtarget->dep_id[i] = resolve_script_rec(curtarget->dep[i]);
      if(curtarget->dep_id[i] == UNKNOWN_FRAME) {
	/* Could not resolve */
	printf("=> Could not resolve target %s (line %d)\n", name, curtarget->linenr);
	return(UNKNOWN_FRAME);
      }
    }
    curtarget->resolving = 0; /* Done resolving */

    /*********** Produce list of processing commands ***********/

    /* Grab a frame number for the result */
    curtarget->calculated = command.ntemp;
    command.ntemp++;
    //printf("Assigning %s to frame %d\n", curtarget->name, curtarget->calculated);

    /* Initial command - could be concatenate */
    if(curtarget->nargs > 1) {
      /* More than one argument - have to concatenate */
      p = add_process(&command, curtarget->nargs);
      curproc = &(command.step[p]);

      curproc->method = PROC_CONCATENATE;
      curproc->input = UNKNOWN_FRAME; /* Leave blank - all args are inputs */
      curproc->result = curtarget->calculated; /* Output frame */
      for(i=0;i<curtarget->nargs;i++) {
	/* Set the input frames */
	curproc->args[i].frame = find_dependency(curtarget, curtarget->args[i]);
	if(curproc->args[i].frame == UNKNOWN_FRAME)
	  return(UNKNOWN_FRAME);
      }
      next_input = curtarget->calculated;
    }else if(curtarget->nprocess == 0) {
      /* No processing steps - copy input to output (not very useful, but hey) */
      p = add_process(&command, 0);
      curproc = &(command.step[p]);

      curproc->method = PROC_COPY;
      /* Set the input frame */
      curproc->input = find_dependency(curtarget, curtarget->args[0]);
      /* Set output frame */
      curproc->result = curtarget->calculated;
      next_input = curtarget->calculated;
    }else {
      /* Just the one input being processed */
      next_input = find_dependency(curtarget, curtarget->args[0]); /* Read the input */
    }

    /* Append the processing steps onto end of list. Returns index of first one added */
    q = append_processes(&command, curtarget->nprocess, curtarget->process);

    /* Go through the new commands */
    for(i=q;i<(curtarget->nprocess+q);i++) {
      curproc = &(command.step[i]);

      /* Set input and output */
      curproc->input = next_input;
      curproc->result = curtarget->calculated;

      /* Need to resolve arguments which are frames */
      if(curproc->method == PROC_SUBTRACT) {
	/* One argument - a frame */
	curproc->args[0].frame = find_dependency(curtarget, curproc->args[0].name);
      }

      /* Some processing cannot have the same input as output - check array */
      if(proc_noio[0] != PROC_NULL) { /* If there are any such commands */
	j = 0;
	do {
	  if(curproc->method == proc_noio[j]) {
	    /* This processing step is one of these
	     * Check that the input and output are different */
	    if(curproc->input == curproc->result) {
	      /* Grab a new frame number */
	      curproc->result = command.ntemp;
	      command.ntemp++;
	      curtarget->calculated = curproc->result; /* Change the target frame */
	      //printf("Changed %s to frame %d\n", curtarget->name, curtarget->calculated);
	    }
	    j = -1;
	  }
	}while((proc_noio[j+1] != PROC_NULL) && (j > 0));
      }

      next_input = curproc->result; /* Set the next input to be the result of this step */

      /* Finished this processing step */
    }
    /* Finished compiling target */
    return(curtarget->calculated);
  }else {
    /* frame already calculated */
    //printf("Target %s calculated in %d\n", curtarget->name, curtarget->calculated);
    return(curtarget->calculated);
  }
  return(UNKNOWN_FRAME); /* Should never reach here */
}

/********************** DUMMY-RUN SCRIPT **********************
 * Goes through the command list printing out steps           *
 **************************************************************/

void targ_str(int id)
{
  if(id == UNKNOWN_FRAME) {
    printf("UNKNOWN");
  }else if(id == OUTPUT_FRAME) {
    printf("OUTPUT");
  }else if(id == INPUT_FRAME) {
    printf("INPUT");
  }else {
    printf("<%d>", id);
  }
}

void dummy_script(TCommands *cmd)
{
  int i, j;
  TProcess *proc;
  printf("\n======== SCRIPT OPERATIONS =========\n");
  printf("Number of intermediate frames: %d\n", cmd->ntemp);
  /* Calculate background frames */
  if(cmd->minimum_frame != UNKNOWN_FRAME) {
    printf("Calculate minimum => ");
    targ_str(cmd->minimum_frame);
    printf("\n");
  }
  if(cmd->average_frame != UNKNOWN_FRAME) {
    printf("Calculate average => ");
    targ_str(cmd->average_frame);
    printf("\n");
  }
  /* Go through commands */
  for(i=0;i<cmd->nsteps;i++) {
    proc = &(cmd->step[i]);
    switch(proc->method) {
    case PROC_SUBTRACT: {
      targ_str(proc->input);
      printf(" - ");
      targ_str(proc->args[0].frame);
      printf(" => ");
      targ_str(proc->result);
      break;
    }
    case PROC_NORMALIZE: {
      printf("Normalize(");
      targ_str(proc->input);
      printf(") => ");
      targ_str(proc->result);
      break;
    }
    case PROC_AMPLIFY: {
      targ_str(proc->input);
      printf(" * %f => ", proc->args[0].fval);
      targ_str(proc->result);
      break;
    }
    case PROC_GAMMA: {
      printf("Gamma(");
      targ_str(proc->input);
      printf(", %f) => ", proc->args[0].fval);
      targ_str(proc->result);
      break;
    }
    case PROC_OFFSET: {
      targ_str(proc->input);
      printf(" + %f => ", proc->args[0].fval);
      targ_str(proc->result);
      break;
    }
    case PROC_DESPECKLE_MEDIAN: {
      printf("Despeckle(");
      targ_str(proc->input);
      printf(", %d) => ", proc->args[0].ival);
      targ_str(proc->result);
      break;
    }
    case PROC_KUWAHARA: {
      printf("Kuwahara(");
      targ_str(proc->input);
      printf(", %d) => ", proc->args[0].ival);
      targ_str(proc->result);
      break;
    }
    case PROC_SHARPEN: {
      printf("Sharpen(");
      targ_str(proc->input);
      printf(", %f) => ", proc->args[0].fval);
      targ_str(proc->result);
      break;
    }
    case PROC_UNSHARP_MASK: {
      printf("Unsharp_mask(");
      targ_str(proc->input);
      printf(", %f, %f) => ", proc->args[0].fval, proc->args[1].fval);
      targ_str(proc->result);
      break;
    }
    case PROC_CONCATENATE: {
      printf("Concatenate(");
      for(j=0;j<proc->nargs;j++) {
	targ_str(proc->args[j].frame);
	if(j != (proc->nargs-1)) {
	  printf(", ");
	}
      }
      printf(") => ");
      targ_str(proc->result);
      break;
    }
    case PROC_COPY: {
      printf("Copy ");
      targ_str(proc->input);
      printf(" => ");
      targ_str(proc->result);
      break;
    }
    case PROC_GAUSSBLUR: {
      printf("Gaussian blur(");
      targ_str(proc->input);
      printf(", %f) => ", proc->args[0].fval);
      targ_str(proc->result);
      break;
    }
    default: {
      printf("Error! Unrecognised command %d", cmd->step[i].method);
    }
    }

    printf("\n");
  }
  printf("===================================\n");
}

/*********************** UTILITY ROUTINES *********************/

/* Strips all spaces from the string */
int strip_space(char *string)
{
  int i, n, p;

  n = strlen(string);

  p = 0;
  for(i=0;i<n;i++) {
    if(isspace(string[i]) == 0) {
      /* Not a space */
      string[p] = string[i];
      p++;
    }
  }
  string[p] = 0; /* New termination */

  return(p);
}

/* Add a dependency to a target */
void add_dependency(TTarget *targ, char *depend)
{
  char **tmp;
  int i;

  /* Check if the dependency is already in the list */
  for(i=0;i<targ->ndep;i++) {
    if(strcmp(targ->dep[i], depend) == 0) {
      /* Already in list */
      return;
    }
  }

  tmp = targ->dep;

  targ->dep = (char**) malloc(sizeof(char*)*(targ->ndep+1));
  if(targ->ndep > 0) {
    for(i=0;i<targ->ndep;i++) {
      targ->dep[i] = tmp[i];
    }
    free(tmp);
  }
  targ->dep[targ->ndep] = depend;
  targ->ndep++;
}

int find_dependency(TTarget *targ, char *name)
{
  int n, i;

  n = UNKNOWN_FRAME;
  for(i=0;i<targ->ndep;i++) {
    if(strcmp(targ->dep[i], name) == 0) {
      n = i;
    }
  }
  if(n != UNKNOWN_FRAME)
    n = targ->dep_id[n];
  return(n);
}

/* Adds an argument to a processing step */
int add_arg(TProcess *process)
{
  TProcArg *tmp;

  tmp = process->args;
  process->args = (TProcArg*) malloc(sizeof(TProcArg)*(process->nargs+1));
  if(process->nargs > 0) {
    /* Copy over the data */
    memcpy(process->args, tmp, sizeof(TProcArg)*process->nargs);
    /* Free the old array */
    free(tmp);
  }
  memset(&(process->args[process->nargs]), 0, sizeof(TProcArg));
  process->nargs++;
  return(process->nargs-1);
}

/* Add a frame argument to a processing step */
int add_framearg(TProcess *process, char *name)
{
  int n, p;

  n = strlen(name);
  if(n < 1) {
    return(1);
  }

  p = add_arg(process);
  process->args[p].name = (char*) malloc(sizeof(char)*(n+1));
  strcpy(process->args[p].name, name);
  process->args[p].frame = UNKNOWN_FRAME;

  return(0);
}

int add_floatarg(TProcess *process, char *arg)
{
  float val;
  int p;

  if(sscanf(arg, "%f", &val) != 1) {
    return(1);
  }

  p = add_arg(process);
  process->args[p].fval = val;

  return(0);
}

/* Add an integer argument to a processing step */
int add_intarg(TProcess *process, char *arg)
{
  int val;
  int p;

  if(sscanf(arg, "%d", &val) != 1) {
    return(1);
  }

  p = add_arg(process);
  process->args[p].ival = val;

  return(0);
}

/* Add a processing step to a command list */
int add_process(TCommands *cmd, int nargs)
{
  TProcess *tmp;
  int i, n;

  tmp = cmd->step;
  cmd->step = (TProcess*) malloc(sizeof(TProcess)*(cmd->nsteps+1));
  if(cmd->nsteps > 0) {
    memcpy(cmd->step, tmp, sizeof(TProcess)*cmd->nsteps);
    free(tmp);
  }

  n = cmd->nsteps;
  cmd->nsteps++;

  cmd->step[n].method = PROC_NULL; /* No operation */
  cmd->step[n].nargs = nargs;
  if(nargs > 0) {
    /* Allocate memory */
    cmd->step[n].args = (TProcArg*) malloc(sizeof(TProcArg)*nargs);
    for(i=0;i<nargs;i++) {
      cmd->step[n].args[i].name = (char*) NULL;
      cmd->step[n].args[i].frame = UNKNOWN_FRAME;
    }
  }
  return(n);
}

/* Append a set of processes onto the end of the list */
int append_processes(TCommands *cmd, int nproc, TProcess **proc)
{
  TProcess *tmp;
  int i, n;

  tmp = cmd->step;
  cmd->step = (TProcess*) malloc(sizeof(TProcess)*(nproc+cmd->nsteps));
  if(cmd->nsteps > 0) {
    /* Copy current processing steps */
    memcpy(cmd->step, tmp, sizeof(TProcess)*cmd->nsteps);
    /* Free old data */
    free(tmp);
  }
  /* Append new processing steps */
  for(i=0;i<nproc;i++) {
    memcpy(&(cmd->step[cmd->nsteps+i]), proc[i], sizeof(TProcess));
  }
  n = cmd->nsteps;
  cmd->nsteps += nproc;
  return(n);
}
