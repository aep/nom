#!/bin/sh
make || exit
echo > testresult
cd testcases/
for i in *
do
    printf "==> %-25s " $i
    ../parser $i/input.mk  > $i/result
    r=$(diff -w -T -u2  $i/expected $i/result)
    if [ $? = 0 ]  ; then
        echo "pass" | tee -a ../testresult
    else
        echo "$r" | tail -n +3
        echo "fail" >> ../testresult
    fi
done


numFail=$(grep fail  ../testresult |  wc -l)
numPass=$(grep pass  ../testresult |  wc -l)

echo
echo "passed: ${numPass}"
echo "failed: ${numFail}"
