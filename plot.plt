set term pngcairo
set output "L128.png"

set xlabel "Bond Probability"
set ylabel "Peroclation Probability"

p "output/L128.dat" w e pt 6 t "L=128"
