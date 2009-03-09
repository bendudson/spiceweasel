# Modified unsharp masking with some despeckling

mask_input: input
    despeckle_median 1

blurmask: mask_input 
    GAUSS_blur 4.0 # this is sigma
    subtract mask_input
    # at this point this is the change
    # produced by bluring the image
    amplify 3.0  # this controls how much the image
                 # will be adjusted.

output: input
    # reverse change produced by bluring
    SUBTRACT blurmask

