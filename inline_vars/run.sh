set -x -e
clang++ -std=c++20 inline_vars.cpp inline_vars_tu1.cpp inline_vars_tu2.cpp -o inline_vars  #-O2 -DNDEBUG 
./inline_vars | tee report.txt
