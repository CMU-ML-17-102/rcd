#Script must be run from "code" working directory
make all
mkdir -p ../results
#rm ../results/synctest*

#for t in 1 2 3 4; do
#echo $t >> ../results/synctest.x
#for s in 1 10 100 500; do
#cmd="./rcd qtest 500 50 10 2 ../results/synctest_$s-$t.csv schedule 1 threads $t sync $s iter 999999999 | tee -a ../results/synctest_$s.out"
#echo $cmd
#eval $cmd
#done; done

for s in 1 10 100 500; do
cat ../results/synctest_$s.out | grep Time | awk '{print $NF}' > ../results/synctest_$s.time.y
cat ../results/synctest_$s.out | grep Iter | awk '{print $NF}' > ../results/synctest_$s.iter.y
done

paste -d ',' ../results/synctest.x ../results/synctest_1.time.y ../results/synctest_10.time.y ../results/synctest_100.time.y ../results/synctest_500.time.y  > ../results/synctest_time_all.csv
paste -d ',' ../results/synctest.x ../results/synctest_1.iter.y ../results/synctest_10.iter.y ../results/synctest_100.iter.y ../results/synctest_500.iter.y  > ../results/synctest_iter_all.csv

echo "set datafile separator ','
set term pdf
set output '../results/synctest.pdf'
set xlabel 'P'
set ylabel 'Time (ms)'
plot '../results/synctest_time_all.csv' using 1:2 with line title '1' \
, '../results/synctest_time_all.csv' using 1:3 with line title '10' \
, '../results/synctest_time_all.csv' using 1:4 with line title '100' \
, '../results/synctest_time_all.csv' using 1:5 with line title '500' 
set term x11
replot
pause mouse 'Click on graph to terminate'
" | gnuplot

echo "set datafile separator ','
set term pdf
set output '../results/synctest.pdf'
set xlabel 'P'
set ylabel 'Iterations'
plot '../results/synctest_iter_all.csv' using 1:2 with line title '1' \
, '../results/synctest_iter_all.csv' using 1:3 with line title '10' \
, '../results/synctest_iter_all.csv' using 1:4 with line title '100' \
, '../results/synctest_iter_all.csv' using 1:5 with line title '500' 
set term x11
replot
pause mouse 'Click on graph to terminate'
" | gnuplot


