set -x -e
# clang++ -std=c++2b bp.cpp -o empty.exe -Wall -O2 -DNDEBUG
# perf stat ./empty.exe 2>&1| tee empty1.txt | grep branch
# perf stat ./empty.exe 2>&1| tee empty2.txt | grep branch
# perf stat ./empty.exe 2>&1| tee empty3.txt | grep branch

clang++ -std=c++2b bp.cpp tp2.cpp -o bp.exe -Wall -O0 -DNDEBUG
# perf stat ./bp.exe 1000000 1 0 2>&1 | tee run1.txt
# perf stat ./bp.exe 1000000 1 1 2>&1 | tee run2.txt
# perf stat ./bp.exe 1000000 1 2 2 2>&1 | tee run3.txt
perf stat ./bp.exe 10000000 700 500 1013 2>&1 | grep branches | tee run4_1.txt
perf stat ./bp.exe 20000000 700 500 1013 2>&1 | grep branches | tee run4_2.txt
perf stat ./bp.exe 30000000 700 500 1013 2>&1 | grep branches | tee run4_3.txt

