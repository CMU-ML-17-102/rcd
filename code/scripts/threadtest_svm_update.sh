#Update results
cat ../results/threadtest.out | grep Time | awk '{print $NF}' > ../results/threadtest.y

paste -d ',' ../results/threadtest.x ../results/threadtest.y > ../results/threadtest_all.csv

python3 scripts/time_to_speedup.py < ../results/threadtest_all.csv > ../results/threadtest_speedup.csv

echo "set datafile separator ','
set term pdf
set output '../results/threadtest.pdf'
set xlabel 'P'
set ylabel 'Time (ms)'
plot '../results/threadtest_all.csv' using 1:2 with line" | gnuplot

echo "set datafile separator ','
set term pdf
set output '../results/threadtest_speedup.pdf'
set xlabel 'P'
set ylabel 'Speedup'
plot '../results/threadtest_speedup.csv' using 1:1 with line title 'optimal'\
, '../results/threadtest_speedup.csv' using 1:2 with line title 'actual'" | gnuplot

#, '../results/threadtest_speedup.csv' using 1:2 with line title 'global sync' \

#echo "set datafile separator ','
#set term pdf
#set output '../results/threadtest_progress.pdf'
#set xlabel 'Time'
#set ylabel 'Objective'
#plot '../results/threadtest_0-2.csv' using 2:3 with line title 'global sync' \
#, '../results/threadtest_2-2.csv' using 2:3 with line title 'local sync (double lock)'\
#, '../results/threadtest_3-2.csv' using 2:3 with line title 'local sync (single lock)'\
#, '../results/threadtest_4-2.csv' using 2:3 with line title 'lock-free'" | gnuplot

