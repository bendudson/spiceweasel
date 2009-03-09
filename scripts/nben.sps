
output: amplified
	#subtract blurmask

amplified:input
	subtract minimum
	gamma 1.3
	amplify 8.0
	
mask_input:amplified
	despeckle_median 1

blurmask: mask_input
	gauss_blur 4.0
	subtract mask_input
	amplify 3.0

	
