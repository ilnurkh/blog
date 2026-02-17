set -x -e
clang++ -std=c++20 main.cpp -o nonatomic.exe -Wall -O2 -DNDEBUG 
./nonatomic.exe
./nonatomic.exe "aligned"
