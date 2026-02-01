set -x -e
clang++ -std=c++20 mem_random_access.cpp -o exe_mem_random_access -O2 -DNDEBUG

echo "" > report.txt

./exe_mem_random_access 1 | tee -a report.txt
echo "" >> report.txt
./exe_mem_random_access 128 | tee -a report.txt

cat report.txt
