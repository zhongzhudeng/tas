#!/usr/bin/gnuplot -persist
set terminal pdf font "Latin Modern Roman"
set output "plot.pdf"
set key autotitle columnhead
set key left top
set logscale x 2
              
set key top left
set xlabel "Message Size [Bytes]"
set ylabel "Goodput [Mbps]"
set yrange [0:]

plot 'tp.dat' using 1:2:8 with yerrorlines title 'tas' linetype 1, \
     'tp.dat' using 1:3:8 with yerrorlines title 'virtuoso' linetype 2, \
     'tp.dat' using 1:4:9 with yerrorlines title 'ovs-tas' linetype 3, \
     'tp.dat' using 1:5:10 with yerrorlines title 'linux' linetype 5, \
     'tp.dat' using 1:6:11 with yerrorlines title 'ovs-linux' linetype 4
