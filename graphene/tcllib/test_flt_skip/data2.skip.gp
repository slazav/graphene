#!/usr/bin/gnuplot

plot\
 "data2.dat" u 1:2 w lp pt 6 ps 0.8,\
 "data2_skip.dat" u 1:2 w lp pt 7 ps 0.5,\

pause -1

set terminal png
set output "data1_skip.png"

replot