set -x -e

clang++ -std=c++2b sort_ub.cpp -o sort_ub.exe -Wall -O2 -DNDEBUG
set +e
./sort_ub.exe 9
./sort_ub.exe 3
./sort_ub.exe 2
./sort_ub.exe 1
./sort_ub.exe

