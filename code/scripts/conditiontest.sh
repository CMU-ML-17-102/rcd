#Script must be run from "code" working directory
make all
rm ../results/conditiontest*

for c in 1 2 4 8 16 32
do
echo $c >> ../results/conditiontest.x
./rcd qtest 1000 50 10 2 ../results/conditiontest_$c.csv schedule 0 cond $c iter 999999999 | tee -a ../results/conditiontest.out
done

cat ../results/conditiontest.out | grep Iteration | awk '{print $NF}' > ../results/conditiontest.y

paste -d ',' ../results/conditiontest.x ../results/conditiontest.y > ../results/conditiontest.csv

echo "set datafile separator ','
set term pdf
set output '../results/conditiontest.pdf'
set xlabel 'Condition Number'
set ylabel 'Number of Iterations'
set logscale x
plot '../results/conditiontest.csv' using 1:2 with line notitle' 
" | gnuplot

evince ../results/conditiontest.pdf
