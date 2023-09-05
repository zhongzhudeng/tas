#!/usr/bin/gnuplot -persist
set terminal pdf font "Latin Modern Roman"
set output "plot.pdf"
set key autotitle columnhead
set key bottom right

set datafile separator whitespace
stats 'bare-tas_128_hist.dat' u 2 nooutput

set table 'bare-tas_128_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'bare-tas_128_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'virt-tas_128_hist.dat' u 2 nooutput

set table 'virt-tas_128_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'virt-tas_128_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'ovs-tas_128_hist.dat' u 2 nooutput

set table 'ovs-tas_128_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'ovs-tas_128_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'bare-linux_128_hist.dat' u 2 nooutput

set table 'bare-linux_128_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'bare-linux_128_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table


set datafile separator whitespace
stats 'ovs-linux_128_hist.dat' u 2 nooutput

set table 'ovs-linux_128_cdf.dat'
cumulative = 0
total_count = STATS_sum
plot 'ovs-linux_128_hist.dat' u ($0==0 ? cumulative=0 : cumulative=cumulative+$2, $1):(cumulative/total_count) with lines title 'CDF'
unset table



set yrange [0:1]
set xrange [0:1000]
set xlabel "Latency [us]"
set ylabel "Cumulative Frequency"
plot 'bare-tas_128_cdf.dat' with lines title 'tas', \
     'virt-tas_128_cdf.dat' with lines title 'virtuoso', \
     'ovs-tas_128_cdf.dat' with lines title 'ovs-tas', \
     'bare-linux_128_cdf.dat' with lines title 'linux', \
     'ovs-linux_128_cdf.dat' with lines title 'ovs-dpdk',