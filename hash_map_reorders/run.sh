set -x -e
clang++ -std=c++23 reorder.cpp -o reorder.exe -Wall -O2 -DNDEBUG
./reorder.exe | tee report.txt
