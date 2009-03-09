# Default Spice-weasel Processing Script
# This is a Makefile/Haskell-like script which defines how to process the input frames
# Not case-sensitive, hash starts a comment.

# Processing blocks begin with the name of the result (target). The frames after the colon
# are placed side-by-side (concatenated) and used as input - must have at least
# one frame as input. The order of the processing blocks doesn't matter.

# Pre-defined frames are INPUT, MINIMUM and AVERAGE. 
# These names cannot be used as targets.

# Every script must have an output block
OUTPUT: input, minimum, diffed  # concatenate 3 frames to produce output
   amplify 2

orig: INPUT             # define what orig is - start with input frame
     AMPLIFY 2   # amplify input frame by factor of 2

gamma: orig      # start with frame "orig"
     GAMMA 1.5   # gamma correct with gamma = 1.5
 
diffed: INPUT                # start with input frame
     SUBTRACT MINIMUM        # subtract the minimum background frame
     AMPLIFY 2               # amplify by factor of 2
     GAMMA 2.0               # apply gamma correction
#     DESPECKLE_MEDIAN 1      # despeckle using median filter (radius 1 pixel)
#     UNSHARP_MASK 4.0 1.0    # perform unsharp masking


