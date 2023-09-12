#!/usr/bin/gnuplot -persist
set terminal pdf font "Latin Modern Roman"
set output "plot.pdf"
set key autotitle columnhead
set key bottom right
set colorsequence podo

set datafile separator whitespace
stats 'bare-tas_hist.dat' u 2 nooutput

set table 'bare-tas_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'bare-tas_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


# set datafile separator whitespace
# stats 'bare-vtas_hist.dat' u 2 nooutput

# set table 'bare-vtas_cdf.dat'
# cumulative = 0
# total_count = STATS_sum
# plot 'bare-vtas_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
# unset table


set datafile separator whitespace
stats 'virt-tas_hist.dat' u 2 nooutput

set table 'virt-tas_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'virt-tas_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'ovs-tas_hist.dat' u 2 nooutput

set table 'ovs-tas_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'ovs-tas_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'bare-linux_hist.dat' u 2 nooutput

set table 'bare-linux_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'bare-linux_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'ovs-linux_hist.dat' u 2 nooutput

set table 'ovs-linux_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'ovs-linux_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table



# set yrange [0:1]
# set xrange [0:1000]
# set xlabel "Latency [us]"
# set ylabel "Cumulative Frequency"
# plot 'bare-tas_cdf.dat' with lines dt 1 lw 4 title 'tas', \
#      'virt-tas_cdf.dat' with lines dt 2  lw 4 title 'virtuoso', \
#      'ovs-tas_cdf.dat' with lines dt 3 lw 4 title 'ovs-tas', \
#      'bare-linux_cdf.dat' with lines dt 4 lw 4 title 'linux', \
#      'ovs-linux_cdf.dat' with lines dt 5 lw 4 title 'ovs-linux',