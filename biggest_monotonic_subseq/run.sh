set -x -e
clang++ -std=c++2b monotonic_subseq.cpp -o monotonic_subseq.exe # -Wall -O2 -DNDEBUG
x=`./monotonic_subseq.exe <<< ""`
if [ "$x" != "" ]; then
  echo "fail: $x"
  exit 1;
fi

x=`./monotonic_subseq.exe <<< "1"`
if [ "$x" != "1" ]; then
  echo "fail: $x"
  exit 1;
fi

x=`./monotonic_subseq.exe <<< "1 2"`
if [ "$x" != "1 2" ]; then
  echo "fail: $x"
  exit 1;
fi

x=`./monotonic_subseq.exe <<< "1 2 3"`
if [ "$x" != "1 2 3" ]; then
  echo "fail: $x"
  exit 1;
fi


x=`./monotonic_subseq.exe <<< "5 6 7 1 2 3 4"`
if [ "$x" != "1 2 3 4" ]; then
  echo "fail: $x"
  exit 1;
fi

x=`./monotonic_subseq.exe <<< "5 6 7 1 2 3"`
if [ "$x" != "1 2 3" ]; then
  echo "fail: $x"
  exit 1;
fi


x=`./monotonic_subseq.exe <<< "5 1 6 2 7 3"`
if [ "$x" != "1 2 3" ]; then
  echo "fail: $x"
  exit 1;
fi

x=`./monotonic_subseq.exe <<< "10 12 13 14 15 1 2 3 16 17 18 19 20"`
if [ "$x" != "10 12 13 14 15 16 17 18 19 20" ]; then
  echo "fail: $x"
  exit 1;
fi

x=`./monotonic_subseq.exe <<< "10 12 13 14 15 1 2 3 16 17 18 19 20 4 5 6 7 8 9 11 13 17"`
if [ "$x" != "1 2 3 4 5 6 7 8 9 11 13 17" ]; then
  echo "fail: $x"
  exit 1;
fi
