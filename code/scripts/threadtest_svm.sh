#Script must be run from "code" working directory
set -e

make all
mkdir -p ../results
rm -f ../results/threadtest*

for t in 1 2 3 4; do
	echo $t >> ../results/threadtest.x
    sh scripts/threadtest_svm_run.sh $t &
	wait

	sh scripts/threadtest_svm_update.sh
done

for t in 5 6; do
	echo $t >> ../results/threadtest.x
    sh scripts/threadtest_svm_run.sh $t &
	wait

	sh scripts/threadtest_svm_update.sh
done

for t in 7 8 9 10 11 12 13 14 15; do
	echo $t >> ../results/threadtest.x
    sh scripts/threadtest_svm_run.sh $t

	sh scripts/threadtest_svm_update.sh
done

