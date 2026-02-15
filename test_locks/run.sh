set -x -e
clang++ -std=c++20 test_locks.cpp -o test_locks.exe -Wall -O2 -DNDEBUG 
./test_locks.exe | tee report.txt
