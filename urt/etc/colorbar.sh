#!/bin/sh
scale=${1-1.0}
value=`echo $scale|awk '{print int(255 * $1)}'`
rlebg      0      0      0 -s 96 512 > 0
rlebg      0      0 $value -s 96 512 | repos -i  96 0 | comp - 0 > 1 ; rm 0
rlebg      0 $value      0 -s 96 512 | repos -i 192 0 | comp - 1 > 2 ; rm 1
rlebg      0 $value $value -s 96 512 | repos -i 288 0 | comp - 2 > 3 ; rm 2
rlebg $value      0      0 -s 96 512 | repos -i 384 0 | comp - 3 > 4 ; rm 3
rlebg $value      0 $value -s 96 512 | repos -i 480 0 | comp - 4 > 5 ; rm 4
rlebg $value $value      0 -s 96 512 | repos -i 576 0 | comp - 5 > 6 ; rm 5
rlebg $value $value $value -s 96 512 | repos -i 672 0 | comp - 6 > bars-$scale.rle
rm 6
