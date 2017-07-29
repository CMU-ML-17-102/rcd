#Script must be run from "code" working directory
make all
rm ../results/layouttest*
./rcd qtest 1000 100 20 1 ../results/layouttest_ring.csv schedule 2 iter 10000 eps -1 | tee ../results/layouttest.out
./rcd qtest 1000 100 20 2 ../results/layouttest_clique.csv schedule 2 iter 10000 eps -1 | tee -a ../results/layouttest.out 
./rcd qtest 1000 100 20 3 ../results/layouttest_tree.csv schedule 2 iter 10000 eps -1 | tee -a ../results/layouttest.out
./rcd qtest 1000 100 20 4 ../results/layouttest_star.csv schedule 2 iter 10000 eps -1 | tee -a ../results/layouttest.out
./rcd qtest 1000 100 20 5 ../results/layouttest_supertree.csv schedule 2 iter 10000 eps -1 | tee -a ../results/layouttest.out

echo "set datafile separator ','
set term pdf
set output '../results/layouttest.pdf'
set xlabel 'Iteration'
set ylabel 'Objective'
plot '../results/layouttest_ring.csv' using 1:3 with line title 'Ring' \
, '../results/layouttest_clique.csv' using 1:3 with line title 'Clique' \
, '../results/layouttest_tree.csv' using 1:3 with line title 'Tree' \
, '../results/layouttest_star.csv' using 1:3 with line title 'Star+Ring' \
, '../results/layouttest_supertree.csv' using 1:3 with line title 'Tree+Ring' 
" | gnuplot



