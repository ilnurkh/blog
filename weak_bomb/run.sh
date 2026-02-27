set -x -e
clang++ -std=c++20 -c tu_avx512.cpp -o tu_avx512.a -msse4.2 -mavx -mavx2 -mavx512f
clang++ -std=c++20 -c tu_common.cpp -o tu_common.a -msse4.2 -mavx -mavx2
# for t in `ls *.a`; do  echo "$t"; objdump $t -t -C | grep prep; done
clang++ -std=c++20 main.cpp tu_avx512.a tu_common.a -o weak_bomb.exe
clang++ -std=c++20 main.cpp tu_common.a tu_avx512.a -o weak_bomb2.exe

set -x +e
./weak_bomb.exe 
./weak_bomb2.exe 
