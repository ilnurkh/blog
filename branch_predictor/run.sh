set -x -e
clang++ -std=c++23 bp.cpp -o bp.exe -Wall -O2 -DNDEBUG
./bp.exe 
