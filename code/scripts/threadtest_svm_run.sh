t=$1

#a7a.txt: -5698.02 -5698
#w8a.txt: -1486.3 -1486.15

DATA=w8a.txt
OBJ=-1486.15

cmd="./bin/opt/trainLinearSVM --train_file=../data/$DATA --num_threads=$t --min_obj=$OBJ --iterations=-1 | tee -a ../results/threadtest.out"
echo $cmd
echo $cmd >> "../results/threadtest_commandlog"
eval $cmd
