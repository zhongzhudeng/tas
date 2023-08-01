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

plot 'tp.dat' using 1:2:8 with yerrorlines title 'tas', \
     'tp.dat' using 1:3:9 with yerrorlines title 'bare-virtuoso', \
     'tp.dat' using 1:4:10 with yerrorlines title 'virtuoso', \
     'tp.dat' using 1:5:11 with yerrorlines title 'ovs-tas', \
     'tp.dat' using 1:6:12 with yerrorlines title 'linux', \
     'tp.dat' using 1:7:13 with yerrorlines title 'ovs-dpdk'
