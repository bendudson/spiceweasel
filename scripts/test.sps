output: diff, despec

diff : input
  subtract minimum
  #amplify 4
  normalize
  gamma 2.0

despec: diff
  #despeckle_median 1
  gauss_blur 3.0
  #kuwahara 2

usharp: diff
  unsharp_mask 4.0 1.0

norm: input
  normalize
  #amplify 4.0
