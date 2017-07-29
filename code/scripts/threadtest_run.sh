s=$1
t=$2
cmd="./rcd qtest 10000 500 100 4 ../results/threadtest_$s-$t.csv schedule $s threads $t target 711 iter 999999999 | tee -a ../results/threadtest_$s.out"
echo $cmd
echo $cmd >> "../results/threadtest_commandlog"
eval $cmd
