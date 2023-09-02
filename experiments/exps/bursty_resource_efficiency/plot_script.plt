#!/usr/bin/gnuplot -persist
set terminal pdf font "Latin Modern Roman, 16"
set output "plot.pdf"
set key autotitle columnhead
set key right top
              
set key top left
set xlabel "Time (seconds)"
set ylabel "Goodput [Mbps]"
set yrange [0:15000]
set xrange [200:400]
set xtics 10

plot 'virt-tas-50-6-tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 4 title 'virtuoso-6' , \
     'virt-tas-50-4-tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 1 title 'virtuoso-4', \
     'virt-tas-50-2-tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 2 title 'virtuoso-2', \
     'ovs-tas-50-1-tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 3 title 'ovs-tas'
