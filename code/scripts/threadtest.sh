#Script must be run from "code" working directory
set -e
S="2 3 4"

make all
mkdir -p ../results
rm -f ../results/threadtest*

for t in 1 2 3 4 5; do
	echo $t >> ../results/threadtest.x
	for s in $S; do
	    sh scripts/threadtest_run.sh $s $t &
	done
	wait

	sh scripts/threadtest_update.sh
done

for t in 6 7; do
	echo $t >> ../results/threadtest.x
	for s in 2 3; do
	    sh scripts/threadtest_run.sh $s $t &
	done
	wait

	for s in 4; do
	    sh scripts/threadtest_run.sh $s $t &
	done
	wait

	sh scripts/threadtest_update.sh
done

for t in 8 9 10 11 12 13 14 15; do
	echo $t >> ../results/threadtest.x
	for s in $S; do
	    sh scripts/threadtest_run.sh $s $t
	done

	sh scripts/threadtest_update.sh
done

