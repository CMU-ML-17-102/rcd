#Script must be run from "code" working directory
CFG=$1

make all CONFIG=$CFG
./bin/$CFG/rcd qtest 10 2 2 0 /dev/null schedule 3 iter 999999999 constraint 0 threads 4 eps 1e-6
