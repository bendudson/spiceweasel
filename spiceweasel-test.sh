#!/bin/bash

SWPATH="."
IPXDIR=${SWPATH}
PNGDIR=${SWPATH}

rm $SWPATH/test.txt

for thread in "--enable-threads" ""; do
	cd ${SWPATH}
	make clean
	./configure ${thread}
	make -j3

	for script in pass.sps default.sps; do
	
		for input in test.ipx png; do
			case "$input" in
				"png")
					inputexp="$PNGDIR/processed_%04d.png"
				;;
				*)
					inputexp="$IPXDIR/$input"
				;;
			esac
			
			for output in png ipx; do
				case "$output" in
					"ipx")
						outputexp=""
					;;
					*)
						outputexp="-o testout.ipx"
					;;
				esac

			
			### RUN
			score=0
			./spiceweasel 1 9 1 -i $inputexp -p scripts/$script $outputexp && score=1
			echo "$thread $script $input $output $score" >> test.txt
			### /RUN
			done
		done
	done
done

npass=$(grep '1$' test.txt | wc -l)
ntotal=$(cat test.txt | wc -l)
echo ""
echo "================ SPICEWEASEL TEST SUMMARY ================"
echo $npass of $ntotal tests passed
if test "$npass" != "$ntotal"; then
    echo "Failed tests:"
    echo $(grep '0$' test.txt)
fi
