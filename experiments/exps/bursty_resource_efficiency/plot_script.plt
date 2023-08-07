#!/usr/bin/gnuplot -persist
set terminal pdf font "Latin Modern Roman, 16"
set output "plot.pdf"
set key autotitle columnhead
set key right top
              
set key top left
set xlabel "Time (seconds)"
set ylabel "Goodput [Mbps]"
set yrange [0:8000]
set xrange [0:100]

plot 'virt_tas_3_tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 4 title 'virtuoso-3-cores' , \
     'virt_tas_2_tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 1 title 'virtuoso-2-cores', \
     'virt_tas_1_tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 2 title 'virtuoso-1-core', \
     'ovs_tas_1_tp.dat' using ($1 - 20):($2) with lines lw 3 linetype 3 title 'ovs-tas'
